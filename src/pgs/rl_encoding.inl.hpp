#pragma once
#include "rl_encoding.hpp"

#include <algorithm>
#include <iterator>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

inline std::vector<uint8_t> decode(std::span<uint8_t const> data) {
	if (data.empty()) throw std::invalid_argument("data buffer is empty");

	std::vector<uint8_t> result;

	auto iter = data.begin();
	auto advance = [&]() {
		++iter;
		if (iter == data.end()) throw std::invalid_argument("reached end of data");
		return *iter;
	};

	for (; iter != data.end(); ++iter) {
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
	}

	return result;
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
