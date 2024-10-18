#pragma once
#include <array>
#include <bit>
#include <concepts>
#include <ranges>
#include <span>
#include <type_traits>

#include "meta.hpp"

namespace pgs::serial {

template<size_t N, std::integral T>
struct Resized : public std::reference_wrapper<T> {
	static_assert(N > 0, "Resized: N must be greater than 0");
};

template<size_t N, std::integral T>
auto resized(T& value) {
	return Resized<N, T>(value);
}

template<size_t N, typename T>
struct meta<Resized<N, T>> {
	using type = std::remove_const_t<T>;
	static constexpr const size_t SizeBytes = N;

	static constexpr void loads(std::span<uint8_t const, N> bytes, Resized<N, T>& value, std::endian endianness) {
		auto& value_ = value.get();

		// Copy source data to a mutable buffer
		std::array<uint8_t, std::max(sizeof(T), N)> buffer = {};
		std::span<uint8_t, std::max(sizeof(T), N)> buffer_{buffer};

		// spans below first initialized for std::endian::big
		// actual N bytes
		auto to_copy = buffer_.template subspan<buffer.size() - N, N>();
		auto to_load = buffer_.template subspan<buffer.size() - sizeof(T), sizeof(T)>();
		auto to_discard = buffer_.template subspan<0, buffer.size() - sizeof(T)>();
		uint8_t* highest_byte = &to_copy[0];

		if (endianness == std::endian::little) {
			to_copy = buffer_.template subspan<0, N>();
			highest_byte = &to_copy[N - 1];

			to_load = buffer_.template subspan<0, sizeof(T)>();
			to_discard = buffer_.template subspan<sizeof(T), buffer.size() - sizeof(T)>();
		}
		std::ranges::copy(bytes, to_copy.data());

		// Remove and save sign bit from data (it's in the wrong place for auto loading)
		bool negative = false;
		if constexpr (std::signed_integral<T>) {
			// detect and reset sign bit to 0 (for loading as unsigned later)
			negative = *highest_byte & 0b10000000;
			*highest_byte &= 0b01111111;
		}

		// If there are bytes to discard (SizeBytes > sizeof(T)), check that no data will be lost
		if constexpr (SizeBytes > sizeof(T)) {
			for (auto byte : to_discard) {
				if (byte != 0) throw std::runtime_error("meta<Resized>::loads: non-zero data in discarded bytes");
			}
		}

		serial::loads(to_load, value_, endianness);
		if (negative) value_ |= std::numeric_limits<type>::min();  // add sign bit
	}

	static constexpr void dumps(std::span<uint8_t, N> bytes, Resized<N, T> value, std::endian endianness)
		requires std::unsigned_integral<T>
	{
		type value_ = value.get();

		if constexpr (N < sizeof(T)) {
			// downsizing
			constexpr const T max_allowed_value = [] {
				type result = 1 << (SizeBytes * 8 - 1);  //  = 0b1000...
				result += result - 1;                    // += 0b0111...

				return result;
			}();

			if (value_ > max_allowed_value) {
				throw std::runtime_error("meta<Resized>::dumps: value does not fit");
			}
		}

		std::array<uint8_t, std::max(sizeof(T), N)> buffer = {};

		// values below first initialized for std::endian::big
		// to write intermediate value
		std::span<uint8_t, sizeof(T)> to_write =
			std::span{buffer}.template subspan<buffer.size() - sizeof(T), sizeof(T)>();
		// to copy to output
		std::span<uint8_t, N> to_copy = std::span{buffer}.template subspan<buffer.size() - N, N>();

		if (endianness == std::endian::little) {
			to_write = std::span{buffer}.template subspan<0, sizeof(T)>();
			to_copy = std::span{buffer}.template subspan<0, N>();
		}

		serial::dumps(to_write, value_, endianness);
		std::ranges::copy(to_copy, bytes.data());
	}

	static constexpr void dumps(std::span<uint8_t, N> bytes, Resized<N, T> value, std::endian endianness)
		requires std::signed_integral<T>
	{
		using IntermediateType = uint<sizeof(T)>;
		auto intermediate = std::bit_cast<IntermediateType>(value.get() & ~std::numeric_limits<int>::min());
		// intermediate has all the same bits as value except the sign bit.

		if constexpr (N < sizeof(T)) {
			// downsizing
			constexpr const IntermediateType max_allowed_value = [] {
				IntermediateType result = 1 << (SizeBytes * 8 - 1);  // = 0b1000...
				result = result - 1;                                 // = 0b0111... (max, no sign bit)
				return result;
			}();

			if (intermediate > max_allowed_value) {
				throw std::runtime_error("meta<Resized>::dumps: value does not fit");
			}
		}

		serial::dumps(bytes, resized<N>(intermediate), endianness);
		if (value < 0) {
			if (endianness == std::endian::big) bytes[0] |= 0b10000000;
			else bytes[N - 1] |= 0b10000000;
		}
	}
};

}  // namespace pgs::serial
