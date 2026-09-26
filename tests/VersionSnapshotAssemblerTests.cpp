#include "VersionSnapshotAssembler.hpp"

#include <gtest/gtest.h>

TEST(VersionSnapshotAssembler, WaitsForAllComponentsAndAssemblesInIopOrder)
{
	VersionSnapshotAssembler assembler;
	ASSERT_TRUE(assembler.reset(3));
	ASSERT_TRUE(assembler.set_component(2, { 5, 6 }));
	EXPECT_FALSE(assembler.complete());
	ASSERT_TRUE(assembler.set_component(0, { 1, 2 }));
	std::vector<std::uint8_t> incomplete;
	EXPECT_FALSE(assembler.assemble(incomplete));
	ASSERT_TRUE(assembler.set_component(1, { 3, 4 }));
	ASSERT_TRUE(assembler.complete());
	std::vector<std::uint8_t> result;
	ASSERT_TRUE(assembler.assemble(result));
	EXPECT_EQ(result, (std::vector<std::uint8_t>{ 1, 2, 3, 4, 5, 6 }));
}

TEST(VersionSnapshotAssembler, RejectsInvalidComponentCountAndIndex)
{
	VersionSnapshotAssembler assembler;
	ASSERT_TRUE(assembler.reset(1));
	ASSERT_TRUE(assembler.set_component(0, { 9 }));
	EXPECT_FALSE(assembler.reset(0));
	EXPECT_FALSE(assembler.complete());
	EXPECT_FALSE(assembler.set_component(0, {}));
	ASSERT_TRUE(assembler.reset(1));
	EXPECT_FALSE(assembler.set_component(1, { 1 }));
	EXPECT_FALSE(assembler.complete());
}

TEST(VersionSnapshotAssembler, ReplacingAStagedComponentDoesNotDuplicateIt)
{
	VersionSnapshotAssembler assembler;
	ASSERT_TRUE(assembler.reset(2));
	ASSERT_TRUE(assembler.set_component(0, { 1 }));
	ASSERT_TRUE(assembler.set_component(0, { 2, 3 }));
	ASSERT_TRUE(assembler.set_component(1, { 4 }));
	std::vector<std::uint8_t> result;
	ASSERT_TRUE(assembler.assemble(result));
	EXPECT_EQ(result, (std::vector<std::uint8_t>{ 2, 3, 4 }));
}
