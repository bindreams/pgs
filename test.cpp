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

	ifstream ifs("tests/data/sample-2.sup", ios::binary);
	ifs.exceptions(ios::failbit | ios::badbit);
	stringstream input;
	input << ifs.rdbuf();
	ifs.close();

	vector<Segment> segments;
	while (input.peek() != EOF) segments.push_back(serial::load<Segment>(input));

	for (auto& segment : segments) {
		if (segment.type() == Segment::Type::ObjectDefinition) {
			auto& ods = std::get<ObjectDefinition>(segment);
			if (to_underlying(ods.sequence_flag) & to_underlying(ObjectDefinition::SequenceFlag::First) and
				to_underlying(ods.sequence_flag) & to_underlying(ObjectDefinition::SequenceFlag::Last)) {
				Bitmap bm = rle::decode(span{ods.data});
				ods.data = rle::encode(bm);
			}
		}
	}

	ostringstream output;
	for (auto& segment : segments) serial::dump(output, segment);
	// =================================================================================================================

	// std::ifstream ifs("tests/data/sample-2.sup", std::ios::binary);
	// ifs.exceptions(std::ios::failbit | std::ios::badbit);

	// std::stringstream ss;
	// ss << ifs.rdbuf();
	// ifs.close();

	// std::vector<uint8_t> source(ss.tellp());
	// std::ranges::transform(ss.str(), source.begin(), [](char ch) { return std::bit_cast<uint8_t>(ch); });

	// std::vector<Segment> segments;
	// size_t i = 0;
	// while (ss.peek() != EOF) {
	// 	std::streamoff start = ss.tellg();
	// 	auto x = serial::load<Segment>(ss);
	// 	if (x.type() == Segment::Type::ObjectDefinition) {
	// 		std::cerr << "Len:" << std::get<ObjectDefinition>(x).data.size() << "\n";
	// 	}
	// 	std::span original_bytes{source.data() + start, static_cast<std::size_t>(ss.tellg()) - start};

	// 	std::ostringstream oss;
	// 	serial::dump(oss, x);

	// 	std::vector<uint8_t> new_bytes(oss.tellp());
	// 	std::ranges::transform(oss.str(), new_bytes.begin(), [](char ch) { return std::bit_cast<uint8_t>(ch); });

	// 	if (std::vector<uint8_t>{original_bytes.begin(), original_bytes.end()} != new_bytes) {
	// 		std::cerr << i << "\n";
	// 		std::cerr << (int)std::to_underlying(x.type()) << "\n";

	// 		for (size_t i = 0;; ++i) {
	// 			if (original_bytes[i] != new_bytes[i]) {
	// 				std::cerr << "FAIL: " << i << "\n";
	// 				break;
	// 			}
	// 		}
	// 		print_bytes(original_bytes);
	// 		print_bytes(new_bytes);
	// 		std::cerr << "\n";
	// 		break;
	// 	}

	// 	// segments.push_back(serial::load<Segment>(ss));
	// 	++i;
	// }

	// std::ofstream ofs("out.sup", std::ios::binary);
	// for (auto& segment : segments) serial::dump(ofs, segment);

	// =================================================================================================================

	// namespace bg = boost::gil;
	// static_assert(sizeof(bg::rgba8_pixel_t) == 4);

	// std::array<bg::rgba8_pixel_t, 256> clut;

	// std::ifstream ifs;
	// ifs.exceptions(std::ios::badbit | std::ios::failbit);
	// ifs.open("test.sup", std::ios::binary);

	// size_t image_index = 0;
	// while (ifs.peek() != decltype(ifs)::traits_type::eof()) {
	// 	Segment x = serial::load<Segment>(ifs);
	// 	std::cout << typeid(x).name() << "\n";

	// 	if (x.type() == Segment::Type::PaletteDefinition) {
	// 		auto& palette = get<PaletteDefinition>(x);
	// 		clut.fill(bg::rgba8_pixel_t{});

	// 		for (auto& entry : palette.entries) {
	// 			auto [r, g, b, a] = convert_YCbCrA_BT709_RGBA(entry.y, entry.cb, entry.cr, entry.alpha);
	// 			clut[entry.id] = {r, g, b, a};
	// 		}
	// 	}
	// 	if (x.type() == Segment::Type::ObjectDefinition) {
	// 		auto& obj = get<ObjectDefinition>(x);

	// 		auto bitmap = decode(obj.data, clut);
	// 		assert(std::to_underlying(obj.sequence_flag) & std::to_underlying(ObjectDefinition::SequenceFlag::First));
	// 		assert(std::to_underlying(obj.sequence_flag) & std::to_underlying(ObjectDefinition::SequenceFlag::Last));

	// 		assert(bitmap.size() == obj.width * obj.height);
	// 		auto bitmap_view = bg::interleaved_view(obj.width, obj.height, bitmap.data(), obj.width * 4);
	// 		bg::write_view(std::format("out/{}.png", ++image_index), bitmap_view, bg::png_tag{});
	// 	}
	// }
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
