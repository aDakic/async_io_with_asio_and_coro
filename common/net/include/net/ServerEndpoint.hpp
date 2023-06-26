#pragma once

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <filesystem>
#include <fstream>

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
    using tcp_stream = typename beast::tcp_stream::rebind_executor<
        asio::use_awaitable_t<>::executor_with_default<asio::any_io_executor>>::other;

    template<exe::ExecutionContextType Context>
    struct ServerEndpoint
    {
        ServerEndpoint(Context& context, const std::filesystem::path& storageDir, std::int64_t timeout)
            : _context{ context }, _storageDir{ storageDir }, _timeout{ timeout }
        {
        }

        asio::awaitable<void> doListen(tcp::endpoint endpoint)
        {
            tcp::acceptor acceptor{ _context, endpoint };

            while (acceptor.is_open())
            {
                tcp::socket client = co_await acceptor.async_accept(asio::use_awaitable);
                tcp_stream stream{ std::move(client) };

                spdlog::info("The new client accepted");
                exe::submit(_context, doSession(std::move(stream)));
            }
            
            co_return;
        }

    private:
        asio::awaitable<void> doSession(tcp_stream stream)
        {
            beast::flat_buffer buffer;

            try
            {
                while (true)
                {
                    stream.expires_after(_timeout);

                    http::request<http::string_body> req;
                    co_await http::async_read(stream, buffer, req);

                    http::message_generator msg = handleRequest(std::move(req));

                    bool keepAlive = msg.keep_alive();

                    co_await beast::async_write(stream, std::move(msg), asio::use_awaitable);

                    if (!keepAlive)
                    {
                        spdlog::critical("Connection closed!");
                        break;
                    }

                    stream.socket().shutdown(tcp::socket::shutdown_send);
                    co_return;
                }
            }
            catch (boost::system::system_error& se)
            {
                if (se.code() != http::error::end_of_stream)
                    throw;
            }
        }

        template<class Body, class Allocator>
        http::message_generator handleRequest(http::request<Body, http::basic_fields<Allocator>>&& req)
        {
            // Returns a bad request response
            auto const bad_request = [&req](beast::string_view why)
            {
                http::response<http::string_body> res{ http::status::bad_request, req.version() };
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = std::string(why);
                res.prepare_payload();
                return res;
            };

            // Returns a server error response
            auto const server_error = [&req](beast::string_view what)
            {
                http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, "text/html");
                res.keep_alive(req.keep_alive());
                res.body() = "An error occurred: '" + std::string(what) + "'";
                res.prepare_payload();
                return res;
            };

            if (req.method() != http::verb::post)
                return bad_request("Unknown HTTP-method");

            if (req.target() != "/screenshot")
                return bad_request("Illegal request-target");

            if (req.body().size() == 0)
            {
                return bad_request("Invalid image");
            }

            std::ofstream file{ _storageDir / "receivedImage.jpg", std::ios::binary };
            if (!file.is_open())
            {
                throw;
            }

            file << req.body();

            spdlog::info("New image received!");

            http::response<http::empty_body> res{ http::status::ok, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.keep_alive(req.keep_alive());

            return res;
        }
        
        Context& _context;
        std::filesystem::path _storageDir;
        std::chrono::seconds _timeout;
    };
}  // namespace net