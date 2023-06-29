#include <QApplication>

#include "dir/Monitor.hpp"
#include "exe/Exe.hpp"
#include "net/ServerEndpoint.hpp"
#include "po/ProgramOptions.hpp"
#include "ui/ServerWindow.hpp"

namespace asio = boost::asio;
using namespace std::chrono_literals;

asio::awaitable<void> exitClicked(exe::ExecutionContext auto& context)
{
    asio::steady_timer timer{ context, 100ms };
    while (true)
    {
        co_await timer.async_wait(asio::use_awaitable);
        if (ui::isUIActive())
        {
            timer.expires_after(100ms);
        }
        else
        {
            spdlog::critical("UI is closed!");
            // stop
        }
    }
}

asio::awaitable<void> updateUI(exe::ExecutionContext auto& context, ui::ServerWindow& window, po::ServerOptions& opts)
{
    dir::Monitor monitor{ context, opts.outDir };

    asio::cancellation_state state = co_await asio::this_coro::cancellation_state;
    state.slot().assign(
        [&](auto /*type*/)
        {
            monitor.cancel();
            window.cancel();
        });

    while (true)
    {
        auto imagePath = co_await monitor.getNewImage(boost::asio::use_awaitable);
        
        if (state.cancelled() != asio::cancellation_type::none)
        {
            spdlog::critical("updateUI coroutine is canceled!");
            co_return;
        }

        window.asyncImageUpdate(std::move(imagePath));
    }
}

asio::awaitable<void> receiveImages(exe::ExecutionContext auto& context, po::ServerOptions& opts)
{
    net::ServerEndpoint server{ context, opts.outDir, opts.serverPort, opts.timeout };

    asio::cancellation_state state = co_await asio::this_coro::cancellation_state;
    state.slot().assign(
        [&](auto /*type*/)
        {
            server.cancel();
        });

    co_await server.doListen();
    
    if (state.cancelled() != asio::cancellation_type::none)
    {
        spdlog::critical("receiveImages coroutine is canceled!");
    }

    co_return;
}

asio::awaitable<void> asyncMain(exe::ExecutionContext auto& context, ui::ServerWindow& window, po::ServerOptions& opts)
{
    spdlog::info("Starting async Main...");
    using namespace asio::experimental::awaitable_operators;

    try
    {
        co_await (receiveImages(context, opts) && updateUI(context, window, opts));
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

        using namespace asio::experimental::awaitable_operators;
        exe::submit(io, asyncMain(io, window, options) || exe::stopOnSignals(io, SIGINT));

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