#pragma once

#include "zedio/common/def.hpp"

// C++
#include <concepts>
#include <coroutine>
#include <span>

#ifdef __linux__
#include <sys/socket.h>
#elif _WIN32
#include <WinSock2.h>
#include <windows.h>
#endif

namespace zedio {

template <typename Addr>
concept is_socket_address = requires(Addr addr) {
    { addr.sockaddr() } noexcept -> std::same_as<struct sockaddr *>;
    { addr.length() } noexcept -> std::same_as<SocketLength>;
};

template <class IOAwaiter>
concept is_awaiter = requires(IOAwaiter awaiter) {
    { awaiter.await_ready() };
    { awaiter.await_resume() };
    { awaiter.await_suspend(std::noop_coroutine()) };
};

template <typename C>
concept constructible_to_char_slice = requires(C c) { std::span<const char>{c}; };

} // namespace zedio
