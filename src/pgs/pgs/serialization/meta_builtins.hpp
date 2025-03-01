#pragma once
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

#include "meta.hpp"
#include "traits.hpp"

namespace pgs::serial {

// Integral types ======================================================================================================
template<std::integral T>
struct meta<T> {
	static constexpr const size_t SizeBytes = sizeof(T);
	static_assert(SizeBytes == 1 or SizeBytes == 2 or SizeBytes == 4 or SizeBytes == 8, "unsupported integral size");

	using UnsignedType = std::make_unsigned_t<T>;

	static constexpr void loads(std::span<uint8_t const, SizeBytes> bytes, T& value, std::endian endianness) {
		std::array<uint8_t, SizeBytes> bytes_ = {};
		std::ranges::copy(bytes, bytes_.data());
		value = std::bit_cast<T>(bytes_);

		if (endianness != std::endian::native) value = std::byteswap(value);
	}

	static constexpr void dumps(std::span<uint8_t, SizeBytes> bytes, T value, std::endian endianness) {
		if (endianness != std::endian::native) value = std::byteswap(value);
		auto bytes_ = std::bit_cast<std::array<uint8_t, SizeBytes>>(value);
		std::ranges::copy(bytes_, bytes.data());
	}
};

// bool ================================================================================================================
template<>
struct meta<bool> {
	static constexpr const size_t SizeBytes = 1;

	static constexpr void loads(std::span<uint8_t const, SizeBytes> bytes, bool& value, std::endian endianness) {
		uint8_t temp;
		serial::loads(bytes, temp, endianness);
		value = temp;
	}

	static constexpr void dumps(std::span<uint8_t, SizeBytes> bytes, bool value, std::endian endianness) {
		serial::dumps(bytes, value, endianness);
	}
};

// Floating point types ================================================================================================
template<std::floating_point T>
struct meta<T> {
	static constexpr const size_t SizeBytes = sizeof(T);
	using IntermediateType = uint<SizeBytes>;

	static constexpr void loads(std::span<uint8_t const> bytes, T& value, std::endian endianness) {
		IntermediateType temp = 0;
		serial::loads(bytes, temp, endianness);

		value = std::bit_cast<T>(temp);
	}

	static constexpr void dumps(std::span<uint8_t> bytes, T value, std::endian endianness) {
		IntermediateType temp = std::bit_cast<T>(value);
		serial::dumps(bytes, temp, endianness);
	}
};

// Enums ===============================================================================================================
template<typename T>
	requires(std::is_enum_v<T>)
struct meta<T> {
	static constexpr const size_t SizeBytes = sizeof(T);
	using UnderlyingType = std::underlying_type_t<T>;

	static constexpr void loads(std::span<uint8_t const, SizeBytes> bytes, T& value, std::endian endianness) {
		UnderlyingType temp = 0;
		serial::loads(bytes, temp, endianness);
		value = static_cast<T>(temp);
	}

	static constexpr void dumps(std::span<uint8_t, SizeBytes> bytes, T value, std::endian endianness) {
		serial::dumps(bytes, static_cast<UnderlyingType>(value), endianness);
	}
};

// C-style arrays ======================================================================================================
template<BoundedNDArray T>
struct meta<T> {
	using ValueType = std::remove_extent_t<T>;

	template<InputStream S>
	static constexpr void load(S& stream, T& value, std::endian endianness) {
		for (auto&& item : value) serial::load(stream, item, endianness);
	}

	template<OutputStream S>
	static constexpr void dump(S& stream, T const& value, std::endian endianness) {
		for (auto&& item : value) serial::dump(stream, item, endianness);
	}
};

template<BoundedNDArray T>
	requires HasConstantSizeMeta<std::remove_extent_t<T>>
struct meta<T> {
	using ValueType = std::remove_extent_t<T>;
	static constexpr const size_t ItemSizeBytes = serial::SizeBytes<ValueType>;
	static constexpr const size_t SizeBytes = ItemSizeBytes * std::extent_v<T>;

	static constexpr void loads(std::span<uint8_t const, SizeBytes> bytes, T& value, std::endian endianness) {
		for (size_t i = 0; i < std::extent_v<T>; ++i) {
			serial::loads(
				std::span<uint8_t const, ItemSizeBytes>{bytes.subspan(ItemSizeBytes * i, ItemSizeBytes)},
				value[i],
				endianness
			);
		}
	}

	static constexpr void dumps(std::span<uint8_t, SizeBytes> bytes, T const& value, std::endian endianness) {
		for (size_t i = 0; i < std::extent_v<T>; ++i) {
			serial::dumps(
				std::span<uint8_t, ItemSizeBytes>{bytes.subspan(ItemSizeBytes * i, ItemSizeBytes)}, value[i], endianness
			);
		}
	}
};

// Tuple-like types ====================================================================================================
template<typename T>
concept TupleLike = requires(T value) {
	{ std::tuple_size<T>{} };
	typename std::tuple_element<0, T>;
	{ std::get<0>(value) };
};

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

namespace impl {
template<TupleLike T, size_t I = 0>
consteval bool each_element_has_constant_size_meta() {
	if constexpr (I == std::tuple_size_v<T>) {
		return true;
	} else {
		return HasConstantSizeMeta<std::tuple_element_t<I, T>> and each_element_has_constant_size_meta<T, I + 1>();
	}
}
}  // namespace impl

template<typename T>
concept ConstantSizeTupleLike = TupleLike<T> and impl::each_element_has_constant_size_meta<T>();

template<TupleLike T>
struct meta<T> {
	template<InputStream S>
	static constexpr void load(S& stream, T& value, std::endian endianness) {
		tuple_for_each(
			[&](auto&& item) {
				static_assert(HasMeta<decltype(item)>, "tuple item does not have a meta<> specialization");
				serial::load(stream, item, endianness);
			},
			value
		);
	}

	template<OutputStream S>
	static constexpr void dump(S& stream, T const& value, std::endian endianness) {
		tuple_for_each(
			[&](auto&& item) {
				static_assert(HasMeta<decltype(item)>, "tuple item does not have a meta<> specialization");
				serial::dump(stream, item, endianness);
			},
			value
		);
	}
};

template<ConstantSizeTupleLike T>
struct meta<T> {
	template<ConstantSizeTupleLike T_, size_t I = 0>
	static consteval size_t size_bytes_impl() {
		if constexpr (I == std::tuple_size_v<T_>) {
			return 0;
		} else {
			static_assert(HasConstantSizeMeta<std::tuple_element_t<I, T_>>);
			return serial::SizeBytes<std::tuple_element_t<I, T_>> + size_bytes_impl<T_, I + 1>();
		}
	}

	static constexpr const size_t SizeBytes = size_bytes_impl<T>();

	static constexpr void loads(std::span<uint8_t const, SizeBytes> bytes, T& value, std::endian endianness) {
		size_t offset = 0;
		tuple_for_each(
			[&](auto&& item) {
				constexpr size_t ItemSizeBytes = serial::SizeBytes<decltype(item)>;

				serial::loads(
					std::span<uint8_t const, ItemSizeBytes>{bytes.subspan(offset, ItemSizeBytes)}, item, endianness
				);
				offset += ItemSizeBytes;
			},
			value
		);
	}

	static constexpr void dumps(std::span<uint8_t, SizeBytes> bytes, T const& value, std::endian endianness) {
		size_t offset = 0;
		tuple_for_each(
			[&](auto&& item) {
				constexpr size_t ItemSizeBytes = serial::SizeBytes<decltype(item)>;

				serial::dumps(
					std::span<uint8_t, ItemSizeBytes>{bytes.subspan(offset, ItemSizeBytes)}, item, endianness
				);
				offset += ItemSizeBytes;
			},
			value
		);
	}
};

/// std::chrono types ==================================================================================================
template<typename Rep, typename Period>
struct meta<std::chrono::duration<Rep, Period>> {
	using T = std::chrono::duration<Rep, Period>;
	static constexpr const size_t SizeBytes = sizeof(Rep);

	static constexpr void loads(std::span<uint8_t const, SizeBytes> bytes, T& value, std::endian endianness) {
		auto count = serial::loads<Rep>(bytes, endianness);
		value = T{count};
	}

	static constexpr void dumps(std::span<uint8_t, SizeBytes> bytes, T value, std::endian endianness) {
		serial::dumps<Rep>(bytes, value.count(), endianness);
	}
};

template<typename Clock, typename Duration>
struct meta<std::chrono::time_point<Clock, Duration>> {
	using T = std::chrono::time_point<Clock, Duration>;
	static constexpr const size_t SizeBytes = serial::SizeBytes<Duration>;

	static constexpr void loads(std::span<uint8_t const, SizeBytes> bytes, T& value, std::endian endianness) {
		auto time_since_epoch = serial::loads<Duration>(bytes, endianness);
		value = T{time_since_epoch};
	}

	static constexpr void dumps(std::span<uint8_t, SizeBytes> bytes, T value, std::endian endianness) {
		serial::dumps<Duration>(bytes, value.time_since_epoch(), endianness);
	}
};

}  // namespace pgs::serial
