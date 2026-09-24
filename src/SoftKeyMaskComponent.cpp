/*******************************************************************************
** @file       SoftKeyMaskComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "SoftKeyMaskComponent.hpp"
#include "JuceManagedWorkingSetCache.hpp"

#include "SoftKeyMaskRenderAreaComponent.hpp"

SoftKeyMaskComponent::SoftKeyMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::SoftKeyMask sourceObject, SoftKeyMaskDimensions dimensions) :
  isobus::SoftKeyMask(sourceObject),
  parentWorkingSet(workingSet),
  dimensionInfo(dimensions)
{
	setOpaque(true);
	setBounds(0, 0, dimensions.total_width(), dimensions.height);
	on_content_changed(true);
}

void SoftKeyMaskComponent::on_content_changed(bool initial)
{
	for (std::uint16_t i = 0; i < this->get_number_children(); i++)
	{
		auto child = get_object_by_id(get_child_id(i), parentWorkingSet->get_object_tree());

		if (nullptr != child)
		{
			auto component = JuceManagedWorkingSetCache::create_component(parentWorkingSet, child);
			if (nullptr == component)
			{
				continue;
			}

			const auto slotBounds = dimensionInfo.slot_bounds(i, getWidth(), getHeight());
			if (slotBounds.isEmpty())
			{
				continue;
			}

			component->setBounds(slotBounds);
			addAndMakeVisible(*component);
			childComponents.push_back(std::move(component));
		}
	}

	if (!initial)
	{
		repaint();
	}
}

void SoftKeyMaskComponent::paint(Graphics &g)
{
	auto vtColour = parentWorkingSet->get_colour(backgroundColor);

	g.fillAll(Colour::fromFloatRGBA(vtColour.r, vtColour.g, vtColour.b, 1.0f));
}

int SoftKeyMaskDimensions::key_count() const
{
	return columnCount * rowCount;
}

int SoftKeyMaskDimensions::total_width() const
{
	return juce::jmax(0, columnCount) * juce::jmax(0, keyWidth) +
	       juce::jmax(0, columnCount - 1) * key_spacing();
}

int SoftKeyMaskDimensions::total_height() const
{
	return height;
}

int SoftKeyMaskDimensions::key_spacing() const
{
	return soft_key_mask_layout::inter_key_spacing(height, rowCount, keyHeight);
}

juce::Rectangle<int> SoftKeyMaskDimensions::slot_bounds(int slotIndex, int availableWidth, int availableHeight) const
{
	const auto bounds = soft_key_mask_layout::slot_bounds(slotIndex, columnCount, rowCount, keyWidth, keyHeight, availableWidth, availableHeight);
	return { bounds.x, bounds.y, bounds.width, bounds.height };
}
