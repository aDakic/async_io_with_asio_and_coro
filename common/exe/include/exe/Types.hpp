#pragma once

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core.hpp>

namespace exe
{
    namespace asio  = boost::asio;
    namespace beast = boost::beast;

    using tcp = asio::ip::tcp;
    using namespace asio::experimental::awaitable_operators;

    template<typename T>
    static inline constexpr bool isAwaitable = false;

    template<typename T>
    static inline constexpr bool isAwaitable<asio::awaitable<T>> = true;

    template<typename T>
    concept AwaitableType = isAwaitable<T>;

    template<typename T>
    concept ExecutionContext = std::is_base_of_v<asio::execution_context, T>;
}  // namespace exe