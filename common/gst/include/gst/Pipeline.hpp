#pragma once

#include <gst/gst.h>

namespace gst
{
    struct Pipeline
    {
        explicit Pipeline(const std::string& config)
            : _pipeline{ gst_parse_launch(config.c_str(), nullptr) }, _bus{ gst_pipeline_get_bus(GST_PIPELINE(_pipeline)) }
        {
            if (!_pipeline || !_bus)
            {
                throw;
            }

            gst_bus_get_pollfd(_bus, &_pollFd);
        }

        ~Pipeline() noexcept
        {
            gst_object_unref(_pipeline);
            gst_object_unref(_bus);
        }

        int getPollFd() const noexcept { return _pollFd.fd; }
        
        GstMessage* getMessage() const noexcept
        {
            return gst_bus_pop_filtered(_bus, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        }
        void play() noexcept { gst_element_set_state(_pipeline, GST_STATE_PLAYING); }
        void stop() noexcept { gst_element_set_state(_pipeline, GST_STATE_NULL); }

        explicit operator bool() const noexcept { return _pipeline; }

    private:
        GstElement* _pipeline;
        GstBus* _bus;
        GPollFD _pollFd;
    };
}  // namespace gst