#pragma once
#include <cstdint>
#include <tuple>

namespace pgs {
namespace colormodels {

struct Rgb {
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;

	auto operator<=>(Rgb const&) const = default;
};

struct Rgba {
	uint8_t red = 0;
	uint8_t green = 0;
	uint8_t blue = 0;
	uint8_t alpha = 0;

	explicit operator Rgb() const { return {red, green, blue}; }
	auto operator<=>(Rgba const&) const = default;
};

struct YCbCr_BT709 {
	uint8_t y = 16;
	uint8_t cb = 128;
	uint8_t cr = 128;

	auto operator<=>(YCbCr_BT709 const&) const = default;
};

struct YCbCrA_BT709 {
	uint8_t y = 16;
	uint8_t cb = 128;
	uint8_t cr = 128;
	uint8_t alpha = 0;

	explicit operator YCbCr_BT709() const { return {y, cb, cr}; }
	auto operator<=>(YCbCrA_BT709 const&) const = default;
};

}  // namespace colormodels

template<typename T, typename U>
inline T color_cast(U color);

template<>
inline colormodels::Rgb color_cast<colormodels::Rgb, colormodels::YCbCr_BT709>(colormodels::YCbCr_BT709 color);

template<>
inline colormodels::YCbCr_BT709 color_cast<colormodels::YCbCr_BT709, colormodels::Rgb>(colormodels::Rgb color);

template<>
inline colormodels::Rgba color_cast<colormodels::Rgba, colormodels::YCbCrA_BT709>(colormodels::YCbCrA_BT709 color);

template<>
inline colormodels::YCbCrA_BT709 color_cast<colormodels::YCbCrA_BT709, colormodels::Rgba>(colormodels::Rgba color);

}  // namespace pgs

#include "colors.inl.hpp"
