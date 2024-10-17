#pragma once
#include "rle.hpp"
#include "serialization.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace pgs::rle {

inline ByteBitmap decode(std::span<uint8_t const> data) {
	if (data.empty()) throw std::runtime_error("data buffer is empty");

	auto width = serial::loads<uint16_t>(data.subspan<0, 2>());
	auto height = serial::loads<uint16_t>(data.subspan<2, 2>());
	auto total_size = static_cast<std::size_t>(width) * height;
	data = data.subspan(4);

	std::vector<uint8_t> result;
	result.reserve(std::size_t(width) * height);

	std::size_t row_i = 0;
	for (auto iter = data.begin(); iter != data.end(); ++iter, ++row_i) {
		auto advance = [&]() {
			++iter;
			if (iter == data.end()) throw std::runtime_error(std::format("row {}: unexpected end of buffer", row_i));
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

			uint8_t flag = byte1 >> 6;                 // top 2 flag bits
			uint8_t byte1_clean = byte1 & 0b00111111;  // remove top 2 flag bits
			size_t n_pixels = 0;
			uint8_t color = 0;

			if (flag == 0b00) {
				// 00000000 00LLLLLL: L pixels in color 0 (L between 1 and 63)
				n_pixels = byte1_clean;
			} else if (flag == 0b01) {
				// 00000000 01LLLLLL LLLLLLLL: L pixels in color 0 (L between 64 and 16383)
				uint8_t byte2 = advance();
				n_pixels = (byte1_clean << 8) + byte2;
				if (not(64 <= n_pixels and n_pixels <= 16383)) {
					throw std::runtime_error(
						std::format("unrecognized byte sequence {:#04x} {:#04x} {:#04x}", byte0, byte1, byte2)
					);
				}
			} else if (flag == 0b10) {
				// 00000000 10LLLLLL CCCCCCCC: L pixels in color C (L between 3 and 63)
				n_pixels = byte1_clean;
				if (not(3 <= n_pixels and n_pixels <= 63)) {
					throw std::runtime_error(std::format("unrecognized byte sequence {:#04x} {:#04x}", byte0, byte1));
				}

				uint8_t byte2 = advance();
				color = byte2;
			} else if (flag == 0b11) {
				// 00000000 11LLLLLL LLLLLLLL CCCCCCCC: L pixels in color C (L between 64 and 16383)
				uint8_t byte2 = advance();
				n_pixels = (byte1_clean << 8) + byte2;
				if (not(64 <= n_pixels and n_pixels <= 16383)) {
					throw std::runtime_error(
						std::format("unrecognized byte sequence {:#04x} {:#04x} {:#04x}", byte0, byte1, byte2)
					);
				}

				uint8_t byte3 = advance();
				color = byte3;
			}

			result.insert(result.end(), n_pixels, color);
		}
	}

	if (row_i != height) {
		throw std::runtime_error(
			std::format("decoded row count ({}) does not match row count from metadata ({})", row_i, height)
		);
	}

	if (result.size() != total_size) {
		throw std::runtime_error(std::format(
			"decoded size ({}) does not match size from metadata ({}*{}={})", data.size(), width, height, total_size
		));
	}

	return Bitmap{width, height, std::move(result)};
}

inline std::vector<uint8_t> encode(BitmapView<uint8_t> const& bitmap) {
	std::vector<uint8_t> result;
	result.resize(4);

	serial::dumps(std::span{result}.subspan<0, 2>(), bitmap.width());
	serial::dumps(std::span{result}.subspan<2, 2>(), bitmap.height());

	for (std::size_t y = 0; y < bitmap.height(); ++y) {
		auto row = bitmap.row(y);

		size_t n_same = 0;
		for (size_t x = 0; x < row.size(); x += n_same) {
			// note: max allowed same pixels for packing is 16383
			for (n_same = 1; x + n_same < row.size() and n_same <= 16383; ++n_same) {
				if (row[x] != row[x + n_same]) break;
			}

			uint8_t color = row[x];

			if (color == 0) {
				if (n_same < 64) {
					// 00000000 00LLLLLL: L pixels in color 0 (L between 1 and 63)
					result.push_back(0);
					result.push_back(static_cast<uint8_t>(n_same));
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
	}

	return result;
}

}  // namespace pgs::rle
