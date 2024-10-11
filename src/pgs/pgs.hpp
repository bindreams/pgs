#pragma once
#include <cassert>
#include <chrono>
#include <cstdio>
#include <format>
#include <limits>
#include <spanstream>
#include <variant>

#include "colors.hpp"
#include "rl_encoding.hpp"
#include "serialize.hpp"
#include "utility.hpp"

/// PGS stores timestamps as 4-byte integers with 90kHz precision.
using Duration = std::chrono::duration<uint32_t, std::ratio<1, 90000>>;
using Timestamp = std::chrono::time_point<std::chrono::steady_clock, Duration>;

struct PgsReadError : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct PgsWriteError : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

struct Segment;

struct SegmentHeader {
	Timestamp presentation_ts;
	Timestamp decoding_ts;
};

struct PresentationComposition : SegmentHeader {
	uint16_t width = 0;
	uint16_t height = 0;
	uint8_t framerate = 0;
	uint16_t number = 0;

	enum class State : uint8_t {
		Normal = 0x0,
		AcquisitionPoint = 0x40,
		EpochStart = 0x80,
	} state = State::Normal;

	enum class PaletteUpdateFlag : uint8_t {
		False = 0x0,
		True = 0x80,
	} palette_update = PaletteUpdateFlag::False;
	uint8_t palette_id = 0;

	struct CompositionObject {
		enum class CroppedFlag : uint8_t {
			False = 0x0,
			True = 0x40,
		};

		uint16_t id = 0;
		uint8_t window_id = 0;
		uint16_t x = 0;
		uint16_t y = 0;

		struct Crop {
			uint16_t x = 0;
			uint16_t y = 0;
			uint16_t width = 0;
			uint16_t height = 0;
		};

		// Theoretically, the PGS format allows for multiple crop objects if the cropped flag is set, but does not
		// explain how one should distinguish when the cropped objects list ends and the next composition object begins.
		// I have never seen multiple crop objects in practice.
		std::optional<Crop> crop = std::nullopt;
	};

	std::vector<CompositionObject> composition_objects;

	static Segment load_body(SegmentHeader const& header, std::span<uint8_t const> bytes);

	template<serialize::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;

	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 11;
		constexpr const uint16_t object_size = 16;
		return main_size + (object_size * composition_objects.size());
	}
};

template<>
struct serialize::meta<PresentationComposition::CompositionObject::Crop> {
	static constexpr auto proxy = [](auto& obj) { return std::tie(obj.x, obj.y, obj.width, obj.height); };
};

template<>
struct serialize::meta<PresentationComposition::CompositionObject> {
	template<serialize::InputStream S>
	static void load(S& stream, PresentationComposition::CompositionObject& obj, std::endian endianness) {
		using CroppedFlag = PresentationComposition::CompositionObject::CroppedFlag;

		CroppedFlag cropped_flag;
		serialize::load(stream, std::tie(obj.id, obj.window_id, cropped_flag, obj.x, obj.y), endianness);

		if (cropped_flag == CroppedFlag::True) {
			if (stream.peek() == EOF) {
				// This happens on some subtitle tracks, notably tests/data/sample-1.sup, which is a cutout from
				// The Godfather.
				std::cerr
					<< "Warning: subtitle segment has cropped flag set but does not contain the crop information\n";
			} else {
				obj.crop = PresentationComposition::CompositionObject::Crop{};
				serialize::load(stream, obj.crop.value(), endianness);
			}
		}
	}

	template<serialize::OutputStream S>
	static void dump(S& stream, PresentationComposition::CompositionObject const& obj, std::endian endianness) {
		using CroppedFlag = PresentationComposition::CompositionObject::CroppedFlag;

		CroppedFlag cropped_flag = CroppedFlag::False;
		if (obj.crop.has_value()) cropped_flag = CroppedFlag::True;

		serialize::dump(stream, std::tie(obj.id, obj.window_id, cropped_flag, obj.x, obj.y), endianness);
		if (cropped_flag == CroppedFlag::True) {
			serialize::dump(stream, obj.crop.value(), endianness);
		}
	}
};

struct WindowDefinition : SegmentHeader {
	struct Window {
		uint8_t id = 0;
		uint16_t x = 0;
		uint16_t y = 0;
		uint16_t width = 0;
		uint16_t height = 0;
	};

	std::vector<Window> windows;

	static Segment load_body(SegmentHeader const& header, std::span<uint8_t const> bytes);
	template<serialize::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;

	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 1;
		constexpr const uint16_t window_size = 9;
		return main_size + (window_size * windows.size());
	}
};

template<>
struct serialize::meta<WindowDefinition::Window> {
	static constexpr auto proxy = [](auto&& window) {
		return std::tie(window.id, window.x, window.y, window.width, window.height);
	};
};

struct PaletteDefinition : SegmentHeader {
	uint8_t id = 0;
	uint8_t version = 0;

	struct Entry {
		uint8_t id = 0;
		uint8_t y = 0;      /// Luminance
		uint8_t cr = 0;     /// Color Difference Red
		uint8_t cb = 0;     /// Color Difference Blue
		uint8_t alpha = 0;  /// Transparency
	};

	std::vector<Entry> entries;

	static Segment load_body(SegmentHeader const& header, std::span<uint8_t const> bytes);
	template<serialize::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;
	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 2;
		constexpr const uint16_t entry_size = 5;
		return main_size + (entry_size * entries.size());
	}
};

template<>
struct serialize::meta<PaletteDefinition::Entry> {
	static constexpr auto proxy = [](auto& val) { return std::tie(val.id, val.y, val.cr, val.cb, val.alpha); };
};

struct ObjectDefinition : SegmentHeader {
	uint16_t id = 0;
	uint8_t version = 0;

	enum class SequenceFlag : uint8_t {
		None = 0x0,
		Last = 0x40,
		First = 0x80,
	} sequence_flag = SequenceFlag::None;

	std::vector<uint8_t> data;

	static Segment load_body(SegmentHeader const& header, std::span<uint8_t const> bytes);
	template<serialize::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;

	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 11;
		return main_size + data.size();
	}
};

struct EndOfDisplaySet : SegmentHeader {
	static Segment load_body(SegmentHeader const& header, std::span<uint8_t const> bytes);
	template<serialize::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;

	size_t size_bytes_impl() const { return 0; }
};

struct PresentationComposition;
struct WindowDefinition;
struct PaletteDefinition;
struct ObjectDefinition;
struct EndOfDisplaySet;

template<typename T, typename... U>
concept OneOf = (std::same_as<T, U> or ...);

template<typename T>
concept OneOfSegmentTypes =
	OneOf<T, PresentationComposition, WindowDefinition, PaletteDefinition, ObjectDefinition, EndOfDisplaySet>;

struct Segment
	: public std::
		  variant<EndOfDisplaySet, PresentationComposition, WindowDefinition, PaletteDefinition, ObjectDefinition> {
	using BaseType =
		std::variant<EndOfDisplaySet, PresentationComposition, WindowDefinition, PaletteDefinition, ObjectDefinition>;
	using BaseType::BaseType;

	enum class Type : uint8_t {
		PaletteDefinition = 0x14,
		ObjectDefinition = 0x15,
		PresentationComposition = 0x16,
		WindowDefinition = 0x17,
		EndOfDisplaySet = 0x80,

	};

	SegmentHeader& header() {
		return std::visit([](auto& self) -> SegmentHeader& { return self; }, *this);
	}

	SegmentHeader const& header() const {
		return std::visit([](auto& self) -> SegmentHeader const& { return self; }, *this);
	}

	uint16_t size_bytes() const {
		auto result = std::visit([](auto& self) { return self.size_bytes_impl(); }, *this);
		assert(result <= std::numeric_limits<uint16_t>::max());
		return static_cast<uint16_t>(result);
	}

	Type type() const noexcept {
		if (std::holds_alternative<PresentationComposition>(*this)) return Type::PresentationComposition;
		if (std::holds_alternative<WindowDefinition>(*this)) return Type::WindowDefinition;
		if (std::holds_alternative<PaletteDefinition>(*this)) return Type::PaletteDefinition;
		if (std::holds_alternative<ObjectDefinition>(*this)) return Type::ObjectDefinition;
		if (std::holds_alternative<EndOfDisplaySet>(*this)) return Type::EndOfDisplaySet;

		assert(false);  // unhandled variant alternative
		std::unreachable();
	}
};

template<>
struct serialize::meta<Segment> {
	static void load(std::istream& is, Segment& value, std::endian endianness) {
		assert(endianness == std::endian::big);  // PGS format only supports big-endian
		(void)endianness;

		char magic_bytes[2] = {};
		SegmentHeader header = {};

		Segment::Type segment_type = Segment::Type::EndOfDisplaySet;
		uint16_t segment_size = 0;

		std::streamoff pos = is.tellg();
		serialize::load(
			is, std::tie(magic_bytes, header.presentation_ts, header.decoding_ts, segment_type, segment_size)
		);

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
			body = serialize::read_bytes(is, segment_size);
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

	static void dump(std::ostream& os, Segment const& value, std::endian endianness) {
		assert(endianness == std::endian::big);  // PGS format only supports big-endian
		(void)endianness;

		std::basic_ostringstream<uint8_t> body_stream;
		std::visit([&](auto& self) { self.dump_body(body_stream); }, value);

		char magic_bytes[2] = {'P', 'G'};
		std::uint16_t segment_size = body_stream.tellp();
		auto segment_type = value.type();

		serialize::dump(
			os,
			std::tie(
				magic_bytes, value.header().presentation_ts, value.header().decoding_ts, segment_type, segment_size
			)
		);

		std::vector<uint8_t> body(segment_size);

		serialize::write_bytes(os, std::span<uint8_t const>{body_stream.str()});
	}
};

inline Segment PresentationComposition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<PresentationComposition>};
	auto& obj = get<PresentationComposition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	uint8_t n_composition_objects = 0;
	serialize::load(
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
	for (auto& composition_object : obj.composition_objects) serialize::load(stream, composition_object);

	return result;
}

template<serialize::OutputStream S>
inline void PresentationComposition::dump_body(S& stream) const {
	if (composition_objects.size() > std::numeric_limits<uint8_t>::max()) {
		throw PgsWriteError(std::format(
			"PresentationComposition::dump_body: number of composition objects ({}) does not fit into uint8_t",
			composition_objects.size()
		));
	}

	uint8_t n_composition_objects = composition_objects.size();
	serialize::dump(
		stream, std::tie(width, height, framerate, number, state, palette_update, palette_id, n_composition_objects)
	);

	for (auto& composition_object : composition_objects) serialize::dump(stream, composition_object);
}

inline Segment WindowDefinition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<WindowDefinition>};
	auto& obj = get<WindowDefinition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	auto n_windows = serialize::load<uint8_t>(stream);
	obj.windows.resize(n_windows);
	for (auto& window : obj.windows) serialize::load(stream, window);

	return result;
}

template<serialize::OutputStream S>
inline void WindowDefinition::dump_body(S& stream) const {
	if (windows.size() > std::numeric_limits<uint8_t>::max()) {
		throw PgsWriteError(std::format(
			"WindowDefinition::dump_body: number of window objects ({}) does not fit into uint8_t", windows.size()
		));
	}

	serialize::dump(stream, static_cast<uint8_t>(windows.size()));
	for (auto& window : windows) serialize::dump(stream, window);
}

inline Segment PaletteDefinition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<PaletteDefinition>};
	auto& obj = get<PaletteDefinition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	serialize::load(stream, std::tie(obj.id, obj.version));
	std::size_t n_entries = (bytes.size() - static_cast<std::size_t>(stream.tellg())) / serialize::SizeBytes<Entry>;

	obj.entries.resize(n_entries);
	for (auto& entry : obj.entries) serialize::load(stream, entry);

	return result;
}

template<serialize::OutputStream S>
inline void PaletteDefinition::dump_body(S& stream) const {
	serialize::dump(stream, std::tie(id, version));
	for (auto& entry : entries) serialize::dump(stream, entry);
}

inline Segment ObjectDefinition::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<ObjectDefinition>};
	auto& obj = get<ObjectDefinition>(result);
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	uint64_t data_size = 0;
	auto data_size_ = serialize::resized<3>(data_size);

	serialize::load(stream, std::tie(obj.id, obj.version, obj.sequence_flag, data_size_));

	if (data_size != bytes.size() - static_cast<std::size_t>(stream.tellg()))
		throw PgsReadError(std::format(
			"object definition data length ({}) does not match the remaining size of the segment ({})",
			data_size,
			bytes.size() - static_cast<std::size_t>(stream.tellg())
		));

	obj.data = serialize::read_bytes(stream, data_size);
	return result;
}

template<serialize::OutputStream S>
inline void ObjectDefinition::dump_body(S& stream) const {
	uint64_t data_size = data.size();
	auto data_size_ = serialize::resized<3>(data_size);

	serialize::dump(stream, std::tie(id, version, sequence_flag, data_size_));
	serialize::write_bytes(stream, data);
}

inline Segment EndOfDisplaySet::load_body(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result{std::in_place_type<EndOfDisplaySet>};
	auto& obj = get<EndOfDisplaySet>(result);
	static_cast<SegmentHeader&>(obj) = header;

	assert(bytes.size() == 0);
	(void)bytes;
	return result;
}

template<serialize::OutputStream S>
inline void EndOfDisplaySet::dump_body(S& stream) const {
	(void)stream;
}
