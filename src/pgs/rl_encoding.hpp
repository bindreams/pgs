#pragma once
#include <algorithm>
#include <iterator>
#include <span>
#include <unordered_map>
#include <vector>

std::vector<uint8_t> decode(std::span<uint8_t const> data);

template<typename T>
std::vector<T> decode(std::span<uint8_t const> data, std::array<T, 256> palette) {
	auto indexed = decode(data);
	std::vector<T> result;
	result.reserve(indexed.size());

	std::transform(indexed.begin(), indexed.end(), std::back_inserter(result), [&](uint8_t index) {
		return palette[index];
	});

	return result;
}

std::vector<uint8_t> encode(std::span<uint8_t const> data);

template<typename T>
std::vector<uint8_t> encode(std::span<T const> data, std::unordered_map<T, uint8_t> palette) {
	std::vector<uint8_t> indexed;
	indexed.reserve(data.size());

	std::transform(data.begin(), data.end(), std::back_inserter(indexed), [&](T const& value) {
		return palette[value];
	});

	return encode(indexed);
}

#include "rl_encoding.inl.hpp"
