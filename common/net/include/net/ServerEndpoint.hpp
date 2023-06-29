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
    namespace fs    = std::filesystem;
    using tcp       = boost::asio::ip::tcp;
}  // namespace

namespace net
{
    using Acceptor  = asio::use_awaitable_t<>::as_default_on_t<asio::ip::tcp::acceptor>;
    using TcpStream = asio::use_awaitable_t<>::as_default_on_t<beast::tcp_stream>;

    template<exe::ExecutionContext Context>
    struct ServerEndpoint
    {
        ServerEndpoint(Context& context, const fs::path& storageDir, std::uint16_t port, std::int64_t timeout)
            : _context{ context },
              _storageDir{ storageDir },
              _acceptor{ _context, { tcp::v4(), port } },
              _timeout{ timeout },
              _sessionId{ 0ul }
        {
        }

        asio::awaitable<void> doListen()
        {
            try
            {
                while (_acceptor.is_open())
                {
                    auto clientSocket = co_await _acceptor.async_accept();

                    TcpStream stream{ std::move(clientSocket) };

                    exe::submit(_context, doSession(std::move(stream)));
                }
            }
            catch (boost::system::system_error& se)
            {
                if (se.code() != boost::system::errc::operation_canceled)
                    throw;
            }

            co_return;
        }

        void cancel() { _acceptor.cancel(); }

    private:
        asio::awaitable<void> doSession(TcpStream stream)
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
                if (se.code() != http::error::end_of_stream && se.code() != boost::system::errc::operation_canceled)
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

            auto imageName = fmt::format("recimage{}.jpg", _sessionId++);

            std::ofstream file{ _storageDir / imageName, std::ios::binary };
            if (!file.is_open())
            {
                spdlog::error(fmt::format("Can't open image: {}", imageName));
                throw std::runtime_error("Can't open image");
            }

            file << req.body();

            spdlog::info("New image received!");

            http::response<http::empty_body> res{ http::status::ok, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.keep_alive(req.keep_alive());

            return res;
        }

        Context& _context;
        fs::path _storageDir;
        Acceptor _acceptor;
        std::chrono::seconds _timeout;
        std::size_t _sessionId;
    };
}  // namespace net