#pragma once
#include <cassert>
#include <chrono>
#include <cstdint>
#include <variant>

#include "serialization.hpp"
#include "utility.hpp"

namespace pgs {

/// PGS stores timestamps as 4-byte integers with 90kHz precision.
using Duration = std::chrono::duration<uint32_t, std::ratio<1, 90000>>;
using Timestamp = std::chrono::time_point<std::chrono::steady_clock, Duration>;

struct Segment;

struct SegmentHeader {
	Timestamp presentation_ts;
	Timestamp decoding_ts;
};

struct PresentationComposition : SegmentHeader {
	uint16_t width = 0;
	uint16_t height = 0;
	uint8_t framerate = 0x10;
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

	template<serial::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;

	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 11;
		constexpr const uint16_t object_size = 16;
		return main_size + (object_size * composition_objects.size());
	}
};

DEFINE_ENUM_BITWISE_OPERATORS(PresentationComposition::State);

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
	template<serial::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;

	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 1;
		constexpr const uint16_t window_size = 9;
		return main_size + (window_size * windows.size());
	}
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
	template<serial::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;
	size_t size_bytes_impl() const {
		constexpr const uint16_t main_size = 2;
		constexpr const uint16_t entry_size = 5;
		return main_size + (entry_size * entries.size());
	}
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
	template<serial::OutputStream S>
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
	template<serial::OutputStream S>
	void dump_body(S& stream) const;

private:
	friend struct Segment;

	size_t size_bytes_impl() const { return 0; }
};

DEFINE_ENUM_BITWISE_OPERATORS(ObjectDefinition::SequenceFlag);

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
		unreachable();
	}
};

template<>
struct serial::meta<Segment>;

template<>
struct serial::meta<PaletteDefinition::Entry>;

template<>
struct serial::meta<PresentationComposition::CompositionObject::Crop>;

template<>
struct serial::meta<PresentationComposition::CompositionObject>;

template<>
struct serial::meta<WindowDefinition::Window>;

}  // namespace pgs

#include "segment.inl.hpp"
