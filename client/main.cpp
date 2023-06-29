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

namespace asio = boost::asio;
namespace fs   = std::filesystem;

boost::asio::awaitable<void> uploadImages(exe::ExecutionContext auto& context, const po::ClientOptions& opts)
{
    net::ClientEndpoint endpoint{ context, opts.serverIp, std::to_string(opts.serverPort), opts.timeout };
    dir::Monitor monitor{ context, opts.outDir };

    asio::cancellation_state state = co_await asio::this_coro::cancellation_state;
    state.slot().assign([&](auto /*type*/) { monitor.cancel(); });

    while (true)
    {
        spdlog::warn("ispred monitora {}", std::this_thread::get_id());
        auto imagePath = co_await monitor.getNewImage(boost::asio::use_awaitable);

        if (state.cancelled() != asio::cancellation_type::none)
        {
            spdlog::critical("Canceling images upload...");
            co_return;
        }

        spdlog::info("New image found: {}", imagePath);
        spdlog::warn("ispred endpointa {}", std::this_thread::get_id());
        exe::submit(context, endpoint.sendFile(std::move(imagePath)));
    }

    co_return;
}

boost::asio::awaitable<void> takeCameraShots(exe::ExecutionContext auto& context, const po::ClientOptions& opts)
{
    asio::system_context ctx;
    gst::Camera camera{ ctx, opts.videoDevice, opts.outDir };

    asio::cancellation_state state = co_await asio::this_coro::cancellation_state;
    state.slot().assign([&](auto /*type*/) { camera.cancel(); });

    while (true)
    {
        spdlog::warn("ispred camere {}", std::this_thread::get_id());
        gst::PipelineMessage msg = co_await camera.take(boost::asio::use_awaitable);

        if (state.cancelled() != asio::cancellation_type::none)
        {
            spdlog::critical("Canceling takeCameraShots coroutine");
            co_return;
        }

        if (msg != gst::PipelineMessage::EoS)
            co_return;
    }
}

asio::awaitable<void> asyncMain(exe::ExecutionContext auto& context, const po::ClientOptions& opts)
{
    spdlog::info("Starting the async main...");
    using namespace asio::experimental::awaitable_operators;

    try
    {
        co_await (takeCameraShots(context, opts) && uploadImages(context, opts));
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
        asio::thread_pool pool{ 3ul };

        using namespace asio::experimental::awaitable_operators;
        
        const auto now = std::chrono::high_resolution_clock::now();

        exe::submit(io, asyncMain(io, opts) || exe::stopOnSignals(io, SIGINT) || exe::stopAfter(io, std::chrono::seconds(15)));

        exe::submit(pool, [&]() { io.run(); });
        exe::submit(pool, [&]() { io.run(); });
        exe::submit(pool, [&]() { io.run(); });
        io.run();

        const auto now1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = now1 - now;
        std::cout << ms_double.count() << "ms\n";

    }
    catch (const std::exception& e)
    {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
