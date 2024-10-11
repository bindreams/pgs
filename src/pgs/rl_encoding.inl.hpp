#pragma once
#include "rl_encoding.hpp"
#include "serialize.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

inline Bitmap<uint8_t> decode(std::span<uint8_t const> data) {
	if (data.empty()) throw std::runtime_error("data buffer is empty");

	auto width = serialize::loads<std::uint16_t>(data.subspan<0, 2>());
	auto height = serialize::loads<std::uint16_t>(data.subspan<2, 2>());
	auto total_size = static_cast<std::size_t>(width) * height;
	data = data.subspan(4);
	if (data.size() != total_size) {
		throw std::runtime_error(std::format(
			"pixel buffer size ({}) does not match decoded size ({}*{}={})", data.size(), width, height, total_size
		));
	}

	std::vector<uint8_t> result;
	result.reserve(std::size_t(width) * height);

	for (std::size_t row_i = 0; row_i < height; ++row_i) {
		auto row = data.subspan(width * row_i, width);

		auto iter = row.begin();
		auto advance = [&]() {
			++iter;
			if (iter == row.end()) throw std::runtime_error(std::format("row {}: unexpected end of row", row_i));
			return *iter;
		};

		// Decode a single line
		for (uint8_t byte0 = *iter;; byte0 = advance()) {
			if (byte0 != 0) {
				// CCCCCCCC: one pixel in color C
				result.push_back(byte0);
				continue;
			}

			uint8_t byte1 = advance();

			if (byte1 == 0) {
				// 00000000 00000000: end of line
				break;
			}

			uint8_t flag = byte1 >> 6;  // top 2 bits
			byte1 &= 0b00111111;        // remove top 2 bits

			size_t n_pixels = 0;
			uint8_t color = 0;

			if (flag == 0b00) {
				// 00000000 00LLLLLL: L pixels in color 0 (L between 1 and 63)
				n_pixels = byte1;
			} else if (flag == 0b01) {
				// 00000000 01LLLLLL LLLLLLLL: L pixels in color 0 (L between 64 and 16383)
				uint8_t byte2 = advance();
				n_pixels = (byte1 << 8) + byte2;
			} else if (flag == 0b10) {
				// 00000000 10LLLLLL CCCCCCCC: L pixels in color C (L between 3 and 63)
				uint8_t byte2 = advance();
				n_pixels = byte1;
				color = byte2;
			} else if (flag == 0b11) {
				// 00000000 11LLLLLL LLLLLLLL CCCCCCCC: L pixels in color C (L between 64 and 16383)
				uint8_t byte2 = advance();
				uint8_t byte3 = advance();
				n_pixels = (byte1 << 8) + byte2;
				color = byte3;
			}

			result.insert(result.end(), n_pixels, color);
		}

		++iter;
		if (iter != row.end()) throw std::runtime_error(std::format("unexpected data after end of row {}", row_i));
	}

	return Bitmap{width, height, std::move(result)};
}

inline std::vector<uint8_t> encode(std::span<uint8_t const> data) {
	std::vector<uint8_t> result;

	size_t n_same = 0;
	for (size_t i = 0; i < data.size(); i += n_same) {
		size_t j = 0;
		for (j = i + 1; j < data.size(); ++j) {
			if (data[j] != data[i]) break;
		}

		size_t n_same = std::min<size_t>(j - i, 16383);  // max allowed same pixels is 16383
		uint8_t color = data[i];

		if (color == 0) {
			if (n_same < 64) {
				// 00000000 00LLLLLL: L pixels in color 0 (L between 1 and 63)
				result.push_back(0);
				result.push_back(n_same);
				continue;
			}

			// 00000000 01LLLLLL LLLLLLLL: L pixels in color 0 (L between 64 and 16383)
			result.push_back(0);
			result.push_back(n_same >> 8);
			result.push_back(n_same % 256);
			continue;
		}

		if (n_same < 3) {
			// CCCCCCCC: one pixel in color C
			// ...repeated n_same times
			result.insert(result.end(), n_same, color);
			continue;
		}
		if (n_same < 64) {
			// 00000000 10LLLLLL CCCCCCCC: L pixels in color C (L between 3 and 63)
			result.push_back(0);
			result.push_back(n_same | 0b10000000);
			result.push_back(color);
			continue;
		}

		// 00000000 11LLLLLL LLLLLLLL CCCCCCCC: L pixels in color C (L between 64 and 16383)
		result.push_back(0);
		result.push_back((n_same >> 8) | 0b11000000);
		result.push_back(n_same % 256);
		result.push_back(color);
	}

	// 00000000 00000000: end of line
	result.insert(result.end(), 2, 0);
	return result;
}
