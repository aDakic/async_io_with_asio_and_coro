#pragma once

#include <boost/program_options.hpp>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs       = std::filesystem;
namespace boost_po = boost::program_options;
using namespace std::chrono_literals;

namespace po
{
    struct CommonOptions
    {
        std::string serverIp;
        std::uint16_t serverPort;
        fs::path outDir;
        std::int64_t timeout;

        void addOptions(boost_po::options_description& description)
        {
            // clang-format off
            description.add_options()
            ("ip", boost_po::value<std::string>(&serverIp)->default_value("localhost"), "Server IP address")
            ("port,p", boost_po::value<std::uint16_t>(&serverPort)->required(), "Server port")
            ("outdir", boost_po::value<fs::path>(&outDir)->required(), "Output directory")
            ("timeout", boost_po::value<std::int64_t>(&timeout)->default_value(30), "Connection timeout");
            // clang-format on
        }
    };

    struct ClientOptions : CommonOptions
    {
        fs::path videoDevice;
        std::int64_t recTime;

        void addOptions(boost_po::options_description& description)
        {
            CommonOptions::addOptions(description);
            // clang-format off
            description.add_options()
            ("videodevice", boost_po::value<fs::path>(&videoDevice)->default_value("/dev/video0"), "Video Device")
            ("rectime", boost_po::value<std::int64_t>(&recTime)->default_value(10), "Recording time");
            // clang-format on
        }
    };

    struct ServerOptions : CommonOptions
    {
        void addOptions(boost_po::options_description& description) { CommonOptions::addOptions(description); }
    };

    inline bool
    parse(int argc, char* argv[], auto& opts)
    {
        boost_po::options_description description("Usage");

        try
        {
            description.add_options()("help,h", "Display this help message");
            opts.addOptions(description);

            boost_po::variables_map vm;
            boost_po::store(boost_po::command_line_parser(argc, argv).options(description).run(), vm);

            if (vm.count("help") == 1)
            {
                std::cerr << description;
                return false;
            }

            boost_po::notify(vm);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << "\n\n";
            std::cerr << description;

            return false;
        }

        return true;
    }
}  // namespace po