#ifndef NUMERIC_VALUE_UTILS_HPP
#define NUMERIC_VALUE_UTILS_HPP

#include <cstdint>

namespace vt_numeric
{
	/// Converts an ISOBUS raw unsigned value to its displayed value.
	/// Promote before addition or a negative offset wraps in uint32_t.
	inline double to_display_value(std::uint32_t rawValue, std::int32_t offset, float scale) noexcept
	{
		return (static_cast<double>(rawValue) + static_cast<double>(offset)) * static_cast<double>(scale);
	}

	/// Converts a displayed value back to the raw domain used by the VT object pool.
	inline double to_raw_value(double displayValue, std::int32_t offset, float scale) noexcept
	{
		return (displayValue / static_cast<double>(scale)) - static_cast<double>(offset);
	}
}

#endif // NUMERIC_VALUE_UTILS_HPP
