#pragma once
#include <cstdint>
#include <tuple>

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> convert_RGBA_YCbCrA_BT709(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> rgba
);

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>
convert_RGBA_YCbCrA_BT709(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha);

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> convert_RGB_YCbCr_BT709(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> rgb
);

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>
convert_RGB_YCbCr_BT709(std::uint8_t red, std::uint8_t green, std::uint8_t blue);

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> convert_YCbCrA_BT709_RGBA(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> ycbcra
);

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>
convert_YCbCrA_BT709_RGBA(std::uint8_t y, std::uint8_t cb, std::uint8_t cr, std::uint8_t alpha);

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> convert_YCbCr_BT709_RGB(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> ycbcr
);

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>
convert_YCbCr_BT709_RGB(std::uint8_t y, std::uint8_t cb, std::uint8_t cr);

#include "colors.inl.hpp"
