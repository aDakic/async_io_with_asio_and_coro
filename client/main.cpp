#include <fmt/std.h>
#include <spdlog/spdlog.h>

#include <filesystem>

#include "dir/Monitor.hpp"
#include "exe/Exe.hpp"
#include "gst/Recorder.hpp"
#include "net/ClientEndpoint.hpp"
#include "po/ProgramOptions.hpp"

namespace asio = boost::asio;
namespace fs   = std::filesystem;

template<typename ExecutionContext>
asio::awaitable<void> uploadImages(ExecutionContext& context, const po::ClientOptions& opts)
{
    net::ClientEndpoint endpoint{ context, opts.timeout };
    dir::Monitor monitor{ context, opts.outDir };

    while (true)
    {
        auto imagePath = co_await monitor.getNewImage(asio::use_awaitable);
        spdlog::info("New image found: {}", imagePath);

        co_await endpoint.sendFile(opts.serverIp, std::to_string(opts.serverPort), imagePath);
    }

    co_return;
}

template<typename ExecutionContext>
asio::awaitable<void> takeCameraShots(ExecutionContext& context, const po::ClientOptions& opts)
{
    spdlog::info("takeCameraShots");
    gst::CameraShot camera{ context, opts.videoDevice, opts.outDir };

    while (true)
    {
        gst::PipelineMessage msg = co_await camera.take(asio::use_awaitable);

        if (msg != gst::PipelineMessage::EoS)
            co_return;
    }
}

template<typename ExecutionContext>
asio::awaitable<void> takeImages(ExecutionContext& context, const po::ClientOptions& opts)
{
    spdlog::info("takeImages");
    co_await exe::whenOne(takeCameraShots(context, opts), exe::stopAfter(context, std::chrono::seconds(15)));
}

int main(int argc, char* argv[])
{
    try
    {
        po::ClientOptions options;
        if (!po::parse(argc, argv, options))
        {
            return EXIT_FAILURE;
        }

        gst_init(nullptr, nullptr);

        asio::system_context anyThread;
        asio::io_context io;

        exe::submit(io, takeImages(io, options));
        exe::submit(io, uploadImages(io, options));

        io.run();
    }
    catch (const std::exception& e)
    {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }
}