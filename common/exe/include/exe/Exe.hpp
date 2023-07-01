#pragma once

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>

namespace
{
  namespace asio = boost::asio;
  using namespace asio::experimental::awaitable_operators;
  using NoThrowAwaitable = asio::as_tuple_t<asio::use_awaitable_t<>>;
  using Signal           = NoThrowAwaitable::as_default_on_t<asio::signal_set>;
  using Timer            = NoThrowAwaitable::as_default_on_t<asio::steady_timer>;
}  // namespace

namespace exe
{
  template<typename T>
  static inline constexpr bool isAwaitable = false;

  template<typename T>
  static inline constexpr bool isAwaitable<asio::awaitable<T>> = true;

  template<typename T>
  concept AwaitableType = isAwaitable<T>;

  template<typename T>
  concept ExecutionContext = std::is_base_of_v<asio::execution_context, T>;

  template<typename T>
  concept Executor = asio::is_executor<T>::value || asio::execution::is_executor<T>::value;

  template<AwaitableType Awaitable>
  void submit(Executor auto&& executor, Awaitable&& awaitable)
  {
    using returnType = typename Awaitable::value_type;

    if constexpr (std::is_void_v<returnType>)
    {
      asio::co_spawn(executor, std::forward<Awaitable>(awaitable),
                     [](std::exception_ptr excPtr, auto...)
                     {
                       if (excPtr)
                       {
                         spdlog::error("Exception thrown from coroutine!");
                         std::rethrow_exception(excPtr);
                       }
                     });
    }
  }

  template<AwaitableType Awaitable>
  asio::awaitable<void> submit(Awaitable&& awaitable)
  {
    auto executor = co_await asio::this_coro::executor;

    co_await asio::co_spawn(executor, std::forward<Awaitable>(awaitable), asio::use_awaitable);
  }

  template<AwaitableType... Awaitable>
  void whenOneOf(exe::ExecutionContext auto& ctx, Awaitable&&... awaitables)
  {
    asio::co_spawn(ctx, (std::forward<Awaitable>(awaitables) || ...),
                   [](std::exception_ptr excPtr, auto)
                   {
                     if (excPtr)
                     {
                       spdlog::error("Unhandled exception thrown from coroutine:");
                       std::rethrow_exception(excPtr);
                     }
                   });
  }

  template<AwaitableType... Awaitable>
  asio::awaitable<void> whenAll(Awaitable&&... awaitables)
  {
    auto executor = co_await asio::this_coro::executor;

    co_await asio::co_spawn(executor, (std::forward<Awaitable>(awaitables) && ...), asio::use_awaitable);
  }

  asio::awaitable<void> stopAfter(std::int64_t timeout)
  {
    auto executor = co_await asio::this_coro::executor;

    Timer t{ executor, std::chrono::seconds{ timeout } };

    co_await t.async_wait(asio::use_awaitable);
  }

  asio::awaitable<void> stopOnSignals(auto... sigs)
  {
    auto executor = co_await asio::this_coro::executor;

    Signal sigSet{ executor, sigs... };

    auto [error, sig] = co_await sigSet.async_wait();
    if (!error)
    {
      spdlog::critical("Signal {} received.", sig);
    }

    co_return;
  }
}  // namespace exe