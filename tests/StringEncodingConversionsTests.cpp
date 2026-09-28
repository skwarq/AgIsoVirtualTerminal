#include <gtest/gtest.h>

#include "StringEncodingConversions.hpp"

TEST(StringEncodingConversions, DecodesPolishCharactersFromISO8859_2)
{
	const std::string encoded = "Kra\xF1" "cowy";
	std::string decoded;

	convert_string_to_utf_8(SourceEncoding::ISO8859_2, encoded, decoded, false);

	EXPECT_EQ(decoded, "Krańcowy");
}
