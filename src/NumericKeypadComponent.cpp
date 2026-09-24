/*******************************************************************************
** @file       NumericKeypadComponent.cpp
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "NumericKeypadComponent.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>

namespace
{
bool parseNumericEntry(const juce::String& entry, std::uint8_t decimals, double& value)
{
	if (entry.isEmpty())
		return false;

	bool hasDigit = false;
	bool hasSeparator = false;
	int fractionalDigits = 0;
	for (int index = 0; index < entry.length(); ++index)
	{
		const auto character = entry[index];
		if (character >= '0' && character <= '9')
		{
			hasDigit = true;
			if (hasSeparator && ++fractionalDigits > static_cast<int>(decimals))
				return false;
			continue;
		}

		if (character == '-' && index == 0)
			continue;

		if ((character == '.' || character == ',') && !hasSeparator && decimals > 0)
		{
			hasSeparator = true;
			continue;
		}

		return false;
	}

	// A trailing decimal separator is an unfinished edit, not a value to commit.
	if (!hasDigit || (hasSeparator && fractionalDigits == 0))
		return false;

	value = entry.replace(",", ".").getDoubleValue();
	return std::isfinite(value);
}
}

NumericKeypadComponent::NumericKeypadComponent(double initialValue,
                                               double minimumValue,
                                               double maximumValue,
                                               std::uint8_t numberOfDecimals) :
  minimum(std::min(minimumValue, maximumValue)),
  maximum(std::max(minimumValue, maximumValue)),
  decimals(numberOfDecimals)
{
	entry = String(initialValue, static_cast<int>(decimals));

	keysViewport.setViewedComponent(&keysContent, false);
	keysViewport.setScrollBarsShown(false, false);
	addAndMakeVisible(keysViewport);

	entryLabel.setJustificationType(Justification::centredRight);
	entryLabel.setFont(Font(FontOptions{}.withHeight(34.0f).withStyle("Bold")));
	addAndMakeVisible(entryLabel);

	rangeLabel.setJustificationType(Justification::centredRight);
	rangeLabel.setFont(Font(FontOptions{}.withHeight(13.0f)));
	rangeLabel.setText("Range " + String(minimum, static_cast<int>(decimals)) +
	                     " to " + String(maximum, static_cast<int>(decimals)),
	                   dontSendNotification);
	addAndMakeVisible(rangeLabel);

	systemEntryEditor.setText(entry, dontSendNotification);
	systemSignButton.setButtonText(entry.startsWith("-") ? "+" : "-");
	systemEntryEditor.setFont(Font(FontOptions{}.withHeight(24.0f)));
	systemEntryEditor.setJustification(Justification::centred);
	systemEntryEditor.setSelectAllWhenFocused(true);
	systemEntryEditor.setKeyboardType(decimals == 0 ? TextInputTarget::numericKeyboard
	                                                  : TextInputTarget::decimalKeyboard);
	systemEntryEditor.setInputRestrictions(64, "0123456789-.,");
	systemEntryEditor.onTextChange = [this]() {
		entry = systemEntryEditor.getText();
		systemSignButton.setButtonText(entry.startsWith("-") ? "+" : "-");
		hasTypedSinceOpening = true;
		refresh_display();
	};
	addAndMakeVisible(systemEntryEditor);
	systemEntryEditor.setVisible(false);
	systemSignButton.onClick = [this]() { toggleSign(); };
	systemSignButton.setWantsKeyboardFocus(false);
	addAndMakeVisible(systemSignButton);
	systemSignButton.setVisible(false);

	// Built row by row exactly as they are laid out: three digits then an editing key.
	// The digits are ordered like a telephone keypad, which is what an operator expects.
	const char *digitRows[3] = { "789", "456", "123" };
	std::function<void()> editActions[3] = {
		[this]() { backspace(); },
		[this]() { clear(); },
		[this]() { toggleSign(); }
	};
	const char *editLabels[3] = { "\xe2\x8c\xab", "C", "\xc2\xb1" };
	const char *editTooltips[3] = { "Backspace", "Clear the entry", "Change the sign" };

	for (int row = 0; row < 3; row++)
	{
		for (const char *c = digitRows[row]; '\0' != *c; c++)
		{
			const auto character = static_cast<juce::juce_wchar>(*c);
			add_key(String::charToString(character), [this, character]() { append(character); });
		}

		auto *editKey = add_key(String::fromUTF8(editLabels[row]), editActions[row]);
		editKey->setTooltip(editTooltips[row]);

		// Keep sign entry available; range validation reports invalid negatives.
	}

	// Bottom row: a double width zero under the digits, then the decimal point
	add_key("0", [this]() { append('0'); });
	add_key(".", [this]() { append('.'); })->setEnabled(0 != decimals);

	refresh_display();

	setSize((NUMBER_OF_COLUMNS * KEY_SIZE) + ((NUMBER_OF_COLUMNS + 1) * KEY_GAP), FULL_KEYPAD_HEIGHT);
}

TextButton *NumericKeypadComponent::add_key(const String &text, std::function<void()> action)
{
	auto *key = new TextButton(text);

	key->onClick = std::move(action);
	key->setWantsKeyboardFocus(false);
	keysContent.addAndMakeVisible(key);
	keys.add(key);
	return key;
}

void NumericKeypadComponent::append(juce::juce_wchar character)
{
	// The displayed entry is the object's current value until the operator actually types
	// something - the first digit or decimal point overwrites it instead of appending to it,
	// matching what a touch typist expects instead of requiring an explicit Clear press first.
	if (!hasTypedSinceOpening)
	{
		hasTypedSinceOpening = true;
		entry.clear();
	}

	if ('.' == character)
	{
		// Only one decimal point, and only when the object actually displays decimals
		if ((0 == decimals) || entry.containsChar('.'))
		{
			return;
		}
	}
	else if (0 != decimals)
	{
		// Refuse digits which would be dropped when the value is displayed anyway
		const int pointIndex = entry.indexOfChar('.');

		if ((pointIndex >= 0) && ((entry.length() - pointIndex - 1) >= static_cast<int>(decimals)))
		{
			return;
		}
	}

	// A lone leading zero is replaced rather than appended to
	if (entry.equalsIgnoreCase("0") && ('.' != character))
	{
		entry = String::charToString(character);
	}
	else if (entry.equalsIgnoreCase("-0") && ('.' != character))
	{
		entry = "-" + String::charToString(character);
	}
	else
	{
		entry += String::charToString(character);
	}
	refresh_display();
}

void NumericKeypadComponent::backspace()
{
	hasTypedSinceOpening = true;
	if (entry.isNotEmpty())
	{
		entry = entry.dropLastCharacters(1);
	}
	refresh_display();
}

void NumericKeypadComponent::clear()
{
	hasTypedSinceOpening = true;
	entry.clear();
	refresh_display();
}

void NumericKeypadComponent::toggleSign()
{
	hasTypedSinceOpening = true;
	entry = entry.startsWith("-") ? entry.substring(1) : ("-" + entry);
	if (useSystemKeyboard)
	{
		systemSignButton.setButtonText(entry.startsWith("-") ? "+" : "-");
		systemEntryEditor.setText(entry, dontSendNotification);
		systemEntryEditor.setCaretPosition(entry.length());
	}
	refresh_display();
}

void NumericKeypadComponent::refresh_display()
{
	entryLabel.setText(entry.isEmpty() ? "0" : entry, dontSendNotification);

	const bool valid = is_within_range();
	if (!hasTypedSinceOpening && !valid)
	{
		rangeLabel.setText("Current value is outside range", dontSendNotification);
	}
	else if (hasTypedSinceOpening && !valid)
	{
		const bool complete = entry.isNotEmpty() && (entry != "-") && (entry != ".") && (entry != "-.");
		rangeLabel.setText(complete
		                     ? "Enter " + String(minimum, static_cast<int>(decimals)) + " to " + String(maximum, static_cast<int>(decimals))
		                     : "Enter a valid number",
		                   dontSendNotification);
	}
	else
	{
		rangeLabel.setText("Range " + String(minimum, static_cast<int>(decimals)) +
		                     " to " + String(maximum, static_cast<int>(decimals)),
		                   dontSendNotification);
	}

	entryLabel.setColour(Label::textColourId,
	                     valid ? getLookAndFeel().findColour(Label::textColourId) : Colours::red);
	rangeLabel.setColour(Label::textColourId,
	                     valid ? getLookAndFeel().findColour(Label::textColourId).withAlpha(0.6f) : Colours::red);
	rangeLabel.repaint();
}

double NumericKeypadComponent::get_value() const
{
	double value = 0.0;
	return parseNumericEntry(entry, decimals, value) ? value : std::numeric_limits<double>::quiet_NaN();
}

bool NumericKeypadComponent::is_within_range() const
{
	double typed = 0.0;
	if (!parseNumericEntry(entry, decimals, typed))
		return false;

	return (typed >= minimum) && (typed <= maximum);
}

bool NumericKeypadComponent::can_confirm() const
{
	return !hasTypedSinceOpening || is_within_range();
}

void NumericKeypadComponent::paint(Graphics &graphics)
{
	const int displayHeight = useSystemKeyboard
	    ? SYSTEM_ENTRY_HEIGHT + (rangeLabel.isVisible() ? SYSTEM_RANGE_HEIGHT : 0)
	    : DISPLAY_HEIGHT;
	auto displayArea = getLocalBounds().removeFromTop(displayHeight)
	                       .reduced(KEY_GAP, useSystemKeyboard ? 0 : KEY_GAP);

	graphics.setColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId).darker(0.4f));
	graphics.fillRoundedRectangle(displayArea.toFloat(), 4.0f);
}

void NumericKeypadComponent::activateSystemKeyboardIfNeeded()
{
	if (useSystemKeyboard && isShowing())
		systemEntryEditor.grabKeyboardFocus();
}

void NumericKeypadComponent::resized()
{
	const bool wasUsingSystemKeyboard = useSystemKeyboard;
	if (useSystemKeyboard && getHeight() >= FULL_KEYPAD_HEIGHT)
	{
		useSystemKeyboard = false;
		systemEntryEditor.setVisible(false);
		systemSignButton.setVisible(false);
		entryLabel.setVisible(true);
		keysViewport.setVisible(true);
	}
	else if (!useSystemKeyboard && getHeight() < FULL_KEYPAD_HEIGHT)
	{
		useSystemKeyboard = true;
		keysViewport.setVisible(false);
		entryLabel.setVisible(false);
		systemEntryEditor.setVisible(true);
		systemSignButton.setVisible(true);
	}
	if (wasUsingSystemKeyboard != useSystemKeyboard && preferredHeightChanged)
	{
		const auto safeThis = juce::Component::SafePointer<NumericKeypadComponent>(this);
		auto callback = preferredHeightChanged;
		const int preferredHeight = useSystemKeyboard ? SYSTEM_DISPLAY_HEIGHT : FULL_KEYPAD_HEIGHT;
		juce::MessageManager::callAsync([safeThis, callback = std::move(callback), preferredHeight]() mutable
		{
			if (safeThis != nullptr)
				callback(preferredHeight);
		});
	}

	auto bounds = getLocalBounds();
	if (useSystemKeyboard)
	{
		auto editorRow = bounds.removeFromTop(SYSTEM_ENTRY_HEIGHT).reduced(KEY_GAP, 0);
		const int signWidth = systemSignButton.isVisible() ? 42 : 0;
		if (signWidth > 0)
			systemSignButton.setBounds(editorRow.removeFromRight(signWidth));
		systemEntryEditor.setBounds(editorRow);
		const bool showRange = bounds.getHeight() >= SYSTEM_RANGE_HEIGHT;
		rangeLabel.setVisible(showRange);
		if (showRange)
			rangeLabel.setBounds(bounds.removeFromTop(SYSTEM_RANGE_HEIGHT).reduced(KEY_GAP, 0));
		return;
	}

	auto displayArea = bounds.removeFromTop(DISPLAY_HEIGHT).reduced(KEY_GAP * 2, KEY_GAP);
	rangeLabel.setBounds(displayArea.removeFromBottom(18));
	entryLabel.setBounds(displayArea);

	keysViewport.setBounds(bounds);
	const int keysHeight = (NUMBER_OF_ROWS * KEY_SIZE) + ((NUMBER_OF_ROWS + 1) * KEY_GAP);
	// Set the full grid height first so the viewport can account for a scrollbar
	// before the touch targets are laid out horizontally.
	keysContent.setSize(keysViewport.getWidth(), keysHeight);
	const int contentWidth = juce::jmax(1, keysViewport.getMaximumVisibleWidth());
	keysContent.setSize(contentWidth, keysHeight);
	const int columnWidth = juce::jmax(1, (contentWidth - ((NUMBER_OF_COLUMNS + 1) * KEY_GAP)) / NUMBER_OF_COLUMNS);

	// Rows of three digits plus an editing key
	int index = 0;
	for (int row = 0; row < NUMBER_OF_ROWS - 1; row++)
	{
		const int y = KEY_GAP + (row * (KEY_SIZE + KEY_GAP));
		for (int column = 0; column < NUMBER_OF_COLUMNS; column++)
		{
			keys[index]->setBounds(KEY_GAP + (column * (columnWidth + KEY_GAP)), y, columnWidth, KEY_SIZE);
			index++;
		}
	}

	// Bottom row: zero spans the first two columns, the decimal point sits in the third.
	const int lastRowY = KEY_GAP + ((NUMBER_OF_ROWS - 1) * (KEY_SIZE + KEY_GAP));
	keys[index]->setBounds(KEY_GAP, lastRowY, (2 * columnWidth) + KEY_GAP, KEY_SIZE);
	index++;
	keys[index]->setBounds(KEY_GAP + (2 * (columnWidth + KEY_GAP)), lastRowY, columnWidth, KEY_SIZE);
}
