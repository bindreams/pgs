#pragma once
#include <cassert>
#include <chrono>
#include <cstdio>
#include <format>
#include <limits>
#include <spanstream>

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

struct PresentationComposition;
struct WindowDefinition;
struct PaletteDefinition;
struct ObjectDefinition;
struct EndOfDisplaySet;

struct SegmentHeader {
	Timestamp presentation_ts;
	Timestamp decoding_ts;
	enum class Type : uint8_t {
		PaletteDefinition = 0x14,
		ObjectDefinition = 0x15,
		PresentationComposition = 0x16,
		WindowDefinition = 0x17,
		EndOfDisplaySet = 0x80,

	} type;
	// uint16_t size;
};

struct Segment : SegmentHeader {
	template<std::derived_from<Segment> T>
	T const& as() const {
		assert(dynamic_cast<T const*>(this) != nullptr);  // casting failed (wrong type)
		assert(dynamic_cast<T const*>(this) == this);     // derived type address is offset from base
		return *static_cast<T const*>(this);
	}

	template<std::derived_from<Segment> T>
	T& as() {
		return const_cast<T&>(std::as_const(*this).as<T>());
	}

	// template<typename F>
	// decltype(auto) visit(F&& callable) const {
	// 	switch (type) {
	// 		case Type::PaletteDefinition:
	// 			return callable(static_cast<PaletteDefinition const&>(*this));
	// 		case Type::ObjectDefinition:
	// 			return callable(static_cast<ObjectDefinition const&>(*this));
	// 		case Type::PresentationComposition:
	// 			return callable(static_cast<PresentationComposition const&>(*this));
	// 		case Type::WindowDefinition:
	// 			return callable(static_cast<WindowDefinition const&>(*this));
	// 		case Type::EndOfDisplaySet:
	// 			return callable(static_cast<EndOfDisplaySet const&>(*this));
	// 		default:
	// 			assert(false);  // Unhandled Segment::Type
	// 			unreachable();
	// 	}
	// }

	// template<typename F>
	// decltype(auto) visit(F&& callable) {
	// 	return std::as_const(*this).visit([&callable](auto& self) {
	// 		return callable(const_cast<std::remove_const_t<decltype(self)>>);
	// 	});
	// }

	uint16_t size_bytes() const {
		auto result = size_bytes_impl();
		assert(result <= std::numeric_limits<uint16_t>::max());
		return static_cast<uint16_t>(result);
	}

	virtual ~Segment() = default;

protected:
	virtual size_t size_bytes_impl() const = 0;
};

struct PresentationComposition : Segment {
	uint16_t width;
	uint16_t height;
	uint8_t framerate;
	uint16_t number;
	enum class State : uint8_t {
		Normal = 0x0,
		AcquisitionPoint = 0x40,
		EpochStart = 0x80,
	} state;
	enum class PaletteUpdateFlag : uint8_t {
		False = 0x0,
		True = 0x80,
	} palette_update;
	uint8_t palette_id;
	// uint8_t n_composition_objects;

	struct CompositionObject {
		uint16_t id;
		uint8_t window_id;
		enum class CroppedFlag : uint8_t {
			False = 0x0,
			True = 0x40,
		} cropped;
		uint16_t x;
		uint16_t y;

		struct Crop {
			uint16_t x;
			uint16_t y;
			uint16_t width;
			uint16_t height;
		};
		std::optional<Crop> crop;
	};

	std::vector<CompositionObject> composition_objects;

	static std::unique_ptr<PresentationComposition> load(SegmentHeader const& header, std::span<uint8_t const> bytes);

protected:
	size_t size_bytes_impl() const override {
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
	enum class CroppedFlag : uint8_t {
		False = 0x0,
		True = 0x40,
	};

	template<serialize::InputStream S>
	static void load(S& stream, PresentationComposition::CompositionObject& obj, std::endian endianness) {
		CroppedFlag cropped_flag;
		serialize::load(stream, std::tie(obj.id, obj.window_id, cropped_flag, obj.x, obj.y), endianness);
		if (cropped_flag == CroppedFlag::True) {
			obj.crop = PresentationComposition::CompositionObject::Crop{};
			serialize::load(stream, obj.crop.value(), endianness);
		}
	}

	template<serialize::OutputStream S>
	static void dump(S& stream, PresentationComposition::CompositionObject const& obj, std::endian endianness) {
		CroppedFlag cropped_flag = CroppedFlag::False;
		if (obj.crop.has_value()) cropped_flag = CroppedFlag::True;

		serialize::dump(stream, std::tie(obj.id, obj.window_id, cropped_flag, obj.x, obj.y), endianness);
		if (cropped_flag == CroppedFlag::True) {
			serialize::dump(stream, obj.crop.value(), endianness);
		}
	}
};

std::unique_ptr<PresentationComposition>
PresentationComposition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	auto result = std::make_unique<PresentationComposition>();
	auto& obj = *result;
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

struct WindowDefinition : Segment {
	struct Window {
		uint8_t id;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
	};

	std::vector<Window> windows;

	static std::unique_ptr<WindowDefinition> load(SegmentHeader const& header, std::span<uint8_t const> bytes);

protected:
	size_t size_bytes_impl() const override {
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

std::unique_ptr<WindowDefinition> WindowDefinition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	auto result = std::make_unique<WindowDefinition>();
	auto& obj = *result;
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	auto n_windows = serialize::load<uint8_t>(stream);
	obj.windows.resize(n_windows);
	for (auto& window : obj.windows) serialize::load(stream, window);

	return result;
}

struct PaletteDefinition : Segment {
	uint8_t id;
	uint8_t version;

	struct Entry {
		uint8_t id;
		uint8_t y;      /// Luminance
		uint8_t cr;     /// Color Difference Red
		uint8_t cb;     /// Color Difference Blue
		uint8_t alpha;  /// Transparency
	};

	std::vector<Entry> entries;

	static std::unique_ptr<PaletteDefinition> load(SegmentHeader const& header, std::span<uint8_t const> bytes);

protected:
	size_t size_bytes_impl() const override {
		constexpr const uint16_t main_size = 2;
		constexpr const uint16_t entry_size = 5;
		return main_size + (entry_size * entries.size());
	}
};

template<>
struct serialize::meta<PaletteDefinition::Entry> {
	static constexpr auto proxy = [](auto& val) { return std::tie(val.id, val.y, val.cr, val.cb, val.alpha); };
};

std::unique_ptr<PaletteDefinition>
PaletteDefinition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	auto result = std::make_unique<PaletteDefinition>();
	auto& obj = *result;
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	serialize::load(stream, std::tie(obj.id, obj.version));
	std::size_t n_entries = (bytes.size() - static_cast<std::size_t>(stream.tellg())) / serialize::SizeBytes<Entry>;

	obj.entries.resize(n_entries);
	for (auto& entry : obj.entries) serialize::load(stream, entry);

	return result;
}

struct ObjectDefinition : Segment {
	uint16_t id;
	uint8_t version;
	enum class SequenceFlag : uint8_t {
		None = 0x0,
		Last = 0x40,
		First = 0x80,
	} sequence_flag;
	// uint48_t object_size;
	uint16_t width;
	uint16_t height;
	std::vector<uint8_t> data;

	static std::unique_ptr<ObjectDefinition> load(SegmentHeader const& header, std::span<uint8_t const> bytes);

protected:
	size_t size_bytes_impl() const override {
		constexpr const uint16_t main_size = 11;
		return main_size + data.size();
	}
};

std::unique_ptr<ObjectDefinition> ObjectDefinition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	auto result = std::make_unique<ObjectDefinition>();
	auto& obj = *result;
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	uint64_t data_size = 0;
	auto data_size_ = serialize::resized<3>(data_size);

	serialize::load(stream, std::tie(obj.id, obj.version, obj.sequence_flag, data_size_));
	if (std::to_underlying(obj.sequence_flag) & std::to_underlying(SequenceFlag::First)) {
		serialize::load(stream, std::tie(obj.width, obj.height));
		data_size -= serialize::SizeBytes<decltype(std::tie(obj.width, obj.height))>;
	} else {
		obj.width = 0;
		obj.height = 0;
	}

	if (data_size != bytes.size() - static_cast<std::size_t>(stream.tellg()))
		throw PgsReadError(std::format(
			"object definition data length ({}) does not match the remaining size of the segment ({})",
			data_size,
			bytes.size() - static_cast<std::size_t>(stream.tellg())
		));

	obj.data = serialize::read_bytes(stream, data_size);
	return result;
}

struct EndOfDisplaySet : Segment {
	static std::unique_ptr<EndOfDisplaySet> load(SegmentHeader const& header, std::span<uint8_t const> bytes);

protected:
	size_t size_bytes_impl() const override { return 0; }
};

std::unique_ptr<EndOfDisplaySet> EndOfDisplaySet::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	auto result = std::make_unique<EndOfDisplaySet>();
	auto& obj = *result;
	static_cast<SegmentHeader&>(obj) = header;

	assert(bytes.size() == 0);
	(void)bytes;
	return result;
}

std::unique_ptr<Segment> load(std::istream& is) {
	char magic_bytes[2] = {};
	SegmentHeader header = {};
	uint16_t segment_size = 0;

	std::streamoff pos = is.tellg();
	serialize::load(is, std::tie(magic_bytes, header.presentation_ts, header.decoding_ts, header.type, segment_size));

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
			"failed to read {} bytes of a segment body, starting at tellg() == {}: {}", segment_size, pos, err.what()
		));
	}

	switch (header.type) {
		case Segment::Type::PaletteDefinition:
			return PaletteDefinition::load(header, body);
		case Segment::Type::ObjectDefinition:
			return ObjectDefinition::load(header, body);
		case Segment::Type::PresentationComposition:
			return PresentationComposition::load(header, body);
		case Segment::Type::WindowDefinition:
			return WindowDefinition::load(header, body);
		case Segment::Type::EndOfDisplaySet:
			return EndOfDisplaySet::load(header, body);
		default:
			throw PgsReadError(std::format("unrecognized segment type {:#x}", std::to_underlying(header.type)));
	}
}
