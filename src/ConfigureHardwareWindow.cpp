/*******************************************************************************
** @file       ConfigureHardwareWindow.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/

#include "ConfigureHardwareWindow.hpp"

ConfigureHardwareWindow::ConfigureHardwareWindow(ServerMainComponent &parentComponent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  ResponsiveDialogWindow("Configure Hardware", ""),
	parentServer(parentComponent),
	content(*this, canDrivers)
{
	addCustomComponent(&content, content.preferredSize().y, 0);
	setVerticalScrollingEnabled(false);
	addButton("OK", 1, [this] { return content.applyConfiguration(); });
	addButton("Cancel", 0);
}
