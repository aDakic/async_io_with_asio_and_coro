#pragma once

#include <boost/asio.hpp>

namespace asio = boost::asio;

namespace exe
{
    using ServiceBase = asio::execution_context::service;

    struct StopService : ServiceBase
    {
        using key_type = StopService;

        static asio::execution_context::id id;

        using ServiceBase::ServiceBase;
        StopService(asio::execution_context &context, std::stop_source stop)
            : ServiceBase{context}, _stopSource(std::move(stop)) {}

        std::stop_source get() const { return _stopSource; }
        bool isStopped() const { return _stopSource.stop_requested(); }
        void requestStop() { _stopSource.request_stop(); }

    private:
        void shutdown() noexcept override {}
        std::stop_source _stopSource;
    };
}