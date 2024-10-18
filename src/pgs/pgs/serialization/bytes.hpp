#pragma once
#include <array>
#include <cstdio>
#include <format>
#include <iostream>
#include <span>
#include <type_traits>
#include <vector>

#include "traits.hpp"

namespace pgs::serial {

template<ByteLike B1, ByteLike B2>
	requires(not std::is_const_v<B2>)
constexpr void read_bytes(std::basic_istream<B1>& stream, std::span<B2> output) {
	stream.read(reinterpret_cast<B1*>(output.data()), output.size());
	size_t read_count = stream.gcount();
	if (read_count < output.size()) {
		if (stream.eof())
			throw std::runtime_error(std::format("read_bytes: read failed: EOF reached after {} bytes", read_count));
		if (stream.fail())
			throw std::runtime_error(std::format("read_bytes: read failed: failbit set after {} bytes", read_count));
		if (stream.bad())
			throw std::runtime_error(std::format("read_bytes: read failed: badbit set after {} bytes", read_count));
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

inline std::vector<uint8_t> read_bytes(FILE* stream, size_t count) {
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

inline void write_bytes(FILE* stream, std::span<uint8_t const> bytes) {
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

}  // namespace pgs::serial
