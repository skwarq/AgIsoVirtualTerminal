/*******************************************************************************
** @file       ConfigureHardwareComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ConfigureHardwareComponent.hpp"

#include "ConfigureHardwareWindow.hpp"
#include "ServerMainComponent.hpp"
#include "TcpCANPlugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#ifdef JUCE_WINDOWS
#include "isobus/hardware_integration/toucan_vscp_canal.hpp"
#elif JUCE_LINUX
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

ConfigureHardwareComponent::ConfigureHardwareComponent(ConfigureHardwareWindow &parent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  okButton("OK"),
  parentCANDrivers(canDrivers)
{
	setSize(400, 390);
	okButton.setSize(100, 30);
	addAndMakeVisible(okButton);

#ifdef JUCE_WINDOWS
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");

#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN", "TouCAN", "SysTec", "TCP (CanTcpGateway)" }, 1);
#else
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN (not supported with mingw)", "TouCAN", "SysTec", "TCP (CanTcpGateway)" }, 1);
#endif
	int selectedID = 1;

	for (std::uint8_t i = 0; i < parentCANDrivers.size(); i++)
	{
		if ((nullptr != parentCANDrivers.at(i)) &&
		    (parentCANDrivers.at(i) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0)))
		{
			selectedID = i + 1;
			break;
		}
	}
	hardwareInterfaceSelector.setSelectedId(selectedID);
	hardwareInterfaceSelector.setSize(getWidth() - 20, 30);
	hardwareInterfaceSelector.setTopLeftPosition(10, 80);
	hardwareInterfaceSelector.onChange = [this]() {
		if (3 == hardwareInterfaceSelector.getSelectedId())
		{
			touCANSerialEditor.setVisible(true);
		}
		else
		{
			touCANSerialEditor.setVisible(false);
		}
		const bool tcpSelected = hardwareInterfaceSelector.getSelectedId() == static_cast<int>(parentCANDrivers.size());
		tcpHostEditor.setVisible(tcpSelected);
		tcpPortEditor.setVisible(tcpSelected);
		repaint();
	};
	addAndMakeVisible(hardwareInterfaceSelector);

	auto tcpDriver = std::static_pointer_cast<TcpCANPlugin>(parentCANDrivers.back());
	tcpHostEditor.setText(tcpDriver->get_host(), dontSendNotification);
	tcpHostEditor.setSize(getWidth() - 20, 30);
	tcpHostEditor.setTopLeftPosition(10, 140);
	tcpHostEditor.setVisible(false);
	addChildComponent(tcpHostEditor);
	tcpPortEditor.setText(std::to_string(tcpDriver->get_port()), dontSendNotification);
	tcpPortEditor.setSize(getWidth() - 20, 30);
	tcpPortEditor.setTopLeftPosition(10, 200);
	tcpPortEditor.setInputFilter(new TextEditor::LengthAndCharacterRestriction(5, "1234567890"), true);
	tcpPortEditor.setVisible(false);
	addChildComponent(tcpPortEditor);

	auto inputFilter = new TextEditor::LengthAndCharacterRestriction(10, "1234567890");
	touCANSerialEditor.setName("TouCAN Serial Number");
	touCANSerialEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(2))->get_serial_number()));
	touCANSerialEditor.setSize(getWidth() - 20, 30);
	touCANSerialEditor.setTopLeftPosition(10, 140);
	touCANSerialEditor.setInputFilter(inputFilter, true);
	addChildComponent(touCANSerialEditor);
#elif JUCE_LINUX
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");
	hardwareInterfaceSelector.addItemList({ "SocketCAN", "TCP (CanTcpGateway)" }, 1);
	hardwareInterfaceSelector.setSelectedId(parentCANDrivers.at(0) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0) ? 1 : 2);
	hardwareInterfaceSelector.setSize(getWidth() - 20, 30);
	hardwareInterfaceSelector.setTopLeftPosition(10, 80);
	hardwareInterfaceSelector.onChange = [this]() {
		const bool tcpSelected = hardwareInterfaceSelector.getSelectedId() == static_cast<int>(parentCANDrivers.size());
		tcpHostEditor.setVisible(tcpSelected);
		tcpPortEditor.setVisible(tcpSelected);
		socketCANNameEditor.setVisible(!tcpSelected);
		repaint();
	};
	addAndMakeVisible(hardwareInterfaceSelector);
	const auto tcpDriver = std::static_pointer_cast<TcpCANPlugin>(parentCANDrivers.back());
	tcpHostEditor.setText(tcpDriver->get_host(), dontSendNotification);
	tcpHostEditor.setSize(getWidth() - 20, 30);
	tcpHostEditor.setTopLeftPosition(10, 140);
	tcpHostEditor.setVisible(false);
	addChildComponent(tcpHostEditor);
	tcpPortEditor.setText(std::to_string(tcpDriver->get_port()), dontSendNotification);
	tcpPortEditor.setSize(getWidth() - 20, 30);
	tcpPortEditor.setTopLeftPosition(10, 200);
	tcpPortEditor.setInputFilter(new TextEditor::LengthAndCharacterRestriction(5, "1234567890"), true);
	tcpPortEditor.setVisible(false);
	addChildComponent(tcpPortEditor);
	socketCANNameEditor.setName("SocketCAN Interface Name");
	socketCANNameEditor.setText(std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->get_device_name());
	socketCANNameEditor.setSize(getWidth() - 20, 30);
	socketCANNameEditor.setTopLeftPosition(10, 140);
	addAndMakeVisible(socketCANNameEditor);
#endif
	hardwareInterfaceSelector.onChange();
	okButton.onClick = [this, &parent]() {
		if (hardwareInterfaceSelector.getSelectedId() == static_cast<int>(parentCANDrivers.size()))
		{
			const auto port = tcpPortEditor.getText().getIntValue();
			if (tcpHostEditor.getText().isEmpty() || port < 1 || port > 65535)
			{
				AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Invalid TCP settings", "Enter a host and a port between 1 and 65535.");
				return;
			}
			std::static_pointer_cast<TcpCANPlugin>(parentCANDrivers.back())->reconfigure(tcpHostEditor.getText().toStdString(), static_cast<std::uint16_t>(port));
		}
#ifdef JUCE_WINDOWS
		if (3 == hardwareInterfaceSelector.getSelectedId()) // TouCAN
		{
			int serial = touCANSerialEditor.getText().trim().getIntValue();
			std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1))->reconfigure(0, static_cast<std::uint32_t>(serial));
		}

		if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
		{
			isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
		}
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1));
		isobus::CANStackLogger::info("Updated assigned CAN driver.");
#elif JUCE_LINUX
		if (hardwareInterfaceSelector.getSelectedId() == 2)
		{
			if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
			{
				isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
			}
			isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.back());
		}
		else
		{
			std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->set_name(socketCANNameEditor.getText().toStdString());
			if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
			{
				isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
			}
			isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.at(0));
			isobus::CANStackLogger::info("Updated socket CAN interface name to: " + socketCANNameEditor.getText().toStdString());
		}
#endif
		parent.parentServer.save_settings();
		parent.exitModalState(1);
		parent.setVisible(false);
	};
}

void ConfigureHardwareComponent::paint(Graphics &graphics)
{
	auto bounds = getLocalBounds();
	graphics.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	graphics.setColour(getLookAndFeel().findColour(Label::textColourId));
	graphics.setFont(16.0f);
#ifdef JUCE_WINDOWS
		graphics.drawFittedText("Select the CAN driver to use", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#elif JUCE_LINUX
	graphics.drawFittedText("Select and configure the CAN transport", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#endif

	graphics.setFont(12.0f);

#ifdef JUCE_WINDOWS
	graphics.drawFittedText("Hardware Driver", hardwareInterfaceSelector.getBounds().getX(), hardwareInterfaceSelector.getBounds().getY() - 14, hardwareInterfaceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);

	if (3 == hardwareInterfaceSelector.getSelectedId())
	{
		graphics.drawFittedText("TouCAN Serial Number", touCANSerialEditor.getBounds().getX(), touCANSerialEditor.getBounds().getY() - 14, touCANSerialEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
	if (hardwareInterfaceSelector.getSelectedId() == static_cast<int>(parentCANDrivers.size()))
	{
		graphics.drawFittedText("TCP Host", tcpHostEditor.getBounds().getX(), tcpHostEditor.getBounds().getY() - 14, tcpHostEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
		graphics.drawFittedText("TCP Port", tcpPortEditor.getBounds().getX(), tcpPortEditor.getBounds().getY() - 14, tcpPortEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
#elif JUCE_LINUX
	graphics.drawFittedText("Hardware Driver", hardwareInterfaceSelector.getBounds().getX(), hardwareInterfaceSelector.getBounds().getY() - 14, hardwareInterfaceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	if (hardwareInterfaceSelector.getSelectedId() == 1)
	{
		graphics.drawFittedText("Socket CAN Interface Name", socketCANNameEditor.getBounds().getX(), socketCANNameEditor.getBounds().getY() - 14, socketCANNameEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
	else
	{
		graphics.drawFittedText("TCP Host", tcpHostEditor.getBounds().getX(), tcpHostEditor.getBounds().getY() - 14, tcpHostEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
		graphics.drawFittedText("TCP Port", tcpPortEditor.getBounds().getX(), tcpPortEditor.getBounds().getY() - 14, tcpPortEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
#endif
}

void ConfigureHardwareComponent::resized()
{
	okButton.setCentrePosition(getWidth() / 2, getHeight() - 30);
}
