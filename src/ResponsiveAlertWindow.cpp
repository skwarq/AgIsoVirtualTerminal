#include "ResponsiveAlertWindow.hpp"

void ResponsiveAlertWindow::fitToDisplay()
{
	const auto available = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.reduced(16);

	// AlertWindow calculates its button positions from the current height. On
	// small displays its content can therefore reach the button row before we
	// get a chance to scale the window. Detect that situation from the actual
	// child bounds and give AlertWindow the missing layout space first.
	int contentBottom = 0;
	int firstButtonTop = getHeight();
	for (int i = 0; i < getNumChildComponents(); ++i)
	{
		if (auto* child = getChildComponent(i); child != nullptr && child->isVisible())
		{
			const auto childBounds = child->getBounds();
			if (dynamic_cast<juce::Button*>(child) != nullptr)
				firstButtonTop = juce::jmin(firstButtonTop, childBounds.getY());
			else
				contentBottom = juce::jmax(contentBottom, childBounds.getBottom());
		}
	}
	if (firstButtonTop <= contentBottom)
		setSize(getWidth(), getHeight() + contentBottom - firstButtonTop + 12);

	// AlertWindow lays out its children before this method is called. If the
	// resulting dialog is taller than the display, resizing it directly makes
	// AlertWindow place its buttons over the last controls. Scale the complete
	// dialog instead so the layout remains intact and input coordinates are
	// transformed together with the controls.
	setTransform(juce::AffineTransform());
	const int width = getWidth() > 0 ? getWidth() : 400;
	const int height = getHeight() > 0 ? getHeight() : 390;
	const auto scale = juce::jmin(1.0f,
	                             static_cast<float>(available.getWidth()) / static_cast<float>(width),
	                             static_cast<float>(available.getHeight()) / static_cast<float>(height));
	const int fittedWidth = juce::jmax(1, juce::roundToInt(width * scale));
	const int fittedHeight = juce::jmax(1, juce::roundToInt(height * scale));
	setTransform(juce::AffineTransform::scale(scale));
	centreWithSize(fittedWidth, fittedHeight);
}
