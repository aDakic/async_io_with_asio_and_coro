#pragma once

#include <sys/inotify.h>
#include <filesystem>

#include <boost/asio.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <fmt/std.h>

#include "exe/Exe.hpp"

namespace
{
    namespace asio = boost::asio;
    namespace fs   = std::filesystem;
}  // namespace

namespace dir
{
    using FileDesc = asio::use_awaitable_t<>::as_default_on_t<asio::posix::stream_descriptor>;

    template<exe::ExecutionContext Context>
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

        void cancel() { _streamDesc.cancel(); }

    private:
        template<typename CompletionHandler>
        asio::awaitable<void> read(CompletionHandler handler)
        {
            try
            {
                std::size_t transferred = co_await _streamDesc.async_read_some(_buf.prepare(_buf.max_size()));

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
                    exe::submit(_context, std::move(handler), std::move(path));
                    co_return;
                }
            }
            catch (const std::exception& e)
            {
                spdlog::info("except");
                exe::submit(_context, std::move(handler), "");
            }
        }

        Context& _context;
        fs::path _listenDir;
        FileDesc _streamDesc;
        asio::streambuf _buf;
        int _wd;
    };

}  // namespace dir