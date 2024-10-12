#include <doctest/doctest.h>
#include <pgs.hpp>

template<typename T, typename U>
void test_color_equivalence(T color1, U color2) {
	CHECK(color1 == pgs::color_cast<T>(color2));
	CHECK(color2 == pgs::color_cast<U>(color1));
}

TEST_CASE("colors.YCbCr_BT709.reference") {
	using namespace pgs::colormodels;
	test_color_equivalence(Rgb{255, 255, 255}, YCbCr_BT709{235, 128, 128});
	test_color_equivalence(Rgb{0, 0, 0}, YCbCr_BT709{16, 128, 128});
}

TEST_CASE("colors.YCbCrA_BT709.reference") {
	// This test just checks that the transparency was not lost.
	using namespace pgs::colormodels;
	test_color_equivalence(Rgba{255, 255, 255, 123}, YCbCrA_BT709{235, 128, 128, 123});
	test_color_equivalence(Rgba{0, 0, 0, 123}, YCbCrA_BT709{16, 128, 128, 123});
}
