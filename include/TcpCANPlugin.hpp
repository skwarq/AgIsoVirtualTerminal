#pragma once

#include "isobus/hardware_integration/can_hardware_plugin.hpp"

#include <JuceHeader.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

/// CAN hardware plugin which transports classic CAN frames over CanTcpGateway.
class TcpCANPlugin final : public isobus::CANHardwarePlugin
{
public:
	TcpCANPlugin(std::string host, std::uint16_t port);
	~TcpCANPlugin() override;

	std::string get_name() const override;
	bool get_is_valid() const override;
	bool is_connected() const;
	void close() override;
	void open() override;
	bool read_frame(isobus::CANMessageFrame &canFrame) override;
	bool write_frame(const isobus::CANMessageFrame &canFrame) override;

	const std::string &get_host() const;
	const std::string &get_configured_host() const;
	std::uint16_t get_configured_port() const;
	std::uint16_t get_port() const;
	void reconfigure(std::string host, std::uint16_t port);
	void use_discovered_endpoint(std::string host, std::uint16_t port);

private:
	bool read_exact(void *destination, int size);
	bool write_exact(const void *source, int size);
	bool connect_if_needed();
	void mark_disconnected();
	void mark_disconnected_locked();

	juce::StreamingSocket socket;
	std::string host;
	std::string configuredHost;
	std::uint16_t configuredPort = 0;
	std::uint16_t port;
	std::atomic_bool valid = false;
	std::atomic_bool openRequested = false;
	std::mutex writeMutex;
	std::mutex connectionMutex;
	std::chrono::steady_clock::time_point nextReconnectAttempt{};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TcpCANPlugin)
};
