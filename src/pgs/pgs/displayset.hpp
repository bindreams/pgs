#pragma once
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base.hpp"
#include "bitmap.hpp"
#include "colors.hpp"
#include "rle.hpp"
#include "segment.hpp"
#include "utility.hpp"

namespace pgs {

struct Subtitle {
	Bitmap<colormodels::Rgba> bitmap;
	uint16_t x = 0;
	uint16_t y = 0;
	Timestamp timestamp;
	Duration duration = UnknownDuration;
};

struct DisplaySet {
	Timestamp timestamp;
	Duration duration = UnknownDuration;

	std::unordered_map<uint8_t, Rect<uint16_t>> windows;
	std::unordered_map<uint8_t, PaletteDefinition> palettes;
	std::unordered_map<uint16_t, ByteBitmap> objects;
	std::vector<PresentationComposition::CompositionObject> compositions;
	uint8_t current_palette = 0;

	bool empty() const { return compositions.empty(); }

	void from_segments(std::span<Segment const> segments) {
		// A display set starts with a PCS and ends with END segment.
		assert(segments.size() >= 2);
		assert(segments[0].type() == Segment::Type::PresentationComposition);
		assert(segments[segments.size() - 1].type() == Segment::Type::EndOfDisplaySet);

		auto& pcs = std::get<PresentationComposition>(segments[0]);

		if (pcs.state != PresentationComposition::State::Normal) {
			windows.clear();
			palettes.clear();
			objects.clear();
			current_palette = 0;
		}

		timestamp = pcs.presentation_ts;
		duration = UnknownDuration;
		compositions = pcs.composition_objects;
		current_palette = pcs.update_palette_id.value_or(current_palette);

		std::unordered_map<uint16_t, std::vector<uint8_t>> incomplete_objects;

		auto other_segments = segments.subspan(1, segments.size() - 2);
		for (Segment const& any_segment : other_segments) {
			if (any_segment.type() == Segment::Type::WindowDefinition) {
				auto& segment = std::get<WindowDefinition>(any_segment);

				for (auto& [id, window] : segment.windows) {
					windows.insert_or_assign(id, window);
				}
			} else if (any_segment.type() == Segment::Type::PaletteDefinition) {
				auto& segment = std::get<PaletteDefinition>(any_segment);

				palettes.insert_or_assign(segment.id, segment);
			} else if (any_segment.type() == Segment::Type::ObjectDefinition) {
				auto& segment = std::get<ObjectDefinition>(any_segment);

				if (std::to_underlying(segment.sequence_flag & ObjectDefinition::SequenceFlag::First)) {
					assert(not incomplete_objects.contains(segment.id));  // object with this id already in progress
					assert(not objects.contains(segment.id));             // object with this id already decoded
				} else {
					assert(incomplete_objects.contains(segment.id));  // no "first" segment was ever found
				}

				incomplete_objects[segment.id].append_range(segment.data);

				if (std::to_underlying(segment.sequence_flag & ObjectDefinition::SequenceFlag::Last)) {
					objects.emplace(segment.id, rle::decode(incomplete_objects[segment.id]));
					incomplete_objects.erase(segment.id);
				}
			} else {
				assert(any_segment.type() != Segment::Type::PresentationComposition);  // PCS inside a display set
				assert(any_segment.type() != Segment::Type::EndOfDisplaySet);          // END inside a display set
				assert(false);                                                         // unhandled segment type
				unreachable();
			}
		}
	}

	std::generator<Subtitle> subtitles() const {
		using colormodels::Rgba;

		for (auto& composition : compositions) {
			auto& window = windows.at(composition.window_id);
			auto& object = objects.at(composition.object_id);
			auto& clut = palettes.at(current_palette).clut;

			auto get_color = [&](uint8_t id) {
				auto it = clut.find(id);
				if (it != clut.end()) return color_cast<Rgba>(it->second);

				// In practice, a color may be missing from the CLUT, in which case we return a transparent color.
				return Rgba{0, 0, 0, 0};
			};
			Bitmap<Rgba> original_bitmap{object.width(), object.height()};
			std::ranges::transform(object, original_bitmap.begin(), get_color);

			auto bitmap = original_bitmap.view();

			// crop 1: from composition object
			if (composition.crop.has_value()) {
				bitmap = bitmap.crop(
					composition.crop->x, composition.crop->y, composition.crop->width, composition.crop->height
				);
			}

			// crop 2: from window
			bitmap = bitmap.crop(0, 0, window.width, window.height);

			co_yield Subtitle{Bitmap<Rgba>(bitmap), window.x, window.y, timestamp, duration};
		}
	}
};

template<std::ranges::input_range R>
	requires std::convertible_to<std::ranges::range_value_t<R>, Segment const&>
std::generator<DisplaySet> from_segments(R&& segments) {
	DisplaySet current;
	std::vector<Segment> segment_buffer;

	for (auto&& segment : segments) {
		segment_buffer.emplace_back(segment);
		if (segment_buffer.back().type() == Segment::Type::EndOfDisplaySet) {
			DisplaySet last = current;
			current.from_segments(segment_buffer);
			last.duration = current.timestamp - last.timestamp;

			if (not last.empty()) co_yield last;
			segment_buffer.clear();
		}
	}

	co_yield current;
	assert(segment_buffer.empty());  // segment range was not terminated by an END segment
}

}  // namespace pgs
