//================================================================================================
/// @file ConfigureHardwareWindow.hpp
///
/// @brief Defines a GUI window which allows selecting and configuring a hardware interface.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Contributors
//================================================================================================
#ifndef CONFIGURE_HARDWARE_WINDOW_HPP
#define CONFIGURE_HARDWARE_WINDOW_HPP

#include "ConfigureHardwareComponent.hpp"
#include "ResponsiveDialogWindow.hpp"

class ServerMainComponent;

class ConfigureHardwareWindow : public ResponsiveDialogWindow
{
public:
	ConfigureHardwareWindow(ServerMainComponent &parentComponent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers);

	ServerMainComponent &parentServer;

private:
	ConfigureHardwareComponent content;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigureHardwareWindow)
};

#endif // CONFIGURE_HARDWARE_WINDOW_HPP
