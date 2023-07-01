#pragma once

#include <fmt/std.h>
#include <sys/inotify.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <filesystem>

#include "exe/Exe.hpp"

namespace
{
  namespace asio = boost::asio;
  namespace fs   = std::filesystem;
  using FileDesc = asio::use_awaitable_t<>::as_default_on_t<asio::posix::stream_descriptor>;
}  // namespace

namespace dir
{

  struct Monitor
  {
    Monitor(const exe::Executor auto& executor, const fs::path& listenDir, int mask)
        : _listenDir{ listenDir },
          _streamDesc{ executor, inotify_init() },
          _buf{ 1024 },
          _mask{ mask },
          _wd{ inotify_add_watch(_streamDesc.native_handle(), _listenDir.c_str(), _mask) }
    {
      if (!fs::exists(_listenDir))
      {
        throw std::runtime_error("Directory doesn't exist");
      }
    }

    asio::awaitable<fs::path> getNewImage1()
    {
      fs::path retPath = "";

      try
      {
        auto transferred = co_await _streamDesc.async_read_some(_buf.prepare(_buf.max_size()));
        size_t processed = 0;

        if (transferred - processed >= sizeof(inotify_event))
        {
          const char* cdata           = processed + boost::asio::buffer_cast<const char*>(_buf.data());
          const inotify_event* ievent = reinterpret_cast<const inotify_event*>(cdata);
          processed += sizeof(inotify_event) + ievent->len;

          retPath = _listenDir;

          if (ievent->len)
          {
            retPath /= ievent->name;
          }
        }
      }
      catch (boost::system::system_error& se)
      {
        if (se.code() != boost::system::errc::operation_canceled)
          throw;
      }

      co_return retPath;
    }

    void cancel() { _streamDesc.cancel(); }

  private:
    fs::path _listenDir;
    FileDesc _streamDesc;
    asio::streambuf _buf;
    int _mask;
    int _wd;
  };

}  // namespace dir