#pragma once

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

namespace
{
    namespace asio = boost::asio;
}

using namespace asio::experimental::awaitable_operators;

namespace exe
{
    template<typename T>
    static inline constexpr bool isAwaitable = false;

    template<typename T>
    static inline constexpr bool isAwaitable<asio::awaitable<T>> = true;

    template<typename T>
    concept AwaitableType = isAwaitable<T>;

    template<typename T>
    concept ExecutionContextType = std::is_base_of_v<asio::execution_context, T>;

    template<AwaitableType Awaitable>
    auto submit(ExecutionContextType auto& ctx, Awaitable&& awaitable)
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
            return asio::co_spawn(ctx, std::forward<Awaitable>(awaitable), asio::use_awaitable);
        }
    }

    template<typename Task, typename... Args>
    void submit(ExecutionContextType auto& ctx, Task&& task, Args&&... args)
    {
        asio::post(ctx, std::bind_front(std::forward<Task>(task), std::forward<Args>(args)...));
    }

    template<AwaitableType... Awaitable>
    auto whenOne(Awaitable&&... awaitables)
    {
        using namespace asio::experimental::awaitable_operators;

        return (... || std::forward<Awaitable>(awaitables));
    }

    asio::awaitable<void> stopAfter(ExecutionContextType auto& ctx, std::chrono::seconds timeout)
    {
        spdlog::info("stopAfter");
        asio::steady_timer t{ ctx, timeout };
        co_await t.async_wait(asio::use_awaitable);
    }

    asio::awaitable<void> stopOnSignals(ExecutionContextType auto& ctx, auto... sigs)
    {
        asio::signal_set sigSet{ ctx, sigs... };

        int sig = co_await sigSet.async_wait(asio::use_awaitable);

        spdlog::critical("Signal {} received.", sig);
        co_return;
    }

}  // namespace exe