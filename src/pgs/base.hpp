#pragma once
#include "serialization.hpp"

namespace pgs {

/// PGS stores timestamps as 4-byte integers with 90kHz precision.
using Duration = std::chrono::duration<uint32_t, std::ratio<1, 90000>>;
using Timestamp = std::chrono::time_point<std::chrono::steady_clock, Duration>;

/// Indicates that a duration value, such as in a Subtitle, is unknown.
static constexpr Duration UnknownDuration{std::numeric_limits<Duration::rep>::max()};

template<typename T>
struct Rect {
	T x = {};
	T y = {};
	T width = {};
	T height = {};

	bool operator==(Rect const&) const = default;
};

template<typename T>
struct serial::meta<Rect<T>> {
	static constexpr auto proxy = [](auto& obj) { return std::tie(obj.x, obj.y, obj.width, obj.height); };
};

}  // namespace pgs
