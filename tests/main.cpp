// calc_test.cpp
#include <gtest/gtest.h>
#include "lib.h"

TEST(UTF8, Transcode) {

	//2344 2368 120 2344 2368 10084
	auto& a = u8"नीxनी❤";

	utf8_encoding::decode_iterator beg(std::begin(a));
	utf8_encoding::decode_iterator end(std::end(a) - 1);

	ASSERT_EQ(*beg, codepoint(2344));
	++beg;
	ASSERT_EQ(*beg, codepoint(2368));
	++beg;
	ASSERT_EQ(*beg, codepoint(120));
	++beg;
	ASSERT_EQ(*beg, codepoint(2344));
	++beg;
	ASSERT_EQ(*beg, codepoint(2368));
	++beg;
	ASSERT_EQ(*beg, codepoint(10084));
	++beg;

	ASSERT_EQ(beg, end);
}

TEST(Segmentation, GraphemeDetection) {

	//नी: 2344 2368

	{
		auto a = codepoint(2344);
		auto b = codepoint(2368);
		auto c = codepoint(U'x');

		ASSERT_TRUE(segmentation::is_grapheme(a, b));
		ASSERT_FALSE(segmentation::is_grapheme(b, c));
	}


	//🖖🏿: 128406 127999 (SkinTone)
	{
		auto a = codepoint(128406);
		auto b = codepoint(127999);
		auto c = codepoint(U'x');

		ASSERT_TRUE(segmentation::is_grapheme(a, b));
		ASSERT_FALSE(segmentation::is_grapheme(b, c));
	}

	//🖖🏿: 128406 127999
	{
		auto a = codepoint(128406);
		auto b = codepoint(127999);
		auto c = codepoint(U'x');

		ASSERT_TRUE(segmentation::is_grapheme(a, b));
		ASSERT_FALSE(segmentation::is_grapheme(b, c));
	}

}

TEST(Character, Literal) {
	auto c1 = U8_CHAR("❤");
	auto c2 = U8_CHAR("1");
	auto c3 = U8_CHAR("नी");
	auto c4 = U8_CHAR("👳‍");
	auto c5 = U8_CHAR("🦸‍");
}

TEST(String, Length) {
	auto a = utf8string(u8"नी");

	ASSERT_EQ(a.length(), 1);

	auto b = utf8string(u8"❤");

	ASSERT_EQ(b.length(), 1);

	auto c = utf8string(u8"👳‍");

	ASSERT_EQ(c.length(), 1);

	auto abc = utf8string(u8"नी❤👳‍");
	ASSERT_EQ(abc.length(), 3);

	auto lon = utf8string(u8"नी❤👳‍नी❤👳‍नी❤👳‍");
	ASSERT_EQ(lon.length(), 9);

	auto mixed = utf8string(u8"abc123😡");
	ASSERT_EQ(mixed.length(), 7);

	auto rl = utf8string(u8"10101010101010101010101010101010101010101010101010101");
	ASSERT_EQ(rl.length(), 53);
}


TEST(String, Index) {
	auto str = utf8string(u8"😁❤X");
	ASSERT_EQ(str[0], U8_CHAR("😁"));
	ASSERT_EQ(str[1], U8_CHAR("❤"));
	ASSERT_EQ(str[2], U8_CHAR("X"));
}

int main(int argc, char** argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
