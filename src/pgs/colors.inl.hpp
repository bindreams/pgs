#pragma once
#include "colors.hpp"

#include <algorithm>
#include <array>
#include <cmath>

template<>
inline colormodels::YCbCrA_BT709 convert<colormodels::YCbCrA_BT709, colormodels::Rgba>(colormodels::Rgba color) {
	auto [y, cb, cr] = convert<colormodels::YCbCr_BT709>(static_cast<colormodels::Rgb>(color));
	return {y, cb, cr, color.alpha};
}

template<>
inline colormodels::YCbCr_BT709 convert<colormodels::YCbCr_BT709, colormodels::Rgb>(colormodels::Rgb color) {
	constexpr const std::array<std::array<double, 3>, 3> matrix = {{
		{+0.183, +0.614, +0.062},
		{-0.101, -0.339, +0.439},
		{+0.439, -0.399, -0.040},
	}};
	constexpr const std::array<double, 3> add = {16, 128, 128};

	auto& [red, green, blue] = color;
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

template<>
inline colormodels::Rgba convert<colormodels::Rgba, colormodels::YCbCrA_BT709>(colormodels::YCbCrA_BT709 color) {
	auto [red, green, blue] = convert<colormodels::Rgb>(static_cast<colormodels::YCbCr_BT709>(color));
	return {red, green, blue, color.alpha};
}

template<>
inline colormodels::Rgb convert<colormodels::Rgb, colormodels::YCbCr_BT709>(colormodels::YCbCr_BT709 color) {
	// Verify that the color passed falls within the YCbCr limits
	assert(16 <= color.y and color.y <= 235);
	assert(16 <= color.cb and color.cb <= 240);
	assert(16 <= color.cr and color.cr <= 240);

	constexpr const std::array<std::array<double, 3>, 3> matrix = {{
		{+1.164, +0.000, +1.793},
		{+1.164, -0.213, -0.533},
		{+1.164, +2.112, +0.000},
	}};
	constexpr const std::array<double, 3> add = {16, 128, 128};

	auto& [y, cb, cr] = color;
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
