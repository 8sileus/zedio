#define BOOST_TEST_MODULE address_test

#include "net/address.hpp"

#include <boost/test/included/unit_test.hpp>
#include <iostream>

using namespace zed;

BOOST_AUTO_TEST_SUITE(address_test)

BOOST_AUTO_TEST_CASE(interface_test) {
    Address4 addr4("192.168.15.33", 9999);
    BOOST_TEST(addr4.isIpv4());
    BOOST_TEST(!addr4.isIpv6());
    BOOST_REQUIRE_EQUAL(addr4.port(), 9999);
    BOOST_REQUIRE_EQUAL(addr4.ip(), "192.168.15.33");
    std::pair<std::string, std::uint16_t> p{"192.168.15.33", 9999};
    BOOST_REQUIRE_EQUAL(addr4.ipPort().first, p.first);
    BOOST_REQUIRE_EQUAL(addr4.ipPort().second, p.second);
    BOOST_REQUIRE_EQUAL(addr4.toString(), "192.168.15.33:9999");
    BOOST_REQUIRE_EQUAL(addr4.len(), sizeof(sockaddr_in));

    Address4 tmp(addr4.addr(), addr4.len());
    BOOST_TEST(tmp.isIpv4());
    BOOST_TEST(!tmp.isIpv6());
    BOOST_REQUIRE_EQUAL(tmp.port(), 9999);
    BOOST_REQUIRE_EQUAL(tmp.ip(), "192.168.15.33");
    BOOST_REQUIRE_EQUAL(tmp.ipPort().first, p.first);
    BOOST_REQUIRE_EQUAL(tmp.ipPort().second, p.second);
    BOOST_REQUIRE_EQUAL(tmp.toString(), "192.168.15.33:9999");
    BOOST_REQUIRE_EQUAL(tmp.len(), sizeof(sockaddr_in));

    Address4 addr("localhost", "ftp");
    BOOST_TEST(addr.isIpv4());
    BOOST_TEST(!addr.isIpv6());
    BOOST_REQUIRE_EQUAL(addr.port(), 21);
    BOOST_REQUIRE_EQUAL(addr.ip(), "127.0.0.1");
}

BOOST_AUTO_TEST_SUITE_END()
