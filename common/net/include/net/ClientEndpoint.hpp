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
  namespace asio         = boost::asio;
  namespace beast        = boost::beast;
  namespace http         = boost::beast::http;
  namespace fs           = std::filesystem;
  using tcp              = asio::ip::tcp;
  using NoThrowAwaitable = asio::as_tuple_t<asio::use_awaitable_t<>>;
  using Resolver         = asio::use_awaitable_t<>::as_default_on_t<tcp::resolver>;
  using TcpStream        = asio::use_awaitable_t<>::as_default_on_t<beast::tcp_stream>;
}  // namespace

namespace net
{

  struct ClientEndpoint
  {
    ClientEndpoint(const exe::Executor auto& executor, std::string host, std::string port, std::int64_t timeout)
        : _resolver{ executor }, _stream{ executor }, _host{ std::move(host) }, _port{ std::move(port) }, _timeout{ timeout }
    {
    }

    asio::awaitable<void> sendFile(fs::path imagePath)
    {
      try
      {
        auto const results = co_await _resolver.async_resolve(_host, _port);
        _stream.expires_after(_timeout);
        co_await _stream.async_connect(results);

        _stream.expires_after(_timeout);
        auto req = prepareRequest(imagePath);
        co_await http::async_write(_stream, req);

        beast::flat_buffer resBuffer;
        http::response<http::dynamic_body> res;

        co_await http::async_read(_stream, resBuffer, res);

        std::cout << res << std::endl;

        _stream.socket().shutdown(tcp::socket::shutdown_both);
      }
      catch (boost::system::system_error& se)
      {
        if (se.code() != boost::system::errc::operation_canceled)
          throw;
      }

      co_return;
    }

  private:
    http::request<http::file_body> prepareRequest(const fs::path& imagePath)
    {
      boost::beast::error_code ec;
      http::file_body::value_type body;
      body.open(imagePath.c_str(), boost::beast::file_mode::read, ec);

      if (ec == beast::errc::no_such_file_or_directory)
        throw std::runtime_error("Can't open image file");

      http::request<http::file_body> req;

      req.method(http::verb::post);
      req.target("/screenshot");
      req.version(11);
      req.set(http::field::host, _host);
      req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
      req.set(http::field::content_type, "image/jpeg");
      req.body() = std::move(body);
      req.prepare_payload();

      return req;
    }

    Resolver _resolver;
    TcpStream _stream;
    std::string _host;
    std::string _port;
    std::chrono::seconds _timeout;
  };
}  // namespace net