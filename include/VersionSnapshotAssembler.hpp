#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

/// Collects independently supplied IOP components and exposes data only when
/// the complete set is present. Components are always assembled by index.
class VersionSnapshotAssembler
{
public:
	bool reset(std::size_t componentCount)
	{
		components.clear();
		received.clear();
		if (componentCount == 0) return false;
		components.assign(componentCount, {});
		received.assign(componentCount, false);
		return true;
	}

	bool set_component(std::size_t index, std::vector<std::uint8_t> component)
	{
		if (index >= components.size() || component.empty()) return false;
		components[index] = std::move(component);
		received[index] = true;
		return true;
	}

	bool complete() const
	{
		return !received.empty() && std::all_of(received.begin(), received.end(), [](bool value) { return value; });
	}

	bool assemble(std::vector<std::uint8_t> &result) const
	{
		if (!complete()) return false;
		std::size_t totalSize = 0;
		for (const auto &component : components)
		{
			if (component.size() > std::numeric_limits<std::size_t>::max() - totalSize) return false;
			totalSize += component.size();
		}
		result.clear();
		result.reserve(totalSize);
		for (const auto &component : components) result.insert(result.end(), component.begin(), component.end());
		return true;
	}

private:
	std::vector<std::vector<std::uint8_t>> components;
	std::vector<bool> received;
};
