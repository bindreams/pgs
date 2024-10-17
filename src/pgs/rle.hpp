#pragma once
#include <algorithm>
#include <iterator>
#include <span>
#include <unordered_map>
#include <vector>

#include "bitmap.hpp"

namespace pgs {

namespace rle {

inline Bitmap<uint8_t> decode(std::span<uint8_t const> data);

template<typename T, typename F>
inline Bitmap<T> decode(std::span<uint8_t const> data, F&& palette) {
	Bitmap indexed = decode(data);
	std::vector<T> result;
	result.reserve(indexed.size());

	std::ranges::transform(indexed, std::back_inserter(result), [&](uint8_t index) { return palette(index); });

	return Bitmap{indexed.width(), indexed.height(), std::move(result)};
}

inline std::vector<uint8_t> encode(BitmapView<uint8_t> const& bitmap);

template<typename T, typename F>
inline std::vector<uint8_t> encode(BitmapView<T> const& bitmap, F&& palette) {
	Bitmap<uint8_t> indexed{bitmap.width(), bitmap.height()};
	std::ranges::transform(bitmap, indexed.begin(), [&](T const& value) { return palette(value); });

	return encode(indexed);
}

}  // namespace rle
}  // namespace pgs

#include "rle.inl.hpp"
