//================================================================================================
/// @file AckSettingsWindow.hpp
///
/// @brief Defines a dialog where ACK settings can be configured
/// @author Miklos Marton
///
/// @copyright The Open-Agriculture Developers
//================================================================================================
#pragma once

#include "ResponsiveDialogWindow.hpp"

class AckSettingsWindow : public ResponsiveDialogWindow
  , public juce::KeyListener
{
public:
	AckSettingsWindow(int alarmAckKeyCode, bool showAckButton, Component *associatedComponent = nullptr);

	bool keyPressed(const juce::KeyPress &key, juce::Component *originatingComponent) override;
	int alarmAckKeyCode() const;
	bool shouldShowAckButton() const;

private:
	juce::TextButton selectAlarmAckKeyButton;
	juce::ToggleButton showAckButtonCheckbox;
	int alarmAckKey = juce::KeyPress::escapeKey;

	bool keySelectionMode = false;

	void updateAlarmAckButtonLabel(const juce::KeyPress &key);

	void setAlarmAckKeySelection(bool isEnabled);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AckSettingsWindow)
};
