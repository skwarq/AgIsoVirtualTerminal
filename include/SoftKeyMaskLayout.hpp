#pragma once

#include <algorithm>
#include <cstdint>

struct SoftKeySlotBounds
{
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;

	bool is_empty() const
	{
		return (width <= 0) || (height <= 0);
	}

	bool contains(int pointX, int pointY) const
	{
		return !is_empty() && pointX >= x && pointX < x + width && pointY >= y && pointY < y + height;
	}
};

namespace soft_key_mask_layout
{
inline int rows_for_minimum_slots(int columns, int configuredRows, int minimumSlots)
{
	if ((columns <= 0) || (configuredRows <= 0) || (minimumSlots <= 0) ||
	    static_cast<std::int64_t>(columns) * configuredRows >= minimumSlots)
	{
		return configuredRows;
	}
	return (minimumSlots + columns - 1) / columns;
}

inline bool fits_height(int availableHeight, int rows, int keyHeight)
{
	return availableHeight > 0 && rows > 0 && keyHeight > 0 &&
	       static_cast<std::int64_t>(rows) * keyHeight <= availableHeight;
}

inline int inter_key_spacing(int availableHeight, int rows, int keyHeight)
{
	if ((rows <= 1) || (keyHeight <= 0))
	{
		return 0;
	}
	const auto freeHeight = static_cast<std::int64_t>(availableHeight) - static_cast<std::int64_t>(rows) * keyHeight;
	const auto nonNegativeFreeHeight = std::max<std::int64_t>(0, freeHeight);
	const auto gapCount = rows - 1;
	return static_cast<int>((nonNegativeFreeHeight + gapCount - 1) / gapCount);
}

inline int offset_for_item(int index, int itemSize, int itemCount, int availableSize)
{
	const auto freeSpace = std::max<std::int64_t>(0, static_cast<std::int64_t>(availableSize) -
	                                                  static_cast<std::int64_t>(itemCount) * itemSize);
	const int gapCount = itemCount - 1;
	if ((gapCount <= 0) || (index <= 0))
	{
		return index * itemSize;
	}

	const auto baseGap = freeSpace / gapCount;
	const int remainder = static_cast<int>(freeSpace % gapCount);
	const int firstExtraGap = (gapCount - remainder) / 2;
	const int extraGapsBefore = std::clamp(index - firstExtraGap, 0, remainder);
	return static_cast<int>(static_cast<std::int64_t>(index) * (itemSize + baseGap) + extraGapsBefore);
}

inline SoftKeySlotBounds slot_bounds(int slotIndex, int columns, int rows, int keyWidth, int keyHeight, int availableWidth, int availableHeight)
{
	if ((slotIndex < 0) || (rows <= 0) || (columns <= 0) || (keyWidth <= 0) || (keyHeight <= 0))
	{
		return {};
	}

	const int row = slotIndex % rows;
	const int columnFromRight = slotIndex / rows;
	if (columnFromRight >= columns)
	{
		return {};
	}
	const int column = columns - 1 - columnFromRight;
	return { offset_for_item(column, keyWidth, columns, availableWidth),
	         offset_for_item(row, keyHeight, rows, availableHeight),
	         keyWidth,
	         keyHeight };
}
} // namespace soft_key_mask_layout
