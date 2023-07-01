#include <fmt/color.h>
#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <filesystem>

#include "dir/Monitor.hpp"
#include "exe/Exe.hpp"
#include "gst/Camera.hpp"
#include "net/ClientEndpoint.hpp"
#include "po/ProgramOptions.hpp"

namespace
{
  namespace asio = boost::asio;
  namespace fs   = std::filesystem;
}  // namespace

boost::asio::awaitable<void> uploadImages(const po::ClientOptions& opts)
{
  auto executor = co_await asio::this_coro::executor;
  auto state    = co_await asio::this_coro::cancellation_state;

  net::ClientEndpoint endpoint{ executor, opts.serverIp, std::to_string(opts.serverPort), opts.timeout };
  dir::Monitor monitor{ executor, opts.outDir, IN_MOVED_TO };

  while (true)
  {
    fs::path imagePath = co_await monitor.getNewImage1();
    if (!imagePath.empty())
    {
      co_await endpoint.sendFile(std::move(imagePath));
    }

    if (state.cancelled() != asio::cancellation_type::none)
    {
      spdlog::critical("Canceling uploadImages coroutine...");
      co_return;
    }
  }
}

boost::asio::awaitable<void> takeCameraShots(const po::ClientOptions& opts)
{
  auto executor = co_await asio::this_coro::executor;
  auto state    = co_await asio::this_coro::cancellation_state;

  gst::Camera camera{ executor, opts.videoDevice, opts.outDir };

  while (true)
  {
    gst::PipelineMessage msg = co_await camera.take1();

    if (state.cancelled() != asio::cancellation_type::none)
    {
      spdlog::critical("Canceling takeCameraShots coroutine...");
      co_return;
    }

    if (msg != gst::PipelineMessage::EoS)
    {
      spdlog::error("Error received from pipeline!");
      throw std::runtime_error("Pipeline error");
    }
  }
}

asio::awaitable<void> asyncMain(const po::ClientOptions& opts)
{
  spdlog::info("Starting the async main...");

  try
  {
    co_await exe::whenAll(takeCameraShots(opts), uploadImages(opts));
  }
  catch (const std::exception& ex)
  {
    spdlog::error("Exception thrown from asyncMain: ");
    spdlog::error("{}", ex.what());
  }

  spdlog::info("Exiting the async main...");
  co_return;
}

int main(int argc, char* argv[])
{
  try
  {
    po::ClientOptions opts;
    if (!po::parse(argc, argv, opts))
    {
      return EXIT_FAILURE;
    }

    gst_init(nullptr, nullptr);

    asio::io_context io;

    exe::whenOneOf(io, asyncMain(opts), exe::stopOnSignals(SIGINT), exe::stopAfter(opts.recTime));

    io.run();
  }
  catch (const std::exception& e)
  {
    spdlog::error("{}", e.what());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}