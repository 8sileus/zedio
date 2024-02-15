#define BOOST_TEST_MODULE address_test

#include "zedio/net/addr.hpp"

#include <boost/test/included/unit_test.hpp>

using namespace zedio::net;

BOOST_AUTO_TEST_SUITE(address_test)

BOOST_AUTO_TEST_CASE(ipv4_test) {
    {
        auto ip = Ipv4Addr::parse("192.168.15.33").value();
        auto addr = SocketAddr{ip, 9999};
        BOOST_TEST(addr.is_ipv4());
        BOOST_TEST(!addr.is_ipv6());
        BOOST_REQUIRE_EQUAL(addr.port(), 9999);
        BOOST_REQUIRE_EQUAL(addr.length(), sizeof(sockaddr_in));
        BOOST_REQUIRE_EQUAL(addr.to_string(), "192.168.15.33:9999");
        BOOST_REQUIRE_EQUAL(std::get<0>(addr.ip()).to_string(), "192.168.15.33");

        SocketAddr tmp(addr.sockaddr(), addr.length());
        BOOST_TEST(tmp.is_ipv4());
        BOOST_TEST(!tmp.is_ipv6());
        BOOST_REQUIRE_EQUAL(tmp.port(), 9999);
        BOOST_REQUIRE_EQUAL(tmp.to_string(), "192.168.15.33:9999");
        BOOST_REQUIRE_EQUAL(tmp.length(), sizeof(sockaddr_in));
        BOOST_REQUIRE_EQUAL(std::get<0>(addr.ip()).to_string(), ip.to_string());
    }
    {
        auto ip = Ipv4Addr(192, 168, 15, 33);
        auto addr = SocketAddr{ip, 9999};
        BOOST_TEST(addr.is_ipv4());
        BOOST_TEST(!addr.is_ipv6());
        BOOST_REQUIRE_EQUAL(addr.port(), 9999);
        BOOST_REQUIRE_EQUAL(addr.length(), sizeof(sockaddr_in));
        BOOST_REQUIRE_EQUAL(addr.to_string(), "192.168.15.33:9999");
        BOOST_REQUIRE_EQUAL(std::get<0>(addr.ip()).to_string(), "192.168.15.33");
    }

    {
        auto addr = SocketAddr::parse("localhost", 21).value();
        BOOST_TEST(addr.is_ipv4());
        BOOST_TEST(!addr.is_ipv6());
        BOOST_REQUIRE_EQUAL(addr.port(), 21);
        BOOST_REQUIRE_EQUAL(std::get<0>(addr.ip()).to_string(), "127.0.0.1");
    }
}

BOOST_AUTO_TEST_CASE(ipv6_test) {
    std::string str = "96CC:F03C:ED86:D0FC:C2BB:8696:7EA5:3D79";
    {
        std::transform(str.begin(), str.end(), str.begin(), tolower);
        auto ip = Ipv6Addr::parse(str).value();

        auto addr = SocketAddr{ip, 1234};
        BOOST_TEST(!addr.is_ipv4());
        BOOST_TEST(addr.is_ipv6());
        BOOST_REQUIRE_EQUAL(addr.port(), 1234);
        BOOST_REQUIRE_EQUAL(std::get<1>(addr.ip()).to_string(), ip.to_string());
        BOOST_REQUIRE_EQUAL(addr.to_string(), std::string("[") + str + "]:1234");
        BOOST_REQUIRE_EQUAL(addr.length(), sizeof(sockaddr_in6));

        SocketAddr tmp(addr.sockaddr(), addr.length());
        BOOST_TEST(!tmp.is_ipv4());
        BOOST_TEST(tmp.is_ipv6());
        BOOST_REQUIRE_EQUAL(tmp.port(), 1234);
        BOOST_REQUIRE_EQUAL(std::get<1>(addr.ip()).to_string(), ip.to_string());
        BOOST_REQUIRE_EQUAL(tmp.to_string(), std::string("[") + str + "]:1234");
        BOOST_REQUIRE_EQUAL(tmp.length(), sizeof(sockaddr_in6));
    }
    {
        auto ip = Ipv6Addr(0x96cc, 0xf03c, 0xed86, 0xd0fc, 0xc2bb, 0x8696, 0x7ea5, 0x3d79);
        auto addr = SocketAddr{ip, 1234};
        BOOST_TEST(!addr.is_ipv4());
        BOOST_TEST(addr.is_ipv6());
        BOOST_REQUIRE_EQUAL(addr.port(), 1234);
        BOOST_REQUIRE_EQUAL(std::get<1>(addr.ip()).to_string(), ip.to_string());
        BOOST_REQUIRE_EQUAL(addr.to_string(), std::string("[") + str + "]:1234");
        BOOST_REQUIRE_EQUAL(addr.length(), sizeof(sockaddr_in6));
    }
}

BOOST_AUTO_TEST_SUITE_END()
