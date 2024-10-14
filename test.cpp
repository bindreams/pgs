#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/locator.hpp>
#include <boost/gil/step_iterator.hpp>
#include <fstream>
#include <pgs.hpp>

template<typename T>
void print_bytes(T&& range) {
	for (auto val : range) std::cerr << std::format("{:#x} ", std::bit_cast<uint8_t>(val));
	std::cerr << "\n";
}

void main_() {
	using namespace std;
	using namespace pgs;

	namespace bg = boost::gil;

	ifstream ifs;
	ifs.exceptions(ios::badbit | ios::failbit);
	ifs.open("test.sup", ios::binary);

	vector<Segment> segments;

	while (ifs.peek() != EOF) {
		segments.push_back(serial::load<Segment>(ifs));
	}

	for (auto&& image : subtitles(segments)) {
		static_assert(sizeof(bg::rgba8_pixel_t) == sizeof(image.bitmap[0, 0]));

		auto bitmap_view = bg::interleaved_view(
			image.bitmap.width(),
			image.bitmap.height(),
			reinterpret_cast<bg::rgba8_pixel_t*>(image.bitmap.data()),
			image.bitmap.width() * 4
		);
		bg::write_view(
			std::format("out-{}.png", image.timestamp.time_since_epoch().count()), bitmap_view, bg::png_tag{}
		);
	}
}

int main() {
#ifdef NDEBUG
	try {
		main_();
	} catch (std::runtime_error& e) {
		std::cout << e.what() << "\n";
	}
#else
	main_();
#endif
}
