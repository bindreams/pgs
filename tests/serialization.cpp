#include <doctest/doctest.h>
#include <pgs.hpp>

#include <array>
#include <cstdint>
#include <span>

template<typename T, std::size_t N>
void test_resized(T expected_value, std::array<std::uint8_t, N> const& expected_bytes) {
	std::array<std::uint8_t, N> actual_bytes = {};
	serialize::dumps(std::span<std::uint8_t>{actual_bytes}, serialize::resized<N>(expected_value));

	CHECK(actual_bytes == expected_bytes);

	T actual_value = 0;
	serialize::loads(expected_bytes, serialize::resized<N>(actual_value));

	CHECK(actual_value == expected_value);
}

TEST_CASE("serialize.resized.downsize.unsigned") {
	test_resized<std::uint32_t, 3>(0x00'03'02'01, {0x03, 0x02, 0x01});
}

TEST_CASE("serialize.resized.downsize.signed") {
	test_resized<std::int32_t, 3>(0x00'03'02'01 | (1 << 31), {0x03 | (1 << 7), 0x02, 0x01});
}

TEST_CASE("serialize.resized.upsize.unsigned") {
	test_resized<std::uint32_t, 5>(0x04'03'02'01, {0x00, 0x04, 0x03, 0x02, 0x01});
}

TEST_CASE("serialize.resized.upsize.signed") {
	test_resized<std::int32_t, 5>(0x04'03'02'01 | (1 << 31), {0x00 | (1 << 7), 0x04, 0x03, 0x02, 0x01});
}
