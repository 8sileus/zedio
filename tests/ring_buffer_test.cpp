#define BOOST_TEST_MODULE ring_buffer_test

#include "zedio/common/util/ring_buffer.hpp"

#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ring_buffer_test)

constexpr std::size_t N = 100;

BOOST_AUTO_TEST_CASE(heap_ring_buffer_simple_test) {
    zedio::util::HeapRingBuffer<std::size_t> rb(N);
    BOOST_CHECK_EQUAL(rb.capacity(), N);
    for (std::size_t i = 1; i <= N; ++i) {
        rb.push(i);
        BOOST_CHECK_EQUAL(rb.size(), i);
    }
    BOOST_CHECK_EQUAL(rb.is_fill(), true);
    for (std::size_t i = 1; i <= N / 2; ++i) {
        BOOST_CHECK_EQUAL(rb.pop().value(), i);
        BOOST_CHECK_EQUAL(rb.size(), N - i);
    }
    for (std::size_t i = 1; i <= N / 2; ++i) {
        rb.push(i);
        BOOST_CHECK_EQUAL(rb.size(), N / 2 + i);
    }
    std::size_t i = N / 2 + 1;
    while (!rb.is_empty()) {
        BOOST_CHECK_EQUAL(rb.pop().value(), i++);
        if (i == rb.capacity() + 1) {
            i = 1;
        }
    }
    BOOST_CHECK_EQUAL(rb.is_empty(), true);
}

BOOST_AUTO_TEST_CASE(flex_ring_buffer_resize_test) {
    constexpr int                    N = 5;
    zedio::util::HeapRingBuffer<int> rb(N);
    BOOST_CHECK_EQUAL(rb.capacity(), N);
    for (std::size_t i = 1; i <= N; ++i) {
        rb.push(i);
        BOOST_CHECK_EQUAL(rb.size(), i);
    }
    BOOST_CHECK_EQUAL(rb.is_fill(), true);
    BOOST_CHECK_EQUAL(rb.resize(2 * N), true);
    BOOST_CHECK_EQUAL(rb.capacity(), 2 * N);
    for (std::size_t i = N + 1; i <= N * 2; ++i) {
        rb.push(i);
        BOOST_CHECK_EQUAL(rb.size(), i);
    }
    BOOST_CHECK_EQUAL(rb.is_fill(), true);
    std::size_t i = 1;
    while (!rb.is_empty()) {
        BOOST_CHECK_EQUAL(rb.pop().value(), i++);
    }
    BOOST_CHECK_EQUAL(rb.is_empty(), true);
    for (std::size_t i = 1; i <= 2 * N; ++i) {
        rb.push(i);
        BOOST_CHECK_EQUAL(rb.size(), i);
    }
    BOOST_CHECK_EQUAL(rb.resize(N), false);
    BOOST_CHECK_EQUAL(rb.is_fill(), true);
    for (std::size_t i = 1; i <= N; ++i) {
        BOOST_CHECK_EQUAL(rb.pop().value(), i);
        rb.push(i);
    }
    BOOST_CHECK_EQUAL(rb.is_fill(), true);
    i = N + 1;
    while (!rb.is_empty()) {
        BOOST_CHECK_EQUAL(rb.pop().value(), i++);
        if (i == rb.capacity() + 1) {
            i = 1;
        }
    }
    BOOST_CHECK_EQUAL(rb.is_empty(), true);
    rb.push(1);
    rb.push(2);
    BOOST_CHECK_EQUAL(rb.resize(N), true);
    BOOST_CHECK_EQUAL(rb.capacity(), 5);
    BOOST_CHECK_EQUAL(rb.size(), 2);
    BOOST_CHECK_EQUAL(rb.pop().value(), 1);
    BOOST_CHECK_EQUAL(rb.pop().value(), 2);
    BOOST_CHECK_EQUAL(rb.is_empty(), true);
}

BOOST_AUTO_TEST_CASE(stack_ring_buffer_test) {
    zedio::util::StackRingBuffer<std::size_t, N> rb;
    BOOST_CHECK_EQUAL(rb.capacity(), N);
    for (std::size_t i = 1; i <= N; ++i) {
        rb.push(i);
        BOOST_CHECK_EQUAL(rb.size(), i);
    }
    BOOST_CHECK_EQUAL(rb.is_fill(), true);
    for (std::size_t i = 1; i <= N / 2; ++i) {
        BOOST_CHECK_EQUAL(rb.take(), i);
        BOOST_CHECK_EQUAL(rb.size(), N - i);
    }
    for (std::size_t i = 1; i <= N / 2; ++i) {
        rb.push(i);
        BOOST_CHECK_EQUAL(rb.size(), N / 2 + i);
    }
    std::size_t i = N / 2 + 1;
    while (!rb.is_empty()) {
        BOOST_CHECK_EQUAL(rb.take(), i++);
        if (i == rb.capacity() + 1) {
            i = 1;
        }
    }
    BOOST_CHECK_EQUAL(rb.is_empty(), true);
}

BOOST_AUTO_TEST_SUITE_END()
