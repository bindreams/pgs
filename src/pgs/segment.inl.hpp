#pragma once
#include "segment.hpp"

#include <bit>
#include <iostream>
#include <spanstream>
#include <tuple>
#include <vector>

#include "exceptions.hpp"
#include "serialization.hpp"

namespace pgs {

template<>
struct serial::meta<PresentationComposition::CompositionObject::Crop> {
	static constexpr auto proxy = [](auto& obj) { return std::tie(obj.x, obj.y, obj.width, obj.height); };
};

template<>
struct serial::meta<PresentationComposition::CompositionObject> {
	template<serial::InputStream S>
	static void load(S& stream, PresentationComposition::CompositionObject& obj, std::endian endianness) {
		using CroppedFlag = PresentationComposition::CompositionObject::CroppedFlag;

		CroppedFlag cropped_flag;
		serial::load(stream, std::tie(obj.object_id, obj.window_id, cropped_flag, obj.x, obj.y), endianness);

		if (cropped_flag == CroppedFlag::True) {
			if (stream.peek() == EOF) {
				// This happens on some subtitle tracks, notably tests/data/sample-1.sup, which is a cutout from
				// The Godfather.
				std::cerr
					<< "Warning: subtitle segment has cropped flag set but does not contain the crop information\n";
			} else {
				obj.crop = PresentationComposition::CompositionObject::Crop{};
				serial::load(stream, obj.crop.value(), endianness);
			}
		}
	}

	template<serial::OutputStream S>
	static void dump(S& stream, PresentationComposition::CompositionObject const& obj, std::endian endianness) {
		using CroppedFlag = PresentationComposition::CompositionObject::CroppedFlag;

		CroppedFlag cropped_flag = CroppedFlag::False;
		if (obj.crop.has_value()) cropped_flag = CroppedFlag::True;

		serial::dump(stream, std::tie(obj.object_id, obj.window_id, cropped_flag, obj.x, obj.y), endianness);
		if (cropped_flag == CroppedFlag::True) {
			serial::dump(stream, obj.crop.value(), endianness);
		}
	}
};

template<>
struct serial::meta<WindowDefinition::Window> {
	static constexpr auto proxy = [](auto&& window) {
		return std::tie(window.id, window.x, window.y, window.width, window.height);
	};
};

template<>
struct serial::meta<PaletteDefinition::Entry> {
	static constexpr auto proxy = [](auto& val) { return std::tie(val.id, val.y, val.cr, val.cb, val.alpha); };
};

template<>
struct serial::meta<Segment> {
	static void load(std::istream& is, Segment& value, std::endian endianness) {
		assert(endianness == std::endian::big);  // PGS format only supports big-endian
		(void)endianness;

		char magic_bytes[2] = {};
		SegmentHeader header = {};

		Segment::Type segment_type = Segment::Type::EndOfDisplaySet;
		uint16_t segment_size = 0;

		std::streamoff pos = is.tellg();
		serial::load(is, std::tie(magic_bytes, header.presentation_ts, header.decoding_ts, segment_type, segment_size));

		std::string_view magic{magic_bytes, 2};
		if (magic != "PG") {
			throw PgsReadError(std::format(
				"failed to read segment header at tellg() == {}: got {:#x} {:#x} instead of magic {:#x} {:#x} (\"PG\")",
				pos,
				magic_bytes[0],
				magic_bytes[1],
				'P',
				'G'
			));
		}

		std::vector<uint8_t> body;
		pos = is.tellg();
		try {
			body = serial::read_bytes(is, segment_size);
		} catch (std::runtime_error const& err) {
			throw PgsReadError(std::format(
				"failed to read {} bytes of a segment body, starting at tellg() == {}: {}",
				segment_size,
				pos,
				err.what()
			));
		}

		switch (segment_type) {
			case Segment::Type::PaletteDefinition:
				value = PaletteDefinition::load_body(header, body);
				break;
			case Segment::Type::ObjectDefinition:
				value = ObjectDefinition::load_body(header, body);
				break;
			case Segment::Type::PresentationComposition:
				value = PresentationComposition::load_body(header, body);
				break;
			case Segment::Type::WindowDefinition:
				value = WindowDefinition::load_body(header, body);
				break;
			case Segment::Type::EndOfDisplaySet:
				value = EndOfDisplaySet::load_body(header, body);
				break;
			default:
				throw PgsReadError(std::format("unrecognized segment type {:#x}", std::to_underlying(segment_type)));
		}
	}

	template<serial::OutputStream S>
	static void dump(S& os, Segment const& value, std::endian endianness) {
		assert(endianness == std::endian::big);  // PGS format only supports big-endian
		(void)endianness;

		std::basic_ostringstream<uint8_t> body_stream;
		std::visit([&](auto& self) { self.dump_body(body_stream); }, value);

		char magic_bytes[2] = {'P', 'G'};
		uint16_t segment_size = body_stream.tellp();
		auto segment_type = value.type();

		serial::dump(
			os,
			std::tie(
				magic_bytes, value.header().presentation_ts, value.header().decoding_ts, segment_type, segment_size
			)
		);

		std::vector<uint8_t> body(segment_size);

		serial::write_bytes(os, std::span<uint8_t const>{body_stream.str()});
	}
};

inline Segment PresentationComposition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<PresentationComposition>};
	auto& obj = get<PresentationComposition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	uint8_t n_composition_objects = 0;
	serial::load(
		stream,
		std::tie(
			obj.width,
			obj.height,
			obj.framerate,
			obj.number,
			obj.state,
			obj.palette_update,
			obj.palette_id,
			n_composition_objects
		)
	);

	obj.composition_objects.resize(n_composition_objects);
	for (auto& composition_object : obj.composition_objects) serial::load(stream, composition_object);

	return result;
}

template<serial::OutputStream S>
inline void PresentationComposition::dump_body(S& stream) const {
	if (composition_objects.size() > std::numeric_limits<uint8_t>::max()) {
		throw PgsWriteError(std::format(
			"PresentationComposition::dump_body: number of composition objects ({}) does not fit into uint8_t",
			composition_objects.size()
		));
	}

	uint8_t n_composition_objects = composition_objects.size();
	serial::dump(
		stream, std::tie(width, height, framerate, number, state, palette_update, palette_id, n_composition_objects)
	);

	for (auto& composition_object : composition_objects) serial::dump(stream, composition_object);
}

inline Segment WindowDefinition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<WindowDefinition>};
	auto& obj = get<WindowDefinition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	auto n_windows = serial::load<uint8_t>(stream);
	obj.windows.resize(n_windows);
	for (auto& window : obj.windows) serial::load(stream, window);

	return result;
}

template<serial::OutputStream S>
inline void WindowDefinition::dump_body(S& stream) const {
	if (windows.size() > std::numeric_limits<uint8_t>::max()) {
		throw PgsWriteError(std::format(
			"WindowDefinition::dump_body: number of window objects ({}) does not fit into uint8_t", windows.size()
		));
	}

	serial::dump(stream, static_cast<uint8_t>(windows.size()));
	for (auto& window : windows) serial::dump(stream, window);
}

inline Segment PaletteDefinition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<PaletteDefinition>};
	auto& obj = get<PaletteDefinition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	serial::load(stream, std::tie(obj.id, obj.version));
	std::size_t n_entries = (bytes.size() - static_cast<std::size_t>(stream.tellg())) / serial::SizeBytes<Entry>;

	obj.entries.resize(n_entries);
	for (auto& entry : obj.entries) serial::load(stream, entry);

	return result;
}

template<serial::OutputStream S>
inline void PaletteDefinition::dump_body(S& stream) const {
	serial::dump(stream, std::tie(id, version));
	for (auto& entry : entries) serial::dump(stream, entry);
}

inline Segment ObjectDefinition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<ObjectDefinition>};
	auto& obj = get<ObjectDefinition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	uint64_t data_size = 0;
	auto data_size_ = serial::resized<3>(data_size);

	serial::load(stream, std::tie(obj.id, obj.version, obj.sequence_flag, data_size_));

	if (data_size != bytes.size() - static_cast<std::size_t>(stream.tellg()))
		throw PgsReadError(std::format(
			"object definition data length ({}) does not match the remaining size of the segment ({})",
			data_size,
			bytes.size() - static_cast<std::size_t>(stream.tellg())
		));

	obj.data = serial::read_bytes(stream, data_size);
	return result;
}

template<serial::OutputStream S>
inline void ObjectDefinition::dump_body(S& stream) const {
	uint64_t data_size = data.size();
	auto data_size_ = serial::resized<3>(data_size);

	serial::dump(stream, std::tie(id, version, sequence_flag, data_size_));
	serial::write_bytes(stream, data);
}

inline Segment EndOfDisplaySet::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<EndOfDisplaySet>};
	auto& obj = get<EndOfDisplaySet>(result);
	static_cast<SegmentHeader&>(obj) = header;

	assert(bytes.size() == 0);
	(void)bytes;
	return result;
}

template<serial::OutputStream S>
inline void EndOfDisplaySet::dump_body(S& stream) const {
	(void)stream;
}

}  // namespace pgs
