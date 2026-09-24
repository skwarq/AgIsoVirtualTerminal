//================================================================================================
/// @file SoftKeyMaskComponent.hpp
///
/// @brief Defines a GUI component to draw a soft key mask.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef SOFT_KEY_MASK_COMPONENT_HPP
#define SOFT_KEY_MASK_COMPONENT_HPP

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

#include "JuceHeader.h"
#include "SoftKeyMaskLayout.hpp"

class SoftKeyMaskDimensions
{
public:
	SoftKeyMaskDimensions() = default;

	/**
	 * @brief total_width
	 * @return Width needed for the configured columns and their inter-key spacing
	 */
	int total_width() const;

	/**
   * @brief total_height
   * @return Available height of the softkey mask
   */
	int total_height() const;

	/**
	 * @brief Returns the bounds for a physical soft-key slot.
	 * Inter-key spacing is shared by both axes; there are no outer margins.
	 */
	juce::Rectangle<int> slot_bounds(int slotIndex, int availableWidth, int availableHeight) const;
	int key_spacing() const;

	/**
	 * @brief key_count
	 * @return the number of the possible key positions in the softkey mask
	 */
	int key_count() const;

	int keyHeight = 75;
	int keyWidth = 75;
	int rowCount = 6;
	int columnCount = 2;
	int height = 480;
};

class SoftKeyMaskComponent : public isobus::SoftKeyMask
  , public Component
{
public:
	SoftKeyMaskComponent(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingSet, isobus::SoftKeyMask sourceObject, SoftKeyMaskDimensions dimensions);

	void on_content_changed(bool initial = false);

	void paint(Graphics &g) override;

private:
	std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> parentWorkingSet;
	std::vector<std::shared_ptr<Component>> childComponents;
	SoftKeyMaskDimensions dimensionInfo;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoftKeyMaskComponent)
};

#endif // SOFT_KEY_MASK_COMPONENT_HPP
