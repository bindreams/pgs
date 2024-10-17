#pragma once
#include "serialization.hpp"

namespace pgs {

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
