#pragma once
#include "colors.hpp"

#include <algorithm>
#include <array>
#include <cmath>

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> convert_RGBA_YCbCrA_BT709(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> rgba
) {
	auto [red, green, blue, alpha] = rgba;
	return convert_RGBA_YCbCrA_BT709(red, green, blue, alpha);
}

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>
convert_RGBA_YCbCrA_BT709(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) {
	auto [y, cb, cr] = convert_RGB_YCbCr_BT709(red, green, blue);
	return {y, cb, cr, alpha};
}

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> convert_RGB_YCbCr_BT709(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> rgb
) {
	auto [red, green, blue] = rgb;
	return convert_RGB_YCbCr_BT709(red, green, blue);
}

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>
convert_RGB_YCbCr_BT709(std::uint8_t red, std::uint8_t green, std::uint8_t blue) {
	constexpr const std::array<std::array<double, 3>, 3> matrix = {{
		{+0.183, +0.614, +0.062},
		{-0.101, -0.339, +0.439},
		{+0.439, -0.399, -0.040},
	}};
	constexpr const std::array<double, 3> add = {16, 128, 128};

	std::array<double, 3> rgb = {static_cast<double>(red), static_cast<double>(green), static_cast<double>(blue)};
	std::array<double, 3> ycbcr = {};

	static_assert(rgb.size() == matrix[0].size());
	static_assert(ycbcr.size() == matrix.size());

	for (size_t row = 0; row < matrix.size(); ++row) {
		for (size_t col = 0; col < matrix[row].size(); ++col) {
			ycbcr[row] += matrix[row][col] * rgb[col];
		}
	}

	for (size_t i = 0; i < ycbcr.size(); ++i) {
		ycbcr[i] += add[i];
	}

	for (auto& color : ycbcr) color = std::round(color);
	ycbcr[0] = std::clamp(ycbcr[0], 16.0, 235.0);
	ycbcr[1] = std::clamp(ycbcr[1], 16.0, 240.0);
	ycbcr[2] = std::clamp(ycbcr[2], 16.0, 240.0);

	return {
		static_cast<std::uint8_t>(ycbcr[0]),
		static_cast<std::uint8_t>(ycbcr[1]),
		static_cast<std::uint8_t>(ycbcr[2]),
	};
}

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> convert_YCbCrA_BT709_RGBA(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t> ycbcra
) {
	auto [y, cb, cr, alpha] = ycbcra;
	return convert_YCbCrA_BT709_RGBA(y, cb, cr, alpha);
}

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t>
convert_YCbCrA_BT709_RGBA(std::uint8_t y, std::uint8_t cb, std::uint8_t cr, std::uint8_t alpha) {
	auto [red, green, blue] = convert_YCbCr_BT709_RGB(y, cb, cr);
	return {red, green, blue, alpha};
}

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> convert_YCbCr_BT709_RGB(
	std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> ycbcr
) {
	auto [y, cb, cr] = ycbcr;
	return convert_YCbCr_BT709_RGB(y, cb, cr);
}

inline std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>
convert_YCbCr_BT709_RGB(std::uint8_t y, std::uint8_t cb, std::uint8_t cr) {
	constexpr const std::array<std::array<double, 3>, 3> matrix = {{
		{+1.164, +0.000, +1.793},
		{+1.164, -0.213, -0.533},
		{+1.164, +2.112, +0.000},
	}};
	constexpr const std::array<double, 3> add = {16, 128, 128};

	std::array<double, 3> ycbcr = {static_cast<double>(y), static_cast<double>(cb), static_cast<double>(cr)};
	std::array<double, 3> rgb = {};

	for (size_t i = 0; i < ycbcr.size(); ++i) {
		ycbcr[i] -= add[i];
	}

	static_assert(ycbcr.size() == matrix[0].size());
	static_assert(rgb.size() == matrix.size());

	for (size_t row = 0; row < matrix.size(); ++row) {
		for (size_t col = 0; col < matrix[row].size(); ++col) {
			rgb[row] += matrix[row][col] * ycbcr[col];
		}
	}

	for (auto& color : rgb) color = std::round(color);
	for (auto& color : rgb) color = std::clamp(color, 0.0, 255.0);

	return {
		static_cast<std::uint8_t>(rgb[0]),
		static_cast<std::uint8_t>(rgb[1]),
		static_cast<std::uint8_t>(rgb[2]),
	};
}
