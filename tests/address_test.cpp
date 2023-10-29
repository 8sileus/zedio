#define BOOST_TEST_MODULE address_test

#include "net/address.hpp"

#include <boost/test/included/unit_test.hpp>

using namespace zed::net;

BOOST_AUTO_TEST_SUITE(address_test)

BOOST_AUTO_TEST_CASE(ipv4_test) {
    {
        Address addr("192.168.15.33", 9999);
        BOOST_TEST(addr.is_ipv4());
        BOOST_TEST(!addr.is_ipv6());
        BOOST_REQUIRE_EQUAL(addr.get_port(), 9999);
        BOOST_REQUIRE_EQUAL(addr.get_ip(), "192.168.15.33");
        BOOST_REQUIRE_EQUAL(addr.to_string(), "192.168.15.33 9999");
        BOOST_REQUIRE_EQUAL(addr.get_length(), sizeof(sockaddr_in));

        Address tmp(addr.get_sockaddr(), addr.get_length());
        BOOST_TEST(tmp.is_ipv4());
        BOOST_TEST(!tmp.is_ipv6());
        BOOST_REQUIRE_EQUAL(tmp.get_port(), 9999);
        BOOST_REQUIRE_EQUAL(tmp.get_ip(), "192.168.15.33");
        BOOST_REQUIRE_EQUAL(tmp.to_string(), "192.168.15.33 9999");
        BOOST_REQUIRE_EQUAL(tmp.get_length(), sizeof(sockaddr_in));
    }

    {
        Address addr("localhost", "ftp");
        BOOST_TEST(addr.is_ipv4());
        BOOST_TEST(!addr.is_ipv6());
        BOOST_REQUIRE_EQUAL(addr.get_port(), 21);
        BOOST_REQUIRE_EQUAL(addr.get_ip(), "127.0.0.1");
    }
}

BOOST_AUTO_TEST_CASE(ipv6_test) {
    std::string ip = "96CC:F03C:ED86:D0FC:C2BB:8696:7EA5:3D79";
    std::transform(ip.begin(), ip.end(), ip.begin(), tolower);

    Address addr(ip, 1234);
    BOOST_TEST(!addr.is_ipv4());
    BOOST_TEST(addr.is_ipv6());
    BOOST_REQUIRE_EQUAL(addr.get_port(), 1234);
    BOOST_REQUIRE_EQUAL(addr.get_ip(), ip);
    BOOST_REQUIRE_EQUAL(addr.to_string(), ip + " 1234");
    BOOST_REQUIRE_EQUAL(addr.get_length(), sizeof(sockaddr_in6));

    Address tmp(addr.get_sockaddr(), addr.get_length());
    BOOST_TEST(!tmp.is_ipv4());
    BOOST_TEST(tmp.is_ipv6());
    BOOST_REQUIRE_EQUAL(tmp.get_port(), 1234);
    BOOST_REQUIRE_EQUAL(tmp.get_ip(), ip);
    BOOST_REQUIRE_EQUAL(tmp.to_string(), ip + " 1234");
    BOOST_REQUIRE_EQUAL(tmp.get_length(), sizeof(sockaddr_in6));
}

BOOST_AUTO_TEST_SUITE_END()
