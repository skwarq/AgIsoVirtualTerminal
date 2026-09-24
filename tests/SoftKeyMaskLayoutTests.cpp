#include "SoftKeyMaskLayout.hpp"

#include <gtest/gtest.h>

using soft_key_mask_layout::slot_bounds;

TEST(SoftKeyMaskLayout, ExactFitUsesNoOuterMarginsOrGaps)
{
	EXPECT_TRUE(soft_key_mask_layout::fits_height(480, 6, 80));

	for (int row = 0; row < 6; ++row)
	{
		const auto bounds = slot_bounds(row, 1, 6, 80, 80, 80, 480);
		EXPECT_EQ(bounds.y, row * 80);
		EXPECT_EQ(bounds.x, 0);
		EXPECT_EQ(bounds.width, 80);
		EXPECT_EQ(bounds.height, 80);
	}

	EXPECT_EQ(slot_bounds(5, 1, 6, 80, 80, 80, 480).y + 80, 480);
}

TEST(SoftKeyMaskLayout, DistributesAvailableHeightBetweenKeys)
{
	constexpr int expectedY[] = { 0, 84, 168, 252, 336, 420 };

	for (int row = 0; row < 6; ++row)
		EXPECT_EQ(slot_bounds(row, 1, 6, 60, 60, 60, 480).y, expectedY[row]);

	EXPECT_EQ(soft_key_mask_layout::inter_key_spacing(480, 6, 60), 24);
	EXPECT_EQ(soft_key_mask_layout::inter_key_spacing(481, 6, 80), 1);
}

TEST(SoftKeyMaskLayout, CentersFractionalSpacingRemainder)
{
	constexpr int expectedY[] = { 0, 80, 160, 241, 321, 401 };

	for (int row = 0; row < 6; ++row)
		EXPECT_EQ(slot_bounds(row, 1, 6, 80, 80, 80, 481).y, expectedY[row]);
}

TEST(SoftKeyMaskLayout, PreservesColumnOrderAndHitTestBounds)
{
	const auto rightColumn = slot_bounds(0, 2, 6, 60, 60, 144, 480);
	const auto leftColumn = slot_bounds(6, 2, 6, 60, 60, 144, 480);

	EXPECT_EQ(leftColumn.x, 0);
	EXPECT_EQ(rightColumn.x, 84);
	EXPECT_EQ(leftColumn.x + leftColumn.width + 24, rightColumn.x);
	EXPECT_FALSE(leftColumn.contains(60, 10));
	EXPECT_TRUE(rightColumn.contains(84, 10));
	EXPECT_FALSE(rightColumn.contains(144, 10));
}

TEST(SoftKeyMaskLayout, OnePixelRemainderProducesHorizontalSeparator)
{
	const auto rightColumn = slot_bounds(0, 2, 6, 80, 80, 161, 481);
	const auto leftColumn = slot_bounds(6, 2, 6, 80, 80, 161, 481);

	EXPECT_EQ(leftColumn.x, 0);
	EXPECT_EQ(rightColumn.x, 81);
	EXPECT_EQ(rightColumn.x - (leftColumn.x + leftColumn.width), 1);
}

TEST(SoftKeyMaskLayout, ExpandsRowsToFitMinimumSlotCount)
{
	EXPECT_EQ(soft_key_mask_layout::rows_for_minimum_slots(1, 1, 6), 6);
	EXPECT_EQ(soft_key_mask_layout::rows_for_minimum_slots(3, 1, 6), 2);
	EXPECT_EQ(soft_key_mask_layout::rows_for_minimum_slots(3, 2, 6), 2);
}

TEST(SoftKeyMaskLayout, RejectsInsufficientHeightAndOutOfRangeSlots)
{
	EXPECT_FALSE(soft_key_mask_layout::fits_height(479, 6, 80));
	EXPECT_TRUE(slot_bounds(12, 2, 6, 60, 60, 144, 480).is_empty());
}
