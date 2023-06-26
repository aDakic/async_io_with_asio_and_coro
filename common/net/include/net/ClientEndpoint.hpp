#pragma once

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "exe/Exe.hpp"

namespace
{
    namespace beast = boost::beast;
    namespace http  = boost::beast::http;
    namespace asio  = boost::asio;
    using tcp       = boost::asio::ip::tcp;

}  // namespace

namespace net
{
    template<exe::ExecutionContextType Context>
    struct ClientEndpoint
    {
        ClientEndpoint(Context& context, std::int64_t timeout) : _context{ context }, _timeout{timeout} { }

        asio::awaitable<void> sendFile(const std::string& host, const std::string& port,
                                       const std::filesystem::path& imagePath)
        {
            auto resolver = asio::use_awaitable.as_default_on(tcp::resolver(co_await asio::this_coro::executor));
            auto stream   = asio::use_awaitable.as_default_on(beast::tcp_stream(co_await asio::this_coro::executor));

            auto const results = co_await resolver.async_resolve(host, port);

            stream.expires_after(_timeout);
            co_await stream.async_connect(results);
            spdlog::info("Connected to the server");

            stream.expires_after(_timeout);
            auto req = prepareRequest(host, imagePath);
            co_await http::async_write(stream, req);
            spdlog::info("Request sent");

            beast::flat_buffer resBuffer;
            http::response<http::dynamic_body> res;

            co_await http::async_read(stream, resBuffer, res);

            spdlog::info("Response received:");
            std::cout << res << std::endl;

            stream.socket().shutdown(tcp::socket::shutdown_both);

            co_return;
        }

    private:
        http::request<http::file_body> prepareRequest(const std::string& host, const std::filesystem::path& imagePath)
        {
            boost::beast::error_code ec;
            http::file_body::value_type body;
            body.open(imagePath.c_str(), boost::beast::file_mode::read, ec);

            if (ec == beast::errc::no_such_file_or_directory)
                throw;

            http::request<http::file_body> req;

            req.method(http::verb::post);
            req.target("/screenshot");
            req.version(11);
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
            req.set(http::field::content_type, "image/jpeg");
            req.body() = std::move(body);
            req.prepare_payload();

            return req;
        }

        Context& _context;
        std::chrono::seconds _timeout;
    };
}  // namespace net