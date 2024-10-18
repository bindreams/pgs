#pragma once
#include <ranges>
#include "colors.hpp"
#include "displayset.hpp"
#include "rle.hpp"
#include "segment.hpp"
#include "utility.hpp"

namespace pgs {

std::generator<Subtitle> subtitles(std::ranges::input_range auto&& segments) {
	for (auto&& ds : from_segments(segments)) {
		for (auto&& sub : ds.subtitles()) {
			co_yield sub;
		}
	}
}

}  // namespace pgs
