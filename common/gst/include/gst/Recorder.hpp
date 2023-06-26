#pragma once

#include <fmt/core.h>
#include <fmt/std.h>

#include <boost/asio.hpp>
#include <filesystem>
#include <string_view>

#include "Pipeline.hpp"
#include "exe/Exe.hpp"

namespace asio = boost::asio;

namespace gst
{
    enum class PipelineMessage
    {
        Idle,
        EoS,
        Error
    };

    template<exe::ExecutionContextType Context>
    struct CameraShot
    {
        CameraShot(Context &context,  const std::filesystem::path &cameraDevice, const std::filesystem::path &path)
            : _context{ context },
              _pipeline{ fmt::format(_config, cameraDevice.c_str(), path.c_str()) },
              _streamDesc{ _context, _pipeline.getPollFd() }
        {
            if (!_pipeline)
            {
                throw;
            }
        }

        template<typename CompleteToken>
        auto take(CompleteToken &&token)
        {
            /* Start playing */
            return asio::async_initiate<CompleteToken, void(PipelineMessage)>(
                [this](auto handler)
                {
                    _pipeline.play();
                    exe::submit(_context, handleMessages(std::move(handler)));
                },
                token);
        }

        ~CameraShot() noexcept { _pipeline.stop(); }

    private:
        template<typename CompleteHandler>
        asio::awaitable<void> handleMessages(CompleteHandler handler)
        {
            PipelineMessage result = PipelineMessage::Idle;

            while (true)
            {
                co_await _streamDesc.async_wait(asio::posix::stream_descriptor::wait_read, asio::use_awaitable);
                GstMessage *message = _pipeline.getMessage();

                if (message)
                {
                    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
                    {
                        GError *err;
                        char *debugInfo;
                        gst_message_parse_error(message, &err, &debugInfo);

                        spdlog::error("Error received from element {}: {}", GST_OBJECT_NAME(message->src), err->message);
                        spdlog::error("{}", debugInfo ? debugInfo : "none");

                        g_clear_error(&err);
                        g_free(debugInfo);

                        result = PipelineMessage::Error;
                    }
                    else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS)
                    {
                        result = PipelineMessage::EoS;
                    }
                    else
                    {
                        spdlog::critical("Unsupported type of message");
                    }

                    exe::submit(_context, [handler = std::move(handler), result]() mutable { handler(result); });
                    _pipeline.stop();
                    gst_message_unref(message);  // FIXME: RAII
                    co_return;
                }
            }
        }

        Context &_context;
        std::filesystem::path _path;
        Pipeline _pipeline;
        asio::posix::stream_descriptor _streamDesc;
        static constexpr const char *_config = "v4l2src device={} num-buffers=1 ! jpegenc !  multifilesink location={}/image\%d.jpg";
    };
}  // namespace gst