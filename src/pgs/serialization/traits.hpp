#pragma once
#include <array>
#include <type_traits>

namespace pgs::serial {
// The following block adapted from https://github.com/bindreams/zint-bindings/blob/main/src/zint/src/utility.hpp ======

namespace impl {
/// See BoundedNDArray concept
template<typename T>
inline constexpr bool is_fully_bounded() {
	if constexpr (std::rank_v<T> == 0) return false;  // Not a valid array
	// For example, this condition is true for arrays like int x[][10] = {}, where first extent is deduced to 0.

	if constexpr (!std::is_bounded_array_v<T>) return false;
	if constexpr (std::rank_v<T> > 1) return is_fully_bounded<std::remove_extent_t<T>>();
	return true;
}
}  // namespace impl

/**
 * @brief Satisfied if `T` is an array bounded in all dimensions, such as `int[10][20][30]`.
 *
 * Not satisfied for non-arrays, arrays with an unbounded dimension, and invalid arrays such as the following:
 * ```c++
 * int x[][10] = {};
 * ```
 *
 * The array above has the highest dimension deduced from initialization, but the initializer list is empty and the size
 * is deduced to 0, which is not allowed per C++ rules.
 */
template<typename T>
concept BoundedNDArray = impl::is_fully_bounded<T>();

namespace impl {
template<BoundedNDArray T, std::size_t I = std::rank_v<T> - 1>
inline constexpr std::array<int, std::rank_v<T>> array_shape() {
	if constexpr (I != 0) {
		auto result = array_shape<T, I - 1>();
		result[I] = std::extent_v<T, I>;
		return result;
	} else {
		std::array<int, std::rank_v<T>> result{};
		result[I] = std::extent_v<T, I>;
		return result;
	}
}
}  // namespace impl

/// A std::array containing the extents of a bounded N-dimensional array in order.
template<BoundedNDArray T>
constexpr const std::array<int, std::rank_v<T>> array_shape = impl::array_shape<T>();
// =====================================================================================================================

// clang-format off
template<size_t N>
requires(N == 1 or N == 2 or N == 4 or N == 8)
using uint = std::conditional_t<
	N == 1, uint8_t,
	std::conditional_t<
		N == 2, uint16_t,
		std::conditional_t<
			N == 4, uint32_t,
			uint64_t
		>
	>
>;
// clang-format on

template<typename T>
concept ByteLike = sizeof(T) == 1 and (std::integral<T> or std::is_enum_v<T>);

}  // namespace pgs::serial
