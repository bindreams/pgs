#pragma once
#include <type_traits>

#include "traits.hpp"

namespace pgs::serial {

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

namespace impl {
template<HasMeta T>
consteval bool has_proxy_or_size_bytes() {
	if constexpr (HasProxyMeta<T>) {
		return has_proxy_or_size_bytes<ProxyType<T>>();
	} else {
		return requires {
			{ meta<T>::SizeBytes };
		};
	}
}
}  // namespace impl

template<typename T>
concept ConstantSizeMeta = HasMeta<T> and impl::has_proxy_or_size_bytes<T>();

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
constexpr void loads(std::span<B, SizeBytes<T>> bytes, T&& value, std::endian endianness = std::endian::big) {
	if constexpr (HasProxyMeta<T>) {
		loads(bytes, meta<T>::proxy(std::forward<T>(value)), endianness);
	} else {
		meta<T>::loads(bytes, value, endianness);
	}
}

template<ConstantSizeMeta T>
constexpr void
loads(std::span<const uint8_t, SizeBytes<T>> bytes, T&& value, std::endian endianness = std::endian::big) {
	return loads<T, const uint8_t>(bytes, std::forward<T>(value), endianness);
}

template<ConstantSizeMeta T, ByteLike B>
constexpr T loads(std::span<B, SizeBytes<T>> bytes, std::endian endianness = std::endian::big) {
	T value;
	loads(bytes, value, endianness);
	return value;
}

template<HasMeta T, ByteLike B>
	requires(not std::is_const_v<B>)
constexpr void dumps(std::span<B, SizeBytes<T>> bytes, T const& value, std::endian endianness = std::endian::big) {
	if constexpr (HasProxyMeta<T>) {
		dumps(bytes, meta<T>::proxy(value), endianness);
	} else {
		meta<T>::dumps(bytes, value, endianness);
	}
}

}  // namespace pgs::serial
