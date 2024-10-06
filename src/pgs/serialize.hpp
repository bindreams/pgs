#pragma once
#include <array>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstdio>
#include <iostream>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace serialize {

// The following block taken from https://github.com/bindreams/zint-bindings/blob/main/src/zint/src/utility.hpp ========

/**
 * @brief Returns true if `T` is an array bounded in all dimensions, such as x[10][20][30].
 *
 * Returns false for non-arrays, arrays with an unbounded dimension, and invalid arrays such as the following:
 * ```c++
 * int x[][10] = {};
 * ```
 *
 * The array above has the highest dimension deduced from initialization, but the initializer list is empty and the size
 * is deduced to 0, which is not allowed per C++ rules.
 */
template<typename T>
inline constexpr bool is_fully_bounded() {
	if constexpr (std::rank_v<T> == 0) return false;  // Not a valid array
	// For example, this condition is true for arrays like int x[][10] = {}, where first extent is deduced to 0.

	if constexpr (!std::is_bounded_array_v<T>) return false;
	if constexpr (std::rank_v<T> > 1) return is_fully_bounded<std::remove_extent_t<T>>();
	return true;
}

/// Concept that encapsulates the behavior of `is_fully_bounded`.
template<typename T>
concept BoundedNDArray = is_fully_bounded<T>();

template<BoundedNDArray T, std::size_t I = std::rank_v<T> - 1>
inline constexpr std::array<int, std::rank_v<T>> array_shape_impl() {
	if constexpr (I != 0) {
		auto result = array_shape_impl<T, I - 1>();
		result[I] = std::extent_v<T, I>;
		return result;
	} else {
		std::array<int, std::rank_v<T>> result{};
		result[I] = std::extent_v<T, I>;
		return result;
	}
}

/// An std::array containing the extents of a bounded N-dimensional array in order.
template<BoundedNDArray T>
inline constexpr std::array<int, std::rank_v<T>> array_shape = array_shape_impl<T>();
// =====================================================================================================================

// clang-format off
template<size_t Size>
using uint = std::conditional_t<
	Size == 1, uint8_t,
	std::conditional_t<
		Size == 2, uint16_t,
		std::conditional_t<
			Size == 4, uint32_t,
			std::conditional_t<
				Size == 8, uint64_t,
				void
			>
		>
	>
>;
// clang-format on

// Byte read/write =====================================================================================================
template<typename T>
concept ByteLike = sizeof(T) == 1 and (std::integral<T> or std::integral<std::underlying_type_t<T>>);

template<ByteLike B1, ByteLike B2>
	requires(not std::is_const_v<B2>)
constexpr void read_bytes(std::basic_istream<B1>& stream, std::span<B2> output) {
	stream.read(reinterpret_cast<B1*>(output.data()), output.size());
	size_t read_count = stream.gcount();
	if (read_count < output.size()) {
		if (stream.eof())
			throw std::runtime_error(
				"read_bytes: read failed: EOF reached after " + std::to_string(read_count) + " bytes"
			);
		if (stream.fail())
			throw std::runtime_error(
				"read_bytes: read failed: failbit set after " + std::to_string(read_count) + " bytes"
			);
		if (stream.bad())
			throw std::runtime_error(
				"read_bytes: read failed: badbit set after " + std::to_string(read_count) + " bytes"
			);
	}
}

template<size_t N, ByteLike B>
constexpr std::array<uint8_t, N> read_bytes(std::basic_istream<B>& stream) {
	std::array<uint8_t, N> result;
	read_bytes(stream, std::span<uint8_t>{result});
	return result;
}

template<ByteLike B>
constexpr std::vector<uint8_t> read_bytes(std::basic_istream<B>& stream, size_t count) {
	std::vector<uint8_t> result(count);
	read_bytes(stream, std::span<uint8_t>{result});
	return result;
}

template<size_t N>
std::array<uint8_t, N> read_bytes(FILE* stream) {
	std::array<uint8_t, N> result;

	size_t read_count = fread(result.data(), result.size(), 1, stream);
	if (read_count < N) throw std::runtime_error("read failed");

	return result;
}

std::vector<uint8_t> read_bytes(FILE* stream, size_t count) {
	std::vector<uint8_t> result(count);

	size_t read_count = fread(result.data(), result.size(), 1, stream);
	if (read_count < count) throw std::runtime_error("read failed");

	return result;
}

template<ByteLike B>
void write_bytes(std::basic_ostream<B>& stream, std::span<uint8_t const> bytes) {
	stream.write(reinterpret_cast<B const*>(bytes.data()), bytes.size());
	if (!stream.good()) throw std::runtime_error("write failed");
}

void write_bytes(FILE* stream, std::span<uint8_t const> bytes) {
	size_t count = std::fwrite(bytes.data(), bytes.size(), 1, stream);
	if (count != 1) throw std::runtime_error("write failed");
}

template<typename T>
concept InputStream = requires(T stream) {
	{ read_bytes(stream, 1) };
};

template<typename T>
concept OutputStream = requires(T stream, std::span<uint8_t const> bytes) {
	{ write_bytes(stream, bytes) };
};

// Object serialize framework ==========================================================================================
template<typename T>
struct meta;

template<typename T>
concept HasMeta = requires(T value) {
	{ meta<std::remove_cvref_t<T>>{} };
};

template<HasMeta T>
struct meta<T&> : public meta<T> {};

template<HasMeta T>
struct meta<T const> : public meta<T> {};

template<HasMeta T>
struct meta<T volatile> : public meta<T> {};

template<typename T>
concept HasProxyMeta = HasMeta<T> and requires(T ref) {
	{ meta<T>::proxy(ref) };
};

template<HasProxyMeta T>
using ProxyType = std::remove_cvref_t<decltype(meta<T>::proxy(std::declval<T&>()))>;

template<HasMeta T>
consteval bool _has_proxy_or_size_bytes() {
	if constexpr (HasProxyMeta<T>) {
		return _has_proxy_or_size_bytes<ProxyType<T>>();
	} else {
		return requires {
			{ meta<T>::SizeBytes };
		};
	}
}

template<typename T>
concept ConstantSizeMeta = HasMeta<T> and _has_proxy_or_size_bytes<T>();

template<ConstantSizeMeta T>
constexpr size_t SizeBytes = [] consteval {
	if constexpr (HasProxyMeta<T>) return SizeBytes<ProxyType<T>>;
	else return meta<T>::SizeBytes;
}();

template<HasMeta T, InputStream S>
constexpr void load(S& stream, T&& value, std::endian endianness = std::endian::big) {
	if constexpr (HasProxyMeta<T>) {
		load(stream, meta<T>::proxy(value), endianness);
	} else if constexpr (ConstantSizeMeta<T>) {
		auto bytes = read_bytes<meta<T>::SizeBytes>(stream);
		meta<T>::loads(std::span{bytes}, value, endianness);
	} else {
		meta<T>::load(stream, value, endianness);
	}
}

template<HasMeta T, InputStream S>
constexpr T load(S& stream, std::endian endianness = std::endian::big) {
	T value;
	load(stream, value, endianness);
	return value;
}

template<HasMeta T, OutputStream S>
constexpr void dump(S& stream, T const& value, std::endian endianness = std::endian::big) {
	if constexpr (HasProxyMeta<T>) {
		dump(stream, meta<T>::proxy(value), endianness);
	} else if constexpr (ConstantSizeMeta<T>) {
		std::array<uint8_t, meta<T>::SizeBytes> bytes;
		meta<T>::dumps(std::span{bytes}, value, endianness);
		write_bytes(stream, std::span{bytes});
	} else {
		meta<T>::dump(stream, value, endianness);
	}
}

template<ConstantSizeMeta T, ByteLike B>
constexpr void loads(std::span<B> bytes, T&& value, std::endian endianness = std::endian::big) {
	if constexpr (HasProxyMeta<T>) {
		loads(bytes, meta<T>::proxy(std::forward<T>(value)), endianness);
	} else {
		if (bytes.size() < meta<T>::SizeBytes) throw std::runtime_error("loads: buffer is too small");
		meta<T>::loads(bytes.subspan(0, meta<T>::SizeBytes), value, endianness);
	}
}

template<ConstantSizeMeta T, ByteLike B>
constexpr T loads(std::span<B> bytes, std::endian endianness = std::endian::big) {
	T value;
	loads(bytes, value, endianness);
	return value;
}

template<HasMeta T, ByteLike B>
constexpr void dumps(std::span<B> bytes, T const& value, std::endian endianness = std::endian::big) {
	if constexpr (HasProxyMeta<T>) {
		dumps(bytes, meta<T>::proxy(value), endianness);
	} else {
		if (bytes.size() < meta<T>::SizeBytes) throw std::runtime_error("dumps: buffer is too small");
		meta<T>::dumps(bytes.subspan(0, meta<T>::SizeBytes), value, endianness);
	}
}

// Built-in meta<T> specializations ====================================================================================
template<std::integral T>
struct meta<T> {
	static constexpr const size_t SizeBytes = sizeof(T);
	static_assert(SizeBytes == 1 or SizeBytes == 2 or SizeBytes == 4 or SizeBytes == 8, "unsupported integral size");

	using UnsignedType = std::make_unsigned_t<T>;

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		assert(bytes.size() == SizeBytes);

		std::array<uint8_t, SizeBytes> bytes_ = {};
		std::ranges::copy(bytes, bytes_.data());
		value = std::bit_cast<T>(bytes_);

		if (endianness != std::endian::native) value = std::byteswap(value);
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T value, std::endian endianness) {
		assert(bytes.size() == SizeBytes);

		if (endianness != std::endian::native) value = std::byteswap(value);
		auto bytes_ = std::bit_cast<std::array<uint8_t, SizeBytes>>(value);
		std::ranges::copy(bytes_, bytes.data());
	}

	// 	static constexpr void loads(T& value, std::span<uint8_t const> bytes, std::endian endianness) {
	// 		assert(bytes.size() == SizeBytes);

	// 		if (std::endian::native == endianness) {
	// 			auto data = std::bit_cast<std::array<uint8_t, SizeBytes>>(value);
	// 			std::ranges::copy(data, bytes.data());
	// 		} else {
	// 			if constexpr (SizeBytes == 1) {
	// 				value = std::bit_cast<T>(bytes[0]);
	// 			} else {
	// 				auto intermediate = std::bit_cast<UnsignedType>(value);

	// #ifdef _MSC_VER
	// 				if constexpr (SizeBytes == 2) {
	// 					value = std::bit_cast<T>(_byteswap_ushort(intermediate));
	// 				} else if constexpr (SizeBytes == 4) {
	// 					value = std::bit_cast<T>(_byteswap_ulong(intermediate));
	// 				} else if constexpr (SizeBytes == 8) {
	// 					value = std::bit_cast<T>(_byteswap_uint64(intermediate));
	// 				} else {
	// 					static_assert(false);
	// 				}
	// #else
	// 				if constexpr (SizeBytes == 2) {
	// 					value = std::bit_cast<T>(__builtin_bswap16(intermediate));
	// 				} else if constexpr (SizeBytes == 4) {
	// 					value = std::bit_cast<T>(__builtin_bswap32(intermediate));
	// 				} else if constexpr (SizeBytes == 8) {
	// 					value = std::bit_cast<T>(__builtin_bswap64(intermediate));
	// 				} else {
	// 					static_assert(false);
	// 				}
	// #endif
	// 			}
	// 		}
	// 	}

	// 	static constexpr void dumps(T value, std::span<uint8_t> bytes, std::endian endianness) {
	// 		assert(bytes.size() == SizeBytes);

	// 		if (std::endian::native == endianness) {
	// 			auto data = std::bit_cast<std::array<uint8_t, SizeBytes>>(value);
	// 			std::ranges::copy(data, bytes.data());
	// 		} else {
	// 			if constexpr (SizeBytes == 1) {
	// 				bytes[0] = value;
	// 			} else {
	// 				UnsignedType data = 0;

	// #ifdef _MSC_VER
	// 				if constexpr (SizeBytes == 2) {
	// 					data = _byteswap_ushort(std::bit_cast<UnsignedType>(value));
	// 				} else if constexpr (SizeBytes == 4) {
	// 					data = _byteswap_ulong(std::bit_cast<UnsignedType>(value));
	// 				} else if constexpr (SizeBytes == 8) {
	// 					data = _byteswap_uint64(std::bit_cast<UnsignedType>(value));
	// 				} else {
	// 					static_assert(false);
	// 				}
	// #else
	// 				if constexpr (SizeBytes == 2) {
	// 					data = __builtin_bswap16(std::bit_cast<UnsignedType>(value));
	// 				} else if constexpr (SizeBytes == 4) {
	// 					data = __builtin_bswap32(std::bit_cast<UnsignedType>(value));
	// 				} else if constexpr (SizeBytes == 8) {
	// 					data = __builtin_bswap64(std::bit_cast<UnsignedType>(value));
	// 				} else {
	// 					static_assert(false);
	// 				}

	// #endif

	// 				memcpy(bytes.data(), &data, SizeBytes);
	// 			}
	// 		}
	// 	}
};

template<>
struct meta<bool> {
	static constexpr const size_t SizeBytes = 1;

	static constexpr void loads(std::span<uint8_t const> bytes, bool& value, std::endian endianness) {
		uint8_t temp;
		serialize::loads(bytes, temp, endianness);
		value = temp;
	}

	static constexpr void dumps(std::span<uint8_t> bytes, bool value, std::endian endianness) {
		serialize::dumps(bytes, value, endianness);
	}
};

template<std::floating_point T>
struct meta<T> {
	static constexpr const size_t SizeBytes = sizeof(T);
	using IntermediateType = uint<SizeBytes>;

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		IntermediateType temp = 0;
		serialize::loads(bytes, temp, endianness);

		value = std::bit_cast<T>(temp);
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T value, std::endian endianness) {
		IntermediateType temp = std::bit_cast<T>(value);
		serialize::dumps(bytes, temp, endianness);
	}
};

template<typename T>
	requires(std::is_enum_v<T>)
struct meta<T> {
	static constexpr const size_t SizeBytes = sizeof(T);
	using UnderlyingType = std::underlying_type_t<T>;

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		UnderlyingType temp = 0;
		serialize::loads(bytes, temp, endianness);
		value = static_cast<T>(temp);
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T value, std::endian endianness) {
		serialize::dumps(bytes, static_cast<UnderlyingType>(value), endianness);
	}
};

template<BoundedNDArray T>
struct meta<T> {
	using ValueType = std::remove_extent_t<T>;

	template<InputStream S>
	static constexpr void load(S& stream, T& value, std::endian endianness) {
		for (auto&& item : value) serialize::load(stream, item, endianness);
	}

	template<OutputStream S>
	static constexpr void dump(S& stream, T const& value, std::endian endianness) {
		for (auto&& item : value) serialize::dump(stream, item, endianness);
	}
};

template<BoundedNDArray T>
	requires ConstantSizeMeta<std::remove_extent_t<T>>
struct meta<T> {
	using ValueType = std::remove_extent_t<T>;
	static constexpr const size_t SizeBytes = meta<ValueType>::SizeBytes * std::extent_v<T>;

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		assert(bytes.size() == SizeBytes);

		for (size_t i = 0; i < std::extent_v<T>; ++i) {
			serialize::loads(
				bytes.subspan(meta<ValueType>::SizeBytes * i, meta<ValueType>::SizeBytes), value[i], endianness
			);
		}
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T const& value, std::endian endianness) {
		assert(bytes.size() == SizeBytes);

		for (size_t i = 0; i < std::extent_v<T>; ++i) {
			serialize::dumps(
				bytes.subspan(meta<ValueType>::SizeBytes * i, meta<ValueType>::SizeBytes), value[i], endianness
			);
		}
	}
};

template<typename T>
concept TupleLike = requires(T value) {
	{ std::tuple_size<T>{} };
	typename std::tuple_element<0, T>;
	{ std::get<0>(value) };
};

static_assert(TupleLike<std::tuple<int>>);
static_assert(not TupleLike<int>);

template<size_t I = 0, typename T, typename F>
	requires TupleLike<std::remove_cvref_t<T>>
void _tuple_for_each_impl(F&& callable, T&& tuple) {
	if constexpr (I < std::tuple_size_v<std::remove_cvref_t<T>>) {
		callable(std::get<I>(std::forward<T>(tuple)));
		_tuple_for_each_impl<I + 1>(std::forward<F>(callable), std::forward<T>(tuple));
	}
}

template<typename T, typename F>
	requires TupleLike<std::remove_cvref_t<T>>
void tuple_for_each(F&& callable, T&& tuple) {
	_tuple_for_each_impl(std::forward<F>(callable), std::forward<T>(tuple));
}

template<TupleLike T, std::size_t I = 0>
consteval bool each_element_has_constant_size_meta() {
	if constexpr (I == std::tuple_size_v<T>) {
		return true;
	} else {
		return ConstantSizeMeta<std::tuple_element_t<I, T>> and each_element_has_constant_size_meta<T, I + 1>();
	}
}

template<typename T>
concept ConstantSizeTupleLike = TupleLike<T> and each_element_has_constant_size_meta<T>();

static_assert(ConstantSizeTupleLike<std::tuple<int>>);
// static_assert(ConstantSizeMeta<std::chrono::time_point<
// 				  std::chrono::steady_clock,
// 				  std::chrono::duration<unsigned int, std::ratio<1, 90000>>>&>);
// static_assert(ConstantSizeTupleLike<std::tuple<
// 				  int,
// 				  std::chrono::time_point<
// 					  std::chrono::steady_clock,
// 					  std::chrono::duration<unsigned int, std::ratio<1, 90000>>>&>>);
static_assert(ConstantSizeTupleLike<std::tuple<int&>>);

template<ConstantSizeTupleLike T, size_t I = 0>
static constexpr size_t _tuple_size_bytes_impl() {
	if constexpr (I == std::tuple_size_v<T>) {
		return 0;
	} else {
		static_assert(ConstantSizeMeta<std::tuple_element_t<I, T>>);
		return SizeBytes<std::tuple_element_t<I, T>> + _tuple_size_bytes_impl<T, I + 1>();
	}
}

template<TupleLike T>
struct meta<T> {
	template<InputStream S>
	static constexpr void load(S& stream, T& value, std::endian endianness) {
		tuple_for_each(
			[&](auto&& item) {
				static_assert(HasMeta<decltype(item)>, "tuple item does not have a meta<> specialization");
				serialize::load(stream, item, endianness);
			},
			value
		);
	}

	template<OutputStream S>
	static constexpr void dump(S& stream, T const& value, std::endian endianness) {
		tuple_for_each(
			[&](auto&& item) {
				static_assert(HasMeta<decltype(item)>, "tuple item does not have a meta<> specialization");
				serialize::dump(stream, item, endianness);
			},
			value
		);
	}
};

template<ConstantSizeTupleLike T>
struct meta<T> {
	static constexpr const size_t SizeBytes = _tuple_size_bytes_impl<T>();

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		assert(bytes.size() == SizeBytes);

		size_t offset = 0;
		tuple_for_each(
			[&](auto&& item) {
				serialize::loads(bytes.subspan(offset, serialize::SizeBytes<decltype(item)>), item, endianness);
				offset += serialize::SizeBytes<decltype(item)>;
			},
			value
		);
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T const& value, std::endian endianness) {
		assert(bytes.size() == SizeBytes);

		size_t offset = 0;
		tuple_for_each(
			[&](auto&& item) {
				serialize::dumps(bytes.subspan(offset, serialize::SizeBytes<decltype(item)>), item, endianness);
				offset += serialize::SizeBytes<decltype(item)>;
			},
			value
		);
	}
};

// Wrapper for resizing integers =======================================================================================
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

	static constexpr void loads(std::span<uint8_t const> bytes, Resized<N, T>& value, std::endian endianness) {
		assert(bytes.size() == SizeBytes);
		auto& value_ = value.get();

		// Copy source data to a mutable buffer
		std::array<uint8_t, std::max(sizeof(T), N)> buffer = {};
		std::span<uint8_t> actual_data;
		std::span<uint8_t> to_load;
		std::span<uint8_t> to_discard;
		uint8_t* highest_byte = nullptr;

		if (endianness == std::endian::big) {
			actual_data = std::span{buffer}.subspan(buffer.size() - N, N);
			highest_byte = &actual_data[0];

			to_load = std::span{buffer}.subspan(buffer.size() - sizeof(T), sizeof(T));
			to_discard = std::span{buffer}.subspan(0, buffer.size() - sizeof(T));
		} else {
			actual_data = std::span{buffer}.subspan(0, N);
			highest_byte = &actual_data[N - 1];

			to_load = std::span{buffer}.subspan(0, sizeof(T));
			to_discard = std::span{buffer}.subspan(sizeof(T), buffer.size() - sizeof(T));
		}
		std::ranges::copy(bytes, actual_data.data());

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

		serialize::loads(to_load, value_, endianness);
		if (negative) value_ |= std::numeric_limits<type>::min();  // add sign bit
	}

	static constexpr void dumps(std::span<uint8_t> bytes, Resized<N, T> value, std::endian endianness)
		requires std::unsigned_integral<T>
	{
		assert(bytes.size() == SizeBytes);
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
		std::span<uint8_t> to_write;  // of size sizeof(T) to write intermediate value
		std::span<uint8_t> to_copy;   // of size N to copy to output

		if (endianness == std::endian::big) {
			to_write = std::span{buffer}.subspan(buffer.size() - sizeof(T), sizeof(T));
			to_copy = std::span{buffer}.subspan(buffer.size() - N, N);
		} else {
			to_write = std::span{buffer}.subspan(0, sizeof(T));
			to_copy = std::span{buffer}.subspan(0, N);
		}

		serialize::dumps(to_write, value_, endianness);
		std::ranges::copy(to_copy, bytes.data());
	}

	static constexpr void dumps(std::span<uint8_t> bytes, Resized<N, T> value, std::endian endianness)
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

		serialize::dumps(bytes, resized<N>(intermediate), endianness);
		if (value < 0) {
			if (endianness == std::endian::big) bytes[0] |= 0b10000000;
			else bytes[N - 1] |= 0b10000000;
		}
	}
};

template<typename Rep, typename Period>
struct meta<std::chrono::duration<Rep, Period>> {
	using T = std::chrono::duration<Rep, Period>;
	static constexpr const size_t SizeBytes = sizeof(Rep);

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		auto count = serialize::loads<Rep>(bytes, endianness);
		value = T{count};
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T value, std::endian endianness) {
		serialize::dumps<Rep>(bytes, value.count(), endianness);
	}
};

template<typename Clock, typename Duration>
struct meta<std::chrono::time_point<Clock, Duration>> {
	using T = std::chrono::time_point<Clock, Duration>;
	static constexpr const size_t SizeBytes = serialize::SizeBytes<Duration>;

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		auto time_since_epoch = serialize::loads<Duration>(bytes, endianness);
		value = T{time_since_epoch};
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T value, std::endian endianness) {
		serialize::dumps<Duration>(bytes, value.time_since_epoch(), endianness);
	}
};

}  // namespace serialize
