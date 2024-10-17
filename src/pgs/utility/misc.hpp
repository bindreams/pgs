#pragma once
#include <concepts>
#include <utility>
#include <version>

namespace pgs {

template<typename T, typename... U>
concept OneOf = (std::same_as<T, U> or ...);

[[noreturn]] inline void unreachable() {
#if __cpp_lib_unreachable >= 202202L
	std::unreachable();

#elif defined(_MSC_VER) && !defined(__clang__)  // MSVC
	// Uses compiler specific extensions if possible.
	// Even if no extension is used, undefined behavior is still raised by
	// an empty function body and the noreturn attribute.
	__assume(false);

#else  // GCC, Clang
	__builtin_unreachable();

#endif
}

}  // namespace pgs

#define DEFINE_ENUM_BITWISE_OPERATORS(T)                                                                               \
	static_assert(std::is_enum_v<T>, "DEFINE_ENUM_BITWISE_OPERATORS: T must be an enum type");                         \
	constexpr T operator~(T lhs) noexcept {                                                                            \
		return static_cast<T>(~std::to_underlying(lhs));                                                               \
	}                                                                                                                  \
	constexpr T operator|(T lhs, T rhs) noexcept {                                                                     \
		return static_cast<T>(std::to_underlying(lhs) | std::to_underlying(rhs));                                      \
	}                                                                                                                  \
	constexpr T operator&(T lhs, T rhs) noexcept {                                                                     \
		return static_cast<T>(std::to_underlying(lhs) & std::to_underlying(rhs));                                      \
	}                                                                                                                  \
	constexpr T operator^(T lhs, T rhs) noexcept {                                                                     \
		return static_cast<T>(std::to_underlying(lhs) ^ std::to_underlying(rhs));                                      \
	}                                                                                                                  \
	constexpr T& operator|=(T& lhs, T rhs) noexcept {                                                                  \
		return (lhs = lhs | rhs);                                                                                      \
	}                                                                                                                  \
	constexpr T& operator&=(T& lhs, T rhs) noexcept {                                                                  \
		return (lhs = lhs & rhs);                                                                                      \
	}                                                                                                                  \
	constexpr T& operator^=(T& lhs, T rhs) noexcept {                                                                  \
		return (lhs = lhs ^ rhs);                                                                                      \
	}                                                                                                                  \
	static_assert(true)
