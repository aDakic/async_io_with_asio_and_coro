#pragma once

#include <spdlog/spdlog.h>

#include "Types.hpp"

namespace exe
{
    template<AwaitableType Awaitable>
    auto submit(ExecutionContext auto& ctx, Awaitable&& awaitable)
    {
        using returnType = typename Awaitable::value_type;

        if constexpr (std::is_void_v<returnType>)
        {
            asio::co_spawn(ctx, std::forward<Awaitable>(awaitable),
                           [](std::exception_ptr excPtr)
                           {
                               if (excPtr)
                               {
                                   spdlog::error("Exception thrown from coroutine!");
                                   std::rethrow_exception(excPtr);
                               }
                           });
        }
        else
        {
            asio::co_spawn(ctx, std::forward<Awaitable>(awaitable),
                           [](std::exception_ptr excPtr, auto)
                           {
                               if (excPtr)
                               {
                                   spdlog::error("Unhandled exception thrown from coroutine:");
                                   std::rethrow_exception(excPtr);
                               }
                           });
        }
    }

    template<typename Task, typename... Args>
    void submit(ExecutionContext auto& ctx, Task&& task, Args&&... args)
    {
        asio::post(ctx, std::bind_front(std::forward<Task>(task), std::forward<Args>(args)...));
    }

    template<AwaitableType... Awaitable>
    auto whenOneOf(Awaitable&&... awaitables)
    {
        using namespace asio::experimental::awaitable_operators;

        return (... || std::forward<Awaitable>(awaitables));
    }

    template<AwaitableType... Awaitable>
    auto whenAll(Awaitable&&... awaitables)
    {
        using namespace asio::experimental::awaitable_operators;

        return (... && std::forward<Awaitable>(awaitables));
    }

    asio::awaitable<void> stopAfter(ExecutionContext auto& ctx, std::chrono::seconds timeout)
    {
        asio::steady_timer t{ ctx, std::chrono::seconds{ timeout } };

        co_await t.async_wait(asio::use_awaitable);
    }

    asio::awaitable<void> stopOnSignals(ExecutionContext auto& ctx, auto... sigs)
    {
        asio::signal_set sigSet{ ctx, sigs... };

        int sig = co_await sigSet.async_wait(asio::use_awaitable);
        spdlog::critical("Signal {} received.", sig);

        co_return;
    }
}  // namespace exe