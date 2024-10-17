#pragma once
#include <cstddef>
#include <cstdint>

namespace pgs {

template<typename T>
struct BitmapView {
	BitmapView(uint16_t width, uint16_t height, uint16_t row_stride, T* begin)
		: m_data(begin), m_width(width), m_height(height), m_row_stride(row_stride) {}

	size_t size() const { return width() * height(); }

	uint16_t width() const { return m_width; }
	uint16_t height() const { return m_height; }

	BitmapView crop(uint16_t x, uint16_t y, uint16_t width, uint16_t height) const {
		assert(x + width <= m_width);
		assert(y + height <= m_height);

		return BitmapView{width, height, m_row_stride, &(*this)[x, y]};
	}

	template<std::constructible_from<T> U>
	void assign(BitmapView<U> other) const {
		using namespace std::ranges;
		auto crop_width = std::min(width(), other.width());
		auto crop_height = std::min(height(), other.height());

		auto lhs = this->crop(0, 0, crop_width, crop_height);
		auto rhs = other.crop(0, 0, crop_width, crop_height);

		if (lhs.size() == 0) return;

		if (&rhs[0, 0] < &lhs[0, 0] and &lhs[0, 0] < &rhs[rhs.width() - 1, rhs.height() - 1] + 1) {
			// target memory starts in the middle of source memory. Copy backwards
			for (int32_t y = crop_height - 1; y >= 0; --y) {
				copy_backward(
					std::span{rhs.m_data + y * rhs.m_row_stride, rhs.m_width}, lhs.m_data + y * lhs.m_row_stride
				);
			}
		} else {
			for (uint16_t y = 0; y < crop_height; ++y) {
				copy(std::span{rhs.m_data + y * rhs.m_row_stride, rhs.m_width}, lhs.m_data + y * lhs.m_row_stride);
			}
		}
	}

	T& operator[](uint16_t x, uint16_t y) const {
		assert(x < width());
		assert(y < height());

		return m_data[y * m_row_stride + x];
	}

	std::span<T> row(uint16_t y) const { return {&(*this)[0, y], m_width}; }

	template<typename U>
	friend struct BitmapView;

private:
	T* m_data;
	uint16_t m_width;
	uint16_t m_height;
	uint16_t m_row_stride;
};

template<typename T>
struct Bitmap {
	Bitmap(uint16_t width, uint16_t height) : m_data(static_cast<size_t>(width) * height), m_width(width) {}

	Bitmap(uint16_t width, uint16_t height, std::vector<T> data) : m_data(std::move(data)), m_width(width) {
		assert(m_data.size() == static_cast<size_t>(width) * height);
	}

	template<std::constructible_from<T> U>
	Bitmap(BitmapView<U> other) : Bitmap(other.width(), other.height()) {
		assign(other);
	}

	// core methods ----------------------------------------------------------------------------------------------------
	T* data() { return m_data.data(); }
	T const* data() const { return m_data.data(); }

	size_t size() const { return m_data.size(); }
	uint16_t width() const { return m_width; }
	uint16_t height() const { return size() / m_width; }

	// bitmap-native methods -------------------------------------------------------------------------------------------
	auto begin() { return m_data.data(); }
	auto begin() const { return m_data.data(); }
	auto end() { return m_data.data() + m_data.size(); }
	auto end() const { return m_data.data() + m_data.size(); }

	BitmapView<T> view() { return BitmapView<T>{width(), height(), width(), data()}; }
	BitmapView<T const> view() const { return BitmapView<T const>{width(), height(), width(), data()}; }
	operator BitmapView<T>() { return view(); }
	operator BitmapView<T>() const { return view(); }

	// view-like methods -----------------------------------------------------------------------------------------------
	template<std::constructible_from<T> U>
	void assign(BitmapView<U> other) {
		view().assign(other);
	}

	BitmapView<T> crop(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
		return view().crop(x, y, width, height);
	}

	BitmapView<T const> crop(uint16_t x, uint16_t y, uint16_t width, uint16_t height) const {
		return view().crop(x, y, width, height);
	}

	T& operator[](uint16_t x, uint16_t y) { return view()[x, y]; }
	T const& operator[](uint16_t x, uint16_t y) const { return view()[x, y]; }

	std::span<T> row(uint16_t y) { return view().row(y); }
	std::span<T const> row(uint16_t y) const { return view().row(y); }

private:
	std::vector<T> m_data;
	uint16_t m_width;
};

using ByteBitmap = Bitmap<uint8_t>;
using ByteBitmapView = BitmapView<uint8_t>;

}  // namespace pgs
