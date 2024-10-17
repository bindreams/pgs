#include <doctest/doctest.h>
#include <pgs.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

using namespace pgs;

static_assert(serial::TupleLike<std::tuple<int>>);
static_assert(not serial::TupleLike<int>);
static_assert(serial::ConstantSizeTupleLike<std::tuple<int>>);
static_assert(serial::ConstantSizeTupleLike<std::tuple<int&>>);

template<typename T, size_t N>
void test_resized(T expected_value, std::array<uint8_t, N> const& expected_bytes) {
	std::array<uint8_t, N> actual_bytes = {};
	serial::dumps(std::span<uint8_t, N>{actual_bytes}, serial::resized<N>(expected_value));

	CHECK(actual_bytes == expected_bytes);

	T actual_value = 0;
	serial::loads(expected_bytes, serial::resized<N>(actual_value));

	CHECK(actual_value == expected_value);
}

TEST_CASE("serialize.resized.downsize.unsigned") {
	test_resized<uint32_t, 3>(0x00'03'02'01, {0x03, 0x02, 0x01});
}

TEST_CASE("serialize.resized.downsize.signed") {
	test_resized<int32_t, 3>(0x00'03'02'01 | (1 << 31), {0x03 | (1 << 7), 0x02, 0x01});
}

TEST_CASE("serialize.resized.upsize.unsigned") {
	test_resized<uint32_t, 5>(0x04'03'02'01, {0x00, 0x04, 0x03, 0x02, 0x01});
}

TEST_CASE("serialize.resized.upsize.signed") {
	test_resized<int32_t, 5>(0x04'03'02'01 | (1 << 31), {0x00 | (1 << 7), 0x04, 0x03, 0x02, 0x01});
}
