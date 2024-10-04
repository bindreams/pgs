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

struct Segment;

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
};

struct PresentationComposition : SegmentHeader {
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

	static Segment load(SegmentHeader const& header, std::span<uint8_t const> bytes);

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

struct WindowDefinition : SegmentHeader {
	struct Window {
		uint8_t id;
		uint16_t x;
		uint16_t y;
		uint16_t width;
		uint16_t height;
	};

	std::vector<Window> windows;

	static Segment load(SegmentHeader const& header, std::span<uint8_t const> bytes);

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

	static Segment load(SegmentHeader const& header, std::span<uint8_t const> bytes);

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

	static Segment load(SegmentHeader const& header, std::span<uint8_t const> bytes);

private:
	friend struct Segment;

	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 11;
		return main_size + data.size();
	}
};

struct EndOfDisplaySet : SegmentHeader {
	static Segment load(SegmentHeader const& header, std::span<uint8_t const> bytes);

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

struct Segment {
	static_assert(std::derived_from<PresentationComposition, SegmentHeader>);
	static_assert(std::derived_from<WindowDefinition, SegmentHeader>);
	static_assert(std::derived_from<PaletteDefinition, SegmentHeader>);
	static_assert(std::derived_from<ObjectDefinition, SegmentHeader>);
	static_assert(std::derived_from<EndOfDisplaySet, SegmentHeader>);

	template<typename T>
		requires OneOfSegmentTypes<T> or std::same_as<T, SegmentHeader>
	T const& as() const {
		if constexpr (std::same_as<T, SegmentHeader>) {
			return static_cast<SegmentHeader const&>(*this);
		} else {
			if (header.type != T::StaticType) throw std::bad_variant_access{};
			return const_cast<Segment&>(*this).unsafe_as<T>();
		}
	}

	template<typename T>
		requires OneOfSegmentTypes<T> or std::same_as<T, SegmentHeader>
	T& as() {
		return const_cast<T&>(std::as_const(*this).as<T>());
	}

	template<typename F>
	decltype(auto) visit(F&& callable) const {
		// After asserting that each class in union was derived from SegmentHeader, we can use type punning to check
		// the header type and then use the actual stored value.
		// An additional assert in each branch checks that the actual SegmentHeader base subobject had the same address,
		// i.e. SegmentHeader was the FIRST base in memory layout.
		// Type punning is undefined behavior, but I can't do it any other way without introducing overhead.
		switch (header.type) {
			case SegmentHeader::Type::PresentationComposition:
				assert(static_cast<SegmentHeader const*>(&pcs) == &pcs);
				return callable(pcs);
			case SegmentHeader::Type::WindowDefinition:
				assert(static_cast<SegmentHeader const*>(&wds) == &wds);
				return callable(wds);
			case SegmentHeader::Type::PaletteDefinition:
				assert(static_cast<SegmentHeader const*>(&pds) == &pds);
				return callable(pds);
			case SegmentHeader::Type::ObjectDefinition:
				assert(static_cast<SegmentHeader const*>(&ods) == &ods);
				return callable(ods);
			case SegmentHeader::Type::EndOfDisplaySet:
				assert(static_cast<SegmentHeader const*>(&eods) == &eods);
				return callable(eods);
			default:
				assert(false);  // unhandled segment type
				std::unreachable();
		}
	}

	template<typename F>
	decltype(auto) visit(F&& callable) {
		return std::as_const(*this).visit([&](auto& self) -> decltype(auto) {
			return callable(const_cast<std::remove_const_t<decltype(self)>>(self));
		});
	}

	uint16_t size_bytes() const {
		auto result = visit([](auto& self) { return self.size_bytes_impl(); });
		assert(result <= std::numeric_limits<uint16_t>::max());
		return static_cast<uint16_t>(result);
	}

	operator SegmentHeader&() { return header; }
	operator SegmentHeader const&() const { return header; }

	Segment() : Segment(EndOfDisplaySet{}) {}

	template<OneOfSegmentTypes T>
	explicit Segment(T const& segment) {
		std::construct_at(&as<T>(), segment);
	}

	template<OneOfSegmentTypes T>
	explicit Segment(T&& segment) {
		std::construct_at(&as<T>(), std::move(segment));
	}

	Segment(Segment const& other) {
		other.visit([&](auto& other_self) {
			std::construct_at(&as<std::remove_cvref_t<decltype(other_self)>>(), other_self);
		});
	}

	Segment(Segment&& other) {
		other.visit([&](auto& other_self) {
			std::construct_at(&as<std::remove_cvref_t<decltype(other_self)>>(), std::move(other_self));
		});
	}

	Segment& operator=(Segment const& other) {
		visit([](auto& self) { std::destroy_at(&self); });
		other.visit([&](auto& other_self) {
			std::construct_at(&as<std::remove_cvref_t<decltype(other_self)>>(), other_self);
		});
	}

	Segment& operator=(Segment&& other) {
		visit([](auto& self) { std::destroy_at(&self); });
		other.visit([&](auto& other_self) {
			std::construct_at(&as<std::remove_cvref_t<decltype(other_self)>>(), std::move(other_self));
		});
	}

	~Segment() {
		visit([](auto& self) { std::destroy_at(&self); });
	}

	template<OneOfSegmentTypes T, typename... Ts>
	static Segment make(Ts&&... args) {
		return Segment(ConstructTag<T>{}, std::forward<Ts>(args)...);
	}

private:
	template<OneOfSegmentTypes T>
	struct ConstructTag {
		using Type = T;
	};

	template<typename Tag, typename... Ts>
	explicit Segment(Tag tag, Ts&&... args) {
		std::construct_at(&as<Tag::Type>(), std::forward<Ts>(args)...);
	}

	template<OneOfSegmentTypes T>
	T& unsafe_as() {
		if constexpr (std::same_as<T, PresentationComposition>) return pcs;
		else if constexpr (std::same_as<T, WindowDefinition>) return wds;
		else if constexpr (std::same_as<T, PaletteDefinition>) return pds;
		else if constexpr (std::same_as<T, ObjectDefinition>) return ods;
		else if constexpr (std::same_as<T, EndOfDisplaySet>) return eods;
		else static_assert(false);
	}

	union {
		SegmentHeader header;
		PresentationComposition pcs;
		WindowDefinition wds;
		PaletteDefinition pds;
		ObjectDefinition ods;
		EndOfDisplaySet eods;
	};
};

struct serialize::meta<Segment> {
	void load(std::istream& is, Segment& value, std::endian endianness) {
		char magic_bytes[2] = {};
		SegmentHeader header = {};
		uint16_t segment_size = 0;

		std::streamoff pos = is.tellg();
		serialize::load(
			is, std::tie(magic_bytes, header.presentation_ts, header.decoding_ts, header.type, segment_size)
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

		switch (header.type) {
			case SegmentHeader::Type::PaletteDefinition:
				value = PaletteDefinition::load(header, body);
				break;
			case SegmentHeader::Type::ObjectDefinition:
				value = ObjectDefinition::load(header, body);
				break;
			case SegmentHeader::Type::PresentationComposition:
				value = PresentationComposition::load(header, body);
				break;
			case SegmentHeader::Type::WindowDefinition:
				value = WindowDefinition::load(header, body);
				break;
			case SegmentHeader::Type::EndOfDisplaySet:
				value = EndOfDisplaySet::load(header, body);
				break;
			default:
				throw PgsReadError(std::format("unrecognized segment type {:#x}", std::to_underlying(header.type)));
		}
	}
};

Segment PresentationComposition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result = Segment::make<PresentationComposition>();
	auto& obj = result.as<PresentationComposition>();
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

Segment WindowDefinition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result = Segment::make<WindowDefinition>();
	auto& obj = result.as<WindowDefinition>();
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	auto n_windows = serialize::load<uint8_t>(stream);
	obj.windows.resize(n_windows);
	for (auto& window : obj.windows) serialize::load(stream, window);

	return result;
}

Segment PaletteDefinition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result = Segment::make<PaletteDefinition>();
	auto& obj = result.as<PaletteDefinition>();
	static_cast<SegmentHeader&>(obj) = header;

	auto stream = std::basic_ispanstream<uint8_t>{bytes};

	serialize::load(stream, std::tie(obj.id, obj.version));
	std::size_t n_entries = (bytes.size() - static_cast<std::size_t>(stream.tellg())) / serialize::SizeBytes<Entry>;

	obj.entries.resize(n_entries);
	for (auto& entry : obj.entries) serialize::load(stream, entry);

	return result;
}

Segment ObjectDefinition::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result = Segment::make<ObjectDefinition>();
	auto& obj = result.as<ObjectDefinition>();
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

Segment EndOfDisplaySet::load(SegmentHeader const& header, std::span<uint8_t const> bytes) {
	Segment result = Segment::make<EndOfDisplaySet>();
	auto& obj = result.as<EndOfDisplaySet>();
	static_cast<SegmentHeader&>(obj) = header;

	assert(bytes.size() == 0);
	(void)bytes;
	return result;
}
