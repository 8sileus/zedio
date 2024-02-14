#define BOOST_TEST_MODULE socket_test

#include "zedio/net/tcp_socket.hpp"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ring_buffer_test)

using namespace zedio::net;

#define X_HAS_VAL(F)                                     \
    if (auto ret = F; !ret) {                            \
        LOG_ERROR("#F fail: {}", ret.error().message()); \
    } else {                                             \
        LOG_TRACE("#F succ: {}", ret.value());           \
    }

#define X_NO_VAL(F)                                      \
    if (auto ret = F; !ret) {                            \
        LOG_ERROR("#F fail: {}", ret.error().message()); \
    } else {                                             \
        LOG_TRACE("#F succ");                            \
    }

BOOST_AUTO_TEST_CASE(tcp_socket_test) {
    auto sock = TcpSocket::v4().value();
    auto addr = SocketAddr::parse("localhost", 9898).value();
    BOOST_CHECK(sock.bind(addr));
    try {
        // test nonblocking
        BOOST_CHECK(sock.nonblocking().value() == true);
        BOOST_CHECK(sock.set_nonblocking(false));
        BOOST_CHECK(sock.nonblocking().value() == false);
        BOOST_CHECK(sock.set_nonblocking(true));
        BOOST_CHECK(sock.nonblocking().value() == true);

    } catch (const std::exception &ex) {
        LOG_ERROR("{}", ex.what());
    }
    // test resuseaddr
    X_HAS_VAL(sock.reuseaddr())
    X_NO_VAL(sock.set_reuseaddr(true))
    X_HAS_VAL(sock.reuseaddr())
    X_NO_VAL(sock.set_reuseaddr(false))
    X_HAS_VAL(sock.reuseaddr())
    // test resuseport
    try {
        BOOST_CHECK(sock.reuseport().value() == false);
        BOOST_CHECK(sock.set_reuseport(true));
        BOOST_CHECK(sock.reuseport().value() == true);
        BOOST_CHECK(sock.set_reuseport(false));
        BOOST_CHECK(sock.reuseport().value() == false);
    } catch (const std::exception &ex) {
        LOG_ERROR("{}", ex.what());
    }
    // test linger
    try {
        BOOST_CHECK(sock.linger().value() == std::nullopt);
        BOOST_CHECK(sock.set_linger(1s));
        BOOST_CHECK(sock.linger().value().value() == 1s);
        BOOST_CHECK(sock.set_linger(std::nullopt));
        BOOST_CHECK(sock.linger().value() == std::nullopt);
    } catch (const std::exception &ex) {
        LOG_ERROR("{}", ex.what());
    }
    // test nodelay
    try {
        LOG_TRACE("nodelay : {}", sock.nodelay().value());
        BOOST_CHECK(sock.set_nodelay(true));
        BOOST_CHECK(sock.nodelay().value() == true);
        BOOST_CHECK(sock.set_nodelay(false));
        BOOST_CHECK(sock.nodelay().value() == false);
    } catch (const std::exception &ex) {
        LOG_ERROR("{}", ex.what());
    }
}

BOOST_AUTO_TEST_SUITE_END()