/*******************************************************************************
** @file       ConfigureHardwareWindow.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/

#include "ConfigureHardwareWindow.hpp"

ConfigureHardwareWindow::ConfigureHardwareWindow(ServerMainComponent &parentComponent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  DocumentWindow("Configure Hardware", juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId), DocumentWindow::closeButton),
  parentServer(parentComponent),
  content(*this, canDrivers)
{
	setOpaque(true);
	setContentNonOwned(&content, false);
	setContentComponentSize(400, 390);
	centreWithSize(400, 390);
}

void ConfigureHardwareWindow::closeButtonPressed()
{
	exitModalState(0);
	setVisible(false);
}
