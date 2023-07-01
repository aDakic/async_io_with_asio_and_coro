#include "dir/Monitor.hpp"
#include "exe/Exe.hpp"
#include "net/ServerEndpoint.hpp"
#include "po/ProgramOptions.hpp"
#include "ui/ServerWindow.hpp"

namespace asio = boost::asio;
using namespace std::chrono_literals;

asio::awaitable<void> updateUI(ui::ServerWindow& window, po::ServerOptions& opts)
{
  auto executor = co_await asio::this_coro::executor;
  auto state    = co_await asio::this_coro::cancellation_state;

  dir::Monitor monitor{ executor, opts.outDir, IN_CLOSE_WRITE };

  while (true)
  {
    fs::path imagePath = co_await monitor.getNewImage1();
    if (!imagePath.empty())
    {
      window.asyncImageUpdate(std::move(imagePath));
    }

    if (state.cancelled() != asio::cancellation_type::none)
    {
      spdlog::critical("Canceling updateUI coroutine...");
      co_return;
    }
  }
}

asio::awaitable<void> receiveImages(po::ServerOptions& opts)
{
  auto executor = co_await asio::this_coro::executor;
  auto state    = co_await asio::this_coro::cancellation_state;

  net::ServerEndpoint server{ executor, opts.outDir, opts.serverPort, opts.timeout };

  co_await server.doListen();

  if (state.cancelled() != asio::cancellation_type::none)
  {
    spdlog::critical("Canceling receiveImages coroutine...");
  }

  co_return;
}

asio::awaitable<void> asyncMain(ui::ServerWindow& window, po::ServerOptions& opts)
{
  spdlog::info("Starting async Main...");

  try
  {
    co_await exe::whenAll(receiveImages(opts), updateUI(window, opts));
    window.requestQuit();
  }
  catch (const std::exception& ex)
  {
    spdlog::error("Exception thrown from asyncMain: ");
    spdlog::error("{}", ex.what());
  }

  spdlog::info("Exiting async Main...");
  co_return;
}

int main(int argc, char* argv[])
{
  try
  {
    po::ServerOptions options;
    if (!po::parse(argc, argv, options))
    {
      return EXIT_FAILURE;
    }

    QApplication ui(argc, argv);
    ui::ServerWindow window;

    asio::io_context io;

    exe::whenOneOf(io, asyncMain(window, options), exe::stopOnSignals(SIGINT));

    std::jthread t{ [&]() { io.run(); } };

    window.show();
    ui.exec();
  }
  catch (const std::exception& e)
  {
    spdlog::error("{}", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}