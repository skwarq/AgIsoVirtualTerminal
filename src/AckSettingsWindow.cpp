/*******************************************************************************
** @file       AckSettingsWindow.cpp
** @author     Miklos Marton
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "AckSettingsWindow.hpp"

AckSettingsWindow::AckSettingsWindow(int alarmAckKeyCode, bool showAckButton, Component *associatedComponent) :
  ResponsiveDialogWindow("ACK button", "Configure the alarm acknowledge key and dedicated ACK button."),
  alarmAckKey(alarmAckKeyCode)
{
	juce::KeyPress alarmAckKey(alarmAckKeyCode);
	addButton("OK", 5);
	addButton("Cancel", 0);
	selectAlarmAckKeyButton.onClick = [this]() { setAlarmAckKeySelection(true); };
	updateAlarmAckButtonLabel(alarmAckKey);
	addCustomComponent(&selectAlarmAckKeyButton);
	showAckButtonCheckbox.setButtonText("Show dedicated ACK button when an alarm mask is active");
	showAckButtonCheckbox.setToggleState(showAckButton, juce::dontSendNotification);
	addCustomComponent(&showAckButtonCheckbox);

	setAlarmAckKeySelection(false);
}

bool AckSettingsWindow::keyPressed(const KeyPress &key, Component *originatingComponent)
{
	if (keySelectionMode)
	{
		alarmAckKey = key.getKeyCode();
		setAlarmAckKeySelection(false);
		updateAlarmAckButtonLabel(key);
		return true;
	}
	return false;
}

int AckSettingsWindow::alarmAckKeyCode() const
{
	return alarmAckKey;
}

bool AckSettingsWindow::shouldShowAckButton() const
{
	return showAckButtonCheckbox.getToggleState();
}

void AckSettingsWindow::updateAlarmAckButtonLabel(const KeyPress &key)
{
	selectAlarmAckKeyButton.setButtonText("Alarm acknowledge key: " + key.getTextDescription());
}

void AckSettingsWindow::setAlarmAckKeySelection(bool isEnabled)
{
	keySelectionMode = isEnabled;
	selectAlarmAckKeyButton.setEnabled(!isEnabled);
	if (isEnabled)
	{
		selectAlarmAckKeyButton.setButtonText("Alarm acknowledge key: press one key...");

		setWantsKeyboardFocus(true);
		addKeyListener(this);
		grabKeyboardFocus();
	}
	else
	{
		setWantsKeyboardFocus(false);
		removeKeyListener(this);
		if (hasKeyboardFocus(false))
			giveAwayKeyboardFocus();
	}
}
