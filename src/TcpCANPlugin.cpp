#include "TcpCANPlugin.hpp"

#include "isobus/isobus/can_stack_logger.hpp"

#include <array>
#include <cstring>

namespace
{
constexpr int wireFrameSize = 16;
constexpr std::uint32_t extendedFrameFlag = 0x80000000U;

std::uint32_t readBigEndian32(const std::uint8_t *data)
{
	return (static_cast<std::uint32_t>(data[0]) << 24) |
	       (static_cast<std::uint32_t>(data[1]) << 16) |
	       (static_cast<std::uint32_t>(data[2]) << 8) |
	       static_cast<std::uint32_t>(data[3]);
}

void writeBigEndian32(std::uint8_t *data, std::uint32_t value)
{
	data[0] = static_cast<std::uint8_t>(value >> 24);
	data[1] = static_cast<std::uint8_t>(value >> 16);
	data[2] = static_cast<std::uint8_t>(value >> 8);
	data[3] = static_cast<std::uint8_t>(value);
}
} // namespace

TcpCANPlugin::TcpCANPlugin(std::string host, std::uint16_t port) : host(std::move(host)), port(port)
{
}

TcpCANPlugin::~TcpCANPlugin()
{
	close();
}

std::string TcpCANPlugin::get_name() const
{
	return "TCP (CanTcpGateway)";
}

bool TcpCANPlugin::get_is_valid() const
{
	// Keep the CAN receive worker active while reconnecting. A failed initial
	// connect must not prevent read_frame() from making the next retry.
	return openRequested.load();
}

void TcpCANPlugin::close()
{
	std::lock_guard lock(connectionMutex);
	openRequested.store(false);
	if (valid.exchange(false))
	{
		isobus::CANStackLogger::info("CAN TCP gateway connection closed: " + host + ":" + std::to_string(port));
	}
	socket.close();
}

void TcpCANPlugin::mark_disconnected()
{
	if (valid.exchange(false))
	{
		isobus::CANStackLogger::info("CAN TCP gateway connection lost: " + host + ":" + std::to_string(port) + "; retrying");
	}
	socket.close();
}

void TcpCANPlugin::open()
{
	openRequested.store(true);
	close();
	openRequested.store(true);
	connect_if_needed();
}

bool TcpCANPlugin::connect_if_needed()
{
	std::lock_guard lock(connectionMutex);
	if (valid.load())
	{
		return true;
	}

	const auto now = std::chrono::steady_clock::now();
	if (now < nextReconnectAttempt)
	{
		return false;
	}
	nextReconnectAttempt = now + std::chrono::seconds(2);

	socket.close();
	if (socket.connect(host, static_cast<int>(port), 250))
	{
		valid.store(true);
		isobus::CANStackLogger::info("Connected to CAN TCP gateway at " + host + ":" + std::to_string(port));
		return true;
	}
	return false;
}

bool TcpCANPlugin::read_exact(void *destination, int size)
{
	if (!valid.load())
	{
		return false;
	}

	auto *bytes = static_cast<std::uint8_t *>(destination);
	int offset = 0;
	while (offset < size)
	{
		if (!socket.waitUntilReady(true, 100))
		{
			return false;
		}
		const auto count = socket.read(bytes + offset, size - offset, false);
		if (count <= 0)
		{
			mark_disconnected();
			return false;
		}
		offset += count;
	}
	return true;
}

bool TcpCANPlugin::write_exact(const void *source, int size)
{
	std::lock_guard lock(writeMutex);
	if (!valid.load())
	{
		return false;
	}

	if (!socket.write(source, size))
	{
		mark_disconnected();
		return false;
	}
	return true;
}

bool TcpCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
{
	if (!connect_if_needed())
	{
		juce::Thread::sleep(25);
		return false;
	}

	std::array<std::uint8_t, wireFrameSize> packet{};
	if (!read_exact(packet.data(), static_cast<int>(packet.size())))
	{
		return false;
	}

	const auto encodedId = readBigEndian32(packet.data());
	const auto dataLength = packet[4];
	if (dataLength > 8)
	{
		mark_disconnected();
		return false;
	}

	canFrame.timestamp_us = static_cast<std::uint64_t>(juce::Time::getMillisecondCounterHiRes() * 1000.0);
	canFrame.identifier = encodedId & 0x1FFFFFFFU;
	canFrame.channel = 0;
	canFrame.dataLength = dataLength;
	canFrame.isExtendedFrame = (encodedId & extendedFrameFlag) != 0;
	std::memcpy(canFrame.data, packet.data() + 8, dataLength);
	return true;
}

bool TcpCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
{
	if (canFrame.dataLength > 8)
	{
		return false;
	}

	std::array<std::uint8_t, wireFrameSize> packet{};
	const auto encodedId = canFrame.identifier | (canFrame.isExtendedFrame ? extendedFrameFlag : 0U);
	writeBigEndian32(packet.data(), encodedId);
	packet[4] = canFrame.dataLength;
	std::memcpy(packet.data() + 8, canFrame.data, canFrame.dataLength);
	return write_exact(packet.data(), static_cast<int>(packet.size()));
}

const std::string &TcpCANPlugin::get_host() const
{
	return host;
}

std::uint16_t TcpCANPlugin::get_port() const
{
	return port;
}

void TcpCANPlugin::reconfigure(std::string newHost, std::uint16_t newPort)
{
	close();
	host = std::move(newHost);
	port = newPort;
}
