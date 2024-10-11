#include <doctest/doctest.h>
#include <pgs.hpp>

#include <filesystem>
#include <fstream>
#include <ranges>

namespace fs = std::filesystem;

/// Check that two byte ranges are equal and throw otherwise.
template<std::ranges::random_access_range R1, std::ranges::random_access_range R2>
	requires serialize::ByteLike<std::ranges::range_value_t<R1>> and serialize::ByteLike<std::ranges::range_value_t<R2>>
void assert_equal(R1&& rng1, R2&& rng2) {
	auto [it1, it2] = std::ranges::mismatch(rng1, rng2);
	if (it1 == rng1.end()) {
		if (it2 == rng2.end()) return;

		throw std::runtime_error(std::format(
			"ranges differ: lhs[{0}] == <end>, rhs[{0}] == {1:#04x}\n",
			it1 - rng1.begin(),
			std::bit_cast<std::uint8_t>(*it2)
		));
	}
	if (it2 == rng2.end()) {
		throw std::runtime_error(
			std::format("ranges differ: lhs[{0}] == {1:#04x}, rhs[{0}] == <end>\n", it1 - rng1.begin(), *it1)
		);
	}

	throw std::runtime_error(std::format(
		"ranges differ: lhs[{0}] == {1:#04x}, rhs[{0}] == {2:#04x}\n",
		it1 - rng1.begin(),
		std::bit_cast<std::uint8_t>(*it1),
		std::bit_cast<std::uint8_t>(*it2)
	));
}

TEST_CASE("roundtrip") {
	std::ifstream ifs("tests/data/sample-2.sup", std::ios::binary);
	ifs.exceptions(std::ios::failbit | std::ios::badbit);
	std::stringstream input;
	input << ifs.rdbuf();
	ifs.close();

	std::vector<Segment> segments;
	while (input.peek() != EOF) segments.push_back(serialize::load<Segment>(input));

	std::ostringstream output;
	for (auto& segment : segments) serialize::dump(output, segment);

	assert_equal(input.str(), output.str());
}

TEST_CASE("rl-encoding.roundtrip") {
	std::ifstream ifs("tests/data/sample-2.sup", std::ios::binary);
	ifs.exceptions(std::ios::failbit | std::ios::badbit);
	std::stringstream input;
	input << ifs.rdbuf();
	ifs.close();

	std::vector<Segment> segments;
	while (input.peek() != EOF) segments.push_back(serialize::load<Segment>(input));

	for (auto& segment : segments) {
		if (segment.type() == Segment::Type::ObjectDefinition) {
			auto& ods = std::get<ObjectDefinition>(segment);
			if (std::to_underlying(ods.sequence_flag) & std::to_underlying(ObjectDefinition::SequenceFlag::First) and
				std::to_underlying(ods.sequence_flag) & std::to_underlying(ObjectDefinition::SequenceFlag::Last)) {
				Bitmap bm = decode(std::span{ods.data});
				auto roundtrip_data = encode(bm);
				assert_equal(ods.data, roundtrip_data);
			}
		}
	}
}
