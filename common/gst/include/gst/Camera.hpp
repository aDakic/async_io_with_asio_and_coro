#pragma once

#include <fmt/core.h>
#include <fmt/std.h>

#include <boost/asio.hpp>
#include <filesystem>
#include <string_view>

#include "Pipeline.hpp"
#include "exe/Exe.hpp"

namespace
{
  namespace asio = boost::asio;
  namespace fs   = std::filesystem;
  using FileDesc = asio::use_awaitable_t<>::as_default_on_t<asio::posix::stream_descriptor>;
}  // namespace

namespace gst
{
  enum class PipelineMessage
  {
    Idle,
    EoS,
    Error
  };

  struct Camera
  {
    Camera(const exe::Executor auto& executor, const fs::path &cameraDevice, const fs::path &path)
        : _pipeline{ fmt::format(_config, cameraDevice.c_str(), path.c_str()) },
          _streamDesc{ executor, _pipeline.getPollFd() }
    {
      if (!_pipeline)
      {
        throw std::runtime_error("Failed to create pipeline");
      }
    }

    asio::awaitable<PipelineMessage> take1()
    {
      PipelineMessage result = PipelineMessage::Idle;
      _pipeline.play();

      try
      {
        while (true)
        {
          co_await _streamDesc.async_wait(asio::posix::stream_descriptor::wait_read);

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

            _pipeline.stop();
            gst_message_unref(message);
            break;
          }
        }
      }
      catch (boost::system::system_error &se)
      {
        if (se.code() != boost::system::errc::operation_canceled)
          throw;
      }

      co_return result;
    }

    void cancel() { _streamDesc.cancel(); }
    ~Camera() noexcept { _pipeline.stop(); }

  private:
    fs::path _path;
    Pipeline _pipeline;
    FileDesc _streamDesc;
    static constexpr const char *_config =
        "v4l2src device={} num-buffers=1 ! jpegenc !  multifilesink location={}/image\%d.jpg";
  };
}  // namespace gst