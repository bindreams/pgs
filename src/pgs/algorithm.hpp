#pragma once
#include <ranges>
#include "colors.hpp"
#include "rle.hpp"
#include "segment.hpp"

namespace pgs {

struct Subtitle {
	Timestamp timestamp;
	Duration duration;

	Bitmap<colormodels::Rgba> bitmap;
};

std::vector<Subtitle> subtitles(std::ranges::input_range auto&& segments) {
	// std::unordered_map<int, Subtitle>
	std::unordered_map<uint8_t, WindowDefinition::Window> windows;
	std::unordered_map<uint8_t, PaletteDefinition> palettes;
	std::unordered_map<uint16_t, ObjectDefinition> objects;
	std::vector<PresentationComposition::CompositionObject> compositions;
	uint8_t current_palette = 0;
	Timestamp timestamp;

	std::vector<Subtitle> current;
	std::vector<Subtitle> result;

	for (Segment& any_segment : segments) {
		if (any_segment.type() == Segment::Type::PresentationComposition) {
			auto& segment = std::get<PresentationComposition>(any_segment);

			timestamp = segment.presentation_ts;

			for (Subtitle& sub : current) {
				sub.duration = timestamp - sub.timestamp;
				result.push_back(std::move(sub));
			}

			current.clear();

			if (segment.state != PresentationComposition::State::Normal) {
				windows.clear();
				palettes.clear();
				objects.clear();
				current_palette = 0;
			}

			compositions.clear();
			current_palette = segment.update_palette_id.value_or(current_palette);

			for (auto& composition : segment.composition_objects) {
				compositions.push_back(composition);
			}

		} else if (any_segment.type() == Segment::Type::WindowDefinition) {
			auto& segment = std::get<WindowDefinition>(any_segment);

			for (auto& window : segment.windows) {
				windows.insert_or_assign(window.id, window);
			}
		} else if (any_segment.type() == Segment::Type::PaletteDefinition) {
			auto& segment = std::get<PaletteDefinition>(any_segment);

			palettes.insert_or_assign(segment.id, segment);
		} else if (any_segment.type() == Segment::Type::ObjectDefinition) {
			auto& segment = std::get<ObjectDefinition>(any_segment);
			assert(
				segment.sequence_flag == (ObjectDefinition::SequenceFlag::First | ObjectDefinition::SequenceFlag::Last)
			);

			objects.insert_or_assign(segment.id, segment);
		} else if (any_segment.type() == Segment::Type::EndOfDisplaySet) {
			using colormodels::Rgba;

			for (auto& composition : compositions) {
				auto& window = windows[composition.window_id];
				auto& object = objects[composition.object_id];
				auto& clut = palettes[current_palette].clut;

				auto get_color = [&](uint8_t id) { return color_cast<Rgba>(clut[id]); };
				Bitmap<Rgba> original_bitmap = rle::decode<Rgba>(object.data, get_color);
				auto bitmap = original_bitmap.view();

				// crop 1: from composition object
				if (composition.crop.has_value()) {
					bitmap = bitmap.crop(
						composition.crop->x, composition.crop->y, composition.crop->width, composition.crop->height
					);
				}

				// crop 2: from window
				bitmap = bitmap.crop(0, 0, window.width, window.height);

				current.push_back({timestamp, Duration{}, Bitmap<Rgba>(bitmap)});
			}
		}
	}

	assert(current.size() == 0);
	return result;
}

}  // namespace pgs
