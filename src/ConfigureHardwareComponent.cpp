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
#elif JUCE_LINUX && !JUCE_ANDROID
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

ConfigureHardwareComponent::ConfigureHardwareComponent(ConfigureHardwareWindow &parent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  parentCANDrivers(canDrivers),
  parentWindow(parent)
{
	setOpaque(false);
	hardwareInterfaceSelector.setJustificationType(Justification::centred);
	socketCANNameEditor.setFont(Font(FontOptions{}.withHeight(16.0f)));
	tcpHostEditor.setFont(Font(FontOptions{}.withHeight(16.0f)));
	tcpPortEditor.setFont(Font(FontOptions{}.withHeight(16.0f)));
	touCANSerialEditor.setFont(Font(FontOptions{}.withHeight(16.0f)));

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
	tcpHostEditor.setVisible(false);
	addChildComponent(tcpHostEditor);
	tcpPortEditor.setText(std::to_string(tcpDriver->get_port()), dontSendNotification);
	tcpPortEditor.setInputFilter(new TextEditor::LengthAndCharacterRestriction(5, "1234567890"), true);
	tcpPortEditor.setVisible(false);
	addChildComponent(tcpPortEditor);

	auto inputFilter = new TextEditor::LengthAndCharacterRestriction(10, "1234567890");
	touCANSerialEditor.setName("TouCAN Serial Number");
	touCANSerialEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(2))->get_serial_number()));
	touCANSerialEditor.setInputFilter(inputFilter, true);
	addChildComponent(touCANSerialEditor);
#elif JUCE_LINUX && !JUCE_ANDROID
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");
	hardwareInterfaceSelector.addItemList({ "SocketCAN", "TCP (CanTcpGateway)" }, 1);
	hardwareInterfaceSelector.setSelectedId(parentCANDrivers.at(0) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0) ? 1 : 2);
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
	tcpHostEditor.setVisible(false);
	addChildComponent(tcpHostEditor);
	tcpPortEditor.setText(std::to_string(tcpDriver->get_port()), dontSendNotification);
	tcpPortEditor.setInputFilter(new TextEditor::LengthAndCharacterRestriction(5, "1234567890"), true);
	tcpPortEditor.setVisible(false);
	addChildComponent(tcpPortEditor);
	socketCANNameEditor.setName("SocketCAN Interface Name");
	socketCANNameEditor.setText(std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->get_device_name());
	addAndMakeVisible(socketCANNameEditor);
#elif JUCE_ANDROID
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");
	hardwareInterfaceSelector.addItem("TCP (CanTcpGateway)", 1);
	hardwareInterfaceSelector.setSelectedId(1, dontSendNotification);
	hardwareInterfaceSelector.onChange = [this]() {
		tcpHostEditor.setVisible(true);
		tcpPortEditor.setVisible(true);
		repaint();
	};
	addAndMakeVisible(hardwareInterfaceSelector);
	const auto tcpDriver = std::static_pointer_cast<TcpCANPlugin>(parentCANDrivers.front());
	tcpHostEditor.setText(tcpDriver->get_host(), dontSendNotification);
	tcpHostEditor.setVisible(true);
	addAndMakeVisible(tcpHostEditor);
	tcpPortEditor.setText(std::to_string(tcpDriver->get_port()), dontSendNotification);
	tcpPortEditor.setInputFilter(new TextEditor::LengthAndCharacterRestriction(5, "1234567890"), true);
	tcpPortEditor.setVisible(true);
	addAndMakeVisible(tcpPortEditor);
#endif
	hardwareInterfaceSelector.onChange();
	const auto initialSize = preferredSize();
	setSize(initialSize.x, initialSize.y);
	/* The dialog owns the OK/Cancel buttons; this component only owns the form. */
	/* Configuration is applied through applyConfiguration(). */
	}

bool ConfigureHardwareComponent::applyConfiguration()
	{
		if (hardwareInterfaceSelector.getSelectedId() == static_cast<int>(parentCANDrivers.size()))
		{
			const auto port = tcpPortEditor.getText().getIntValue();
			if (tcpHostEditor.getText().isEmpty() || port < 1 || port > 65535)
			{
				AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, "Invalid TCP settings", "Enter a host and a port between 1 and 65535.");
				return false;
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
#elif JUCE_LINUX && !JUCE_ANDROID
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
#elif JUCE_ANDROID
		if (hardwareInterfaceSelector.getSelectedId() == 1)
		{
			if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
			{
				isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
			}
			isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.front());
			isobus::CANStackLogger::info("Updated Android TCP CAN transport to " + tcpHostEditor.getText().toStdString() + ":" + tcpPortEditor.getText().toStdString());
		}
#endif
		parentWindow.parentServer.save_settings();
		return true;
}

void ConfigureHardwareComponent::paint(Graphics &graphics)
{
	auto bounds = getLocalBounds();
	graphics.setColour(getLookAndFeel().findColour(Label::textColourId));
	graphics.setFont(16.0f);
	graphics.drawFittedText("Select and configure the CAN transport", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 1);

	graphics.setFont(Font(FontOptions{}.withHeight(16.0f)));

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
#elif JUCE_LINUX && !JUCE_ANDROID
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
#elif JUCE_ANDROID
	graphics.drawFittedText("Hardware Driver", hardwareInterfaceSelector.getBounds().getX(), hardwareInterfaceSelector.getBounds().getY() - 14, hardwareInterfaceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	graphics.drawFittedText("TCP Host", tcpHostEditor.getBounds().getX(), tcpHostEditor.getBounds().getY() - 14, tcpHostEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	graphics.drawFittedText("TCP Port", tcpPortEditor.getBounds().getX(), tcpPortEditor.getBounds().getY() - 14, tcpPortEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
#endif
}

void ConfigureHardwareComponent::resized()
{
	const int fieldWidth = juce::jmax(80, getWidth() - 20);
	const int rowSpacing = ResponsiveDialogWindow::dialogFieldSpacing + 16;
	int nextY = 56;
	hardwareInterfaceSelector.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight);
	nextY += ResponsiveDialogWindow::dialogFieldHeight + rowSpacing;

#ifdef JUCE_WINDOWS
	if (touCANSerialEditor.isVisible()) { touCANSerialEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight); nextY += ResponsiveDialogWindow::dialogFieldHeight + rowSpacing; }
	if (tcpHostEditor.isVisible()) { tcpHostEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight); nextY += ResponsiveDialogWindow::dialogFieldHeight + rowSpacing; }
	if (tcpPortEditor.isVisible()) { tcpPortEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight); }
#elif JUCE_LINUX && !JUCE_ANDROID
	if (socketCANNameEditor.isVisible()) { socketCANNameEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight); nextY += ResponsiveDialogWindow::dialogFieldHeight + rowSpacing; }
	if (tcpHostEditor.isVisible()) { tcpHostEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight); nextY += ResponsiveDialogWindow::dialogFieldHeight + rowSpacing; }
	if (tcpPortEditor.isVisible()) { tcpPortEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight); }
#elif JUCE_ANDROID
	tcpHostEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight); nextY += ResponsiveDialogWindow::dialogFieldHeight + rowSpacing;
	tcpPortEditor.setBounds(10, nextY, fieldWidth, ResponsiveDialogWindow::dialogFieldHeight);
#endif

}

juce::Point<int> ConfigureHardwareComponent::preferredSize() const
{
	int rows = 1;
#ifdef JUCE_WINDOWS
	rows += touCANSerialEditor.isVisible() ? 1 : 0;
	rows += tcpHostEditor.isVisible() ? 1 : 0;
	rows += tcpPortEditor.isVisible() ? 1 : 0;
#elif JUCE_LINUX && !JUCE_ANDROID
	rows += socketCANNameEditor.isVisible() ? 1 : 0;
	rows += tcpHostEditor.isVisible() ? 1 : 0;
	rows += tcpPortEditor.isVisible() ? 1 : 0;
#elif JUCE_ANDROID
	rows += 2;
#endif
	const int rowHeight = ResponsiveDialogWindow::dialogFieldHeight + ResponsiveDialogWindow::dialogFieldSpacing + 16;
	const int lastFieldEnd = 56 + rows * rowHeight - 30;
	return { 400, lastFieldEnd + ResponsiveDialogWindow::contentToButtonGap };
}
