/*******************************************************************************
** @file       Main.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "isobus/hardware_integration/available_can_drivers.hpp"

#include "Main.hpp"
#include "Settings.hpp"
#include "git.h"

AgISOVirtualTerminalApplication::MainWindow::MainWindow(juce::String name,
                                                        const std::string &canLogPath,
                                                        int vtNumberCmdLineArg,
                                                        std::string screenCaptureDir) :
  DocumentWindow(name,
                 juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId),
                 DocumentWindow::allButtons)
{
	int vtNumber = vtNumberCmdLineArg;
#ifdef JUCE_WINDOWS
	canDrivers.push_back(std::make_shared<isobus::PCANBasicWindowsPlugin>(static_cast<WORD>(PCAN_USBBUS1)));
#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
	canDrivers.push_back(std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0));
#else
	canDrivers.push_back(nullptr);
#endif
	canDrivers.push_back(std::make_shared<isobus::TouCANPlugin>(static_cast<std::int16_t>(0), 0));
	canDrivers.push_back(std::make_shared<isobus::SysTecWindowsPlugin>());
#elif defined(JUCE_MAC)
	canDrivers.push_back(std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1));
#elif JUCE_ANDROID
	// Android has no SocketCAN device. Use the TCP gateway as its CAN transport.
	canDrivers.push_back(std::make_shared<TcpCANPlugin>("127.0.0.1", 29500));
#else
	canDrivers.push_back(std::make_shared<isobus::SocketCANInterface>("can0"));
#endif
#if !JUCE_ANDROID
	canDrivers.push_back(std::make_shared<TcpCANPlugin>("127.0.0.1", 29500));
#endif

	jassert(!canDrivers.empty()); // You need some kind of CAN interface to run this program!
	isobus::CANHardwareInterface::set_number_of_can_channels(1);

	auto &config = isobus::CANNetworkManager::CANNetwork.get_configuration();
	config.set_max_number_transport_protocol_sessions(256);
	// A full 255-packet ETP burst can delay the following CTS long enough for
	// some implement ECUs to time out. Keep a larger-than-default window while
	// leaving enough time for the receiver to process each burst.
	config.set_number_of_packets_per_dpo_message(64);
	config.set_number_of_packets_per_cts_message(255);

	isobus::NAME serverNAME(0);

	Settings settings;
	if (!settings.load_settings())
	{
		{
			isobus::CANStackLogger::info("Config file not found, using defaults.");
#ifdef JUCE_LINUX
			std::static_pointer_cast<isobus::SocketCANInterface>(canDrivers.at(0))->set_name("can0");
			isobus::CANStackLogger::warn("Socket CAN interface name not yet configured. Using default of \"can0\"");
#endif
		}
	}
	else
	{
		if (0 == vtNumberCmdLineArg)
		{
			// no command line argument provided -> use the saved setting
			serverNAME.set_function_instance(settings.vt_number() - 1);
			vtNumber = settings.vt_number();
		}
		else
		{
			// VT number provided from the vtNumberCmdLineArg line
			serverNAME.set_function_instance(vtNumberCmdLineArg - 1);
			vtNumber = vtNumberCmdLineArg;
		}
	}

	serverNAME.set_arbitrary_address_capable(true);
	serverNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	serverNAME.set_industry_group(2);
	serverNAME.set_manufacturer_code(1407);
	serverInternalControlFunction = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(serverNAME, 0, 0x26);
	setUsingNativeTitleBar(true);
	setContentOwned(new ServerMainComponent(serverInternalControlFunction, canDrivers, settings.settingsValueTree(), canLogPath, vtNumber, screenCaptureDir), true);

#if JUCE_IOS
	setFullScreen(true);
#elif JUCE_ANDROID
	setResizable(false, false);
#else
	setResizable(true, true);
	centreWithSize(getWidth(), getHeight());
#endif

	setIcon(ImageCache::getFromMemory(AppImages::logosmall_png, AppImages::logosmall_pngSize));
#if JUCE_LINUX
	// this hack is needed on Linux
	ComponentPeer *peer = getPeer();
	if (peer)
	{
		peer->setIcon(ImageCache::getFromMemory(AppImages::logosmall_png, AppImages::logosmall_pngSize));
	}
#endif
	setVisible(true);
#if JUCE_ANDROID
	// Re-apply after the Activity view is attached; Android may otherwise keep
	// the navigation/status insets from the initial window creation.
	if (auto *peer = getPeer())
		peer->setFullScreen(true);
#endif
}

void AgISOVirtualTerminalApplication::MainWindow::closeButtonPressed()
{
	// This is called when the user tries to close this window. Here, we'll just
	// ask the app to quit when this happens, but you can change this to do
	// whatever you need.
	for (const auto &driver : canDrivers)
	{
		if (auto tcpDriver = std::dynamic_pointer_cast<TcpCANPlugin>(driver))
		{
			tcpDriver->close();
		}
	}
	if (isobus::CANHardwareInterface::is_running())
	{
		isobus::CANHardwareInterface::stop();
	}
	for (std::uint8_t channel = 0; channel < isobus::CANHardwareInterface::get_number_of_can_channels(); ++channel)
	{
		isobus::CANHardwareInterface::unassign_can_channel_frame_handler(channel);
	}
	JUCEApplication::getInstance()->systemRequestedQuit();
}

std::string AgISOVirtualTerminalApplication::getApplicationBuildInfo()
{
	std::string gitDescribe = std::string(git::Describe());
	if (gitDescribe.length() > 0)
	{
		return gitDescribe + (git::AnyUncommittedChanges() ? "-dirty" : "");
	}
	return ProjectInfo::versionString;
}

std::string AgISOVirtualTerminalApplication::getApplicationNameWithBuildInfo()
{
	std::string name = ProjectInfo::projectName;
	auto buildInfo = getApplicationBuildInfo();
	if (buildInfo.length() > 0)
	{
		name.append(" - " + buildInfo);
	}
	return name;
}

START_JUCE_APPLICATION(AgISOVirtualTerminalApplication)
