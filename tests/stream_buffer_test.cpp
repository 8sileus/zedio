#define BOOST_TEST_MODULE tcp_buffer_test

#include "zed/net/detail/stream_buffer.hpp"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ring_buffer_test)

constexpr std::size_t N = 100;

BOOST_AUTO_TEST_CASE(simple_tcp_buffer_test) {
    using zed::net::detail::StreamBuffer;
    {
        StreamBuffer buffer;
        BOOST_CHECK_EQUAL(buffer.size(), zed::config::STREAM_BUFFER_DEFAULT_SIZE);
    }
    auto      size = 5;
    StreamBuffer buffer(size);
    BOOST_CHECK_EQUAL(buffer.size(), size);
}

BOOST_AUTO_TEST_SUITE_END()