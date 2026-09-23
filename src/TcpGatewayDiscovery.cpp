#include "TcpGatewayDiscovery.hpp"

#include <array>
#include <chrono>
#include <cstring>
#include <memory>
#include <set>
#include <vector>

TcpGatewayDiscovery::TcpGatewayDiscovery() :
  juce::Thread("CAN TCP gateway discovery") {}

TcpGatewayDiscovery::~TcpGatewayDiscovery()
{
	stop();
}

bool TcpGatewayDiscovery::start()
{
	stop();
	{
		std::lock_guard lock(mutex);
		endpoint.reset();
	}
	return startThread();
}

std::optional<TcpGatewayDiscovery::Endpoint> TcpGatewayDiscovery::get_endpoint() const
{
	std::lock_guard lock(mutex);
	return endpoint;
}

void TcpGatewayDiscovery::run()
{
	constexpr int discoveryPort = 29501;
	constexpr auto requestInterval = std::chrono::milliseconds(3000);
	constexpr auto responseTimeout = std::chrono::milliseconds(1000);
	while (!threadShouldExit())
	{
		std::array<std::uint8_t, 24> request{};
		request[0] = 0x43;
		request[1] = 0x54;
		request[2] = 0x47;
		request[3] = 0x31;
		request[4] = 1;
		request[5] = 1;
		const juce::Uuid token;
		std::memcpy(request.data() + 8, token.getRawData(), juce::Uuid::size());
		std::vector<std::unique_ptr<juce::DatagramSocket>> sockets;
		std::set<std::string> boundAddresses;
		for (const auto &address : juce::IPAddress::getAllAddresses())
		{
			const auto addressText = address.toString().toStdString();
			if (address.isIPv6 || address.isNull() || !boundAddresses.insert(addressText).second)
				continue;
			auto socket = std::make_unique<juce::DatagramSocket>(true);
			if (socket->bindToPort(0, address.toString()))
				sockets.push_back(std::move(socket));
		}
		if (sockets.empty())
		{
			auto socket = std::make_unique<juce::DatagramSocket>(true);
			if (!socket->bindToPort(0))
			{
				const auto retryAt = std::chrono::steady_clock::now() + requestInterval;
				while (!threadShouldExit() && std::chrono::steady_clock::now() < retryAt)
					wait(100);
				continue;
			}
			sockets.push_back(std::move(socket));
		}
		const auto requestStarted = std::chrono::steady_clock::now();
		for (auto &socket : sockets)
			socket->write(juce::IPAddress::broadcast().toString(), discoveryPort, request.data(), static_cast<int>(request.size()));

		const auto responseDeadline = requestStarted + responseTimeout;
		while (!threadShouldExit() && std::chrono::steady_clock::now() < responseDeadline)
		{
			for (auto &socket : sockets)
			{
				if (socket->waitUntilReady(true, 0) != 1)
					continue;
				std::array<std::uint8_t, 64> response{};
				juce::String senderAddress;
				int senderPort = 0;
				const auto bytesRead = socket->read(response.data(), static_cast<int>(response.size()), false, senderAddress, senderPort);
				constexpr std::size_t responseSize = 44;
				if (bytesRead != static_cast<int>(responseSize) || std::memcmp(response.data(), "CTG1", 4) != 0 || response[4] != 1 || response[5] != 2 || std::memcmp(response.data() + 8, request.data() + 8, juce::Uuid::size()) != 0)
					continue;

				const auto port = static_cast<std::uint16_t>((static_cast<std::uint16_t>(response[24]) << 8) | response[25]);
				const auto addressFamily = response[26];
				const auto addressLength = response[27];
				const bool ipv4 = addressFamily == 2 && addressLength == 4;
				const bool ipv6 = addressLength == 16 && (addressFamily == 10 || addressFamily == 23 || addressFamily == 30);
				if (port == 0 || (!ipv4 && !ipv6))
					continue;
				const juce::IPAddress address(response.data() + 28, ipv6);
				if (address.isNull())
					continue;

				std::lock_guard lock(mutex);
				endpoint = Endpoint{ address.toString().toStdString(), port };
				return;
			}
			wait(10);
		}

		const auto nextRequest = requestStarted + requestInterval;
		while (!threadShouldExit() && std::chrono::steady_clock::now() < nextRequest)
			wait(100);
	}
}

void TcpGatewayDiscovery::stop()
{
	stopThread(2000);
	std::lock_guard lock(mutex);
	endpoint.reset();
}
