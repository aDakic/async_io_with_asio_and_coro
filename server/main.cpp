#include "exe/Exe.hpp"
#include "net/ServerEndpoint.hpp"
#include "po/ProgramOptions.hpp"

namespace asio = boost::asio;

template<typename ExecutionContext>
asio::awaitable<void> asyncMain(ExecutionContext& context, po::ServerOptions& opts)
{
    net::ServerEndpoint server{ context, opts.outDir, opts.timeout };

    auto const address = asio::ip::make_address(opts.serverIp);
    auto const port    = opts.serverPort;

    spdlog::info("Starting async Main...");
    spdlog::info("Listening on {}:{}", "127.0.0.1", port);

    co_await exe::submit(context, exe::whenOne(server.doListen({ address, port }), exe::stopOnSignals(context, SIGTERM)));

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

        asio::io_context io;

        exe::submit(io, asyncMain(io, options));

        io.run();
    }
    catch (const std::exception& e)
    {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }
}
