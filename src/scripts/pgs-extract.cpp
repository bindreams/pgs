#include <CLI/CLI.hpp>
#include <boost/gil.hpp>
#include <boost/gil/extension/io/png.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/locator.hpp>
#include <boost/gil/step_iterator.hpp>
#include <filesystem>
#include <fstream>
#include <pgs.hpp>

int main_(int argc, char** argv) {
	using namespace std;
	using namespace pgs;
	namespace bg = boost::gil;
	namespace fs = std::filesystem;

	CLI::App app;
	argv = app.ensure_utf8(argv);

	std::string path;
	app.add_option("file", path, ".pub source file");

	try {
		app.parse(argc, argv);
	} catch (const CLI::ParseError& e) {
		return app.exit(e);
	}

	ifstream ifs;
	ifs.exceptions(ios::badbit | ios::failbit);
	ifs.open(path, ios::binary);

	auto segments = serial::loaditer<Segment>(ifs);
	auto subs = subtitles(segments);
	for (auto&& sub : subs) {
		static_assert(sizeof(bg::rgba8_pixel_t) == sizeof(decltype(sub.bitmap)::value_type));

		auto bitmap_view = bg::interleaved_view(
			sub.bitmap.width(),
			sub.bitmap.height(),
			reinterpret_cast<bg::rgba8_pixel_t*>(sub.bitmap.data()),
			sub.bitmap.width() * 4
		);

		auto timestamp = sub.timestamp.time_since_epoch().count();
		auto duration = sub.duration.count();

		bg::write_view(
			std::format("{}.{}-{}.png", fs::path{path}.stem().string(), timestamp, duration), bitmap_view, bg::png_tag{}
		);
	}

	return 0;
}

int main(int argc, char** argv) {
#ifdef NDEBUG
	try {
		return main_(argc, argv);
	} catch (std::runtime_error& e) {
		std::cout << e.what() << "\n";
	}
#else
	return main_(argc, argv);
#endif
}
