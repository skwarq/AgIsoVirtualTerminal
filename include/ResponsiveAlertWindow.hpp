#pragma once

#include <JuceHeader.h>

class ResponsiveAlertWindow : public juce::AlertWindow
{
public:
	ResponsiveAlertWindow(const juce::String& title,
	                      const juce::String& message,
	                      juce::MessageBoxIconType icon,
	                      juce::Component* associatedComponent = nullptr) :
		juce::AlertWindow(title, message, icon, associatedComponent)
	{
	}

	void fitToDisplay();
};
