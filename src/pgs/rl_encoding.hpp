#pragma once
#include <algorithm>
#include <iterator>
#include <span>
#include <unordered_map>
#include <vector>

template<typename T = std::uint8_t>
struct Bitmap {
	Bitmap(uint16_t width, uint16_t height) : m_data(static_cast<std::size_t>(width) * height), m_row_size(width) {}

	Bitmap(uint16_t width, uint16_t height, std::vector<T> data) : m_data(std::move(data)), m_row_size(width) {
		assert(m_data.size() == static_cast<std::size_t>(width) * height);
	}

	uint16_t rows() const { return m_data.size() / columns(); }
	uint16_t height() const { return rows(); }

	uint16_t columns() const { return m_row_size; }
	uint16_t width() const { return columns(); }

	std::span<T> operator[](uint16_t row) { return std::span{m_data}.subspan(m_row_size * row, m_row_size); }
	std::span<T const> operator[](uint16_t row) const {
		return std::span{m_data}.subspan(m_row_size * row, m_row_size);
	}

	T* data() { return m_data.data(); }
	T const* data() const { return m_data.data(); }

	std::size_t size() const { return m_data.size(); }

	auto begin() { return m_data.data(); }
	auto begin() const { return m_data.data(); }
	auto end() { return m_data.data() + m_data.size(); }
	auto end() const { return m_data.data() + m_data.size(); }

private:
	std::vector<T> m_data;
	uint16_t m_row_size;
};

inline Bitmap<uint8_t> decode(std::span<uint8_t const> data);

template<typename T>
inline Bitmap<T> decode(std::span<uint8_t const> data, std::array<T, 256> palette) {
	Bitmap indexed = decode(data);
	std::vector<T> result;
	result.reserve(indexed.size());

	std::ranges::transform(indexed, std::back_inserter(result), [&](uint8_t index) { return palette[index]; });

	return Bitmap{indexed.width(), indexed.height(), std::move(result)};
}

inline std::vector<uint8_t> encode(Bitmap<uint8_t> const& bitmap);

template<typename T>
inline std::vector<uint8_t> encode(Bitmap<T> const& bitmap, std::unordered_map<T, uint8_t> palette) {
	Bitmap<uint8_t> indexed{bitmap.width(), bitmap.height()};
	std::ranges::transform(bitmap, indexed.begin(), [&](T const& value) { return palette[value]; });

	return encode(indexed);
}

#include "rl_encoding.inl.hpp"
