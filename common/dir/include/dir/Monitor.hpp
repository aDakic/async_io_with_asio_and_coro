#pragma once

#include <sys/inotify.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <filesystem>

#include "exe/Exe.hpp"

namespace asio = boost::asio;
namespace fs   = std::filesystem;

namespace dir
{
    template<exe::ExecutionContextType Context>
    struct Monitor
    {
        Monitor(Context& context, const fs::path& listenDir)
            : _context{ context },
              _listenDir{ listenDir },
              _streamDesc{ _context, inotify_init() },
              _buf{ 1024 },
              _wd{ inotify_add_watch(_streamDesc.native_handle(), _listenDir.c_str(), IN_MOVED_TO) }
        {
        }

        template<typename CompletionToken>
        asio::awaitable<fs::path> getNewImage(CompletionToken&& token)
        {
            return asio::async_initiate<CompletionToken, void(fs::path)>(
                [this](auto handler) { exe::submit(_context, read(std::move(handler))); }, token);
        }

    private:
        template<typename CompletionHandler>
        asio::awaitable<void> read(CompletionHandler handler)
        {
            std::size_t transferred =
                co_await _streamDesc.async_read_some(_buf.prepare(_buf.max_size()), asio::use_awaitable);

            size_t processed = 0;

            while (transferred - processed >= sizeof(inotify_event))
            {
                const char* cdata           = processed + boost::asio::buffer_cast<const char*>(_buf.data());
                const inotify_event* ievent = reinterpret_cast<const inotify_event*>(cdata);
                processed += sizeof(inotify_event) + ievent->len;

                fs::path path = _listenDir;

                if (ievent->len)
                {
                    path /= ievent->name;
                }

                spdlog::info("New file detected {}", path);
                handler(std::move(path));
            }
        }
        Context& _context;
        fs::path _listenDir;
        asio::posix::stream_descriptor _streamDesc;
        asio::streambuf _buf;
        int _wd;
    };

}  // namespace dir