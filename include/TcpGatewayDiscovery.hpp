#pragma once

#include <JuceHeader.h>

#include <mutex>
#include <optional>
#include <string>

class TcpGatewayDiscovery : private juce::Thread
{
public:
	struct Endpoint
	{
		std::string host;
		std::uint16_t port;
	};

	TcpGatewayDiscovery();
	~TcpGatewayDiscovery();

	bool start();
	std::optional<Endpoint> get_endpoint() const;
	void stop();

private:
	void run() override;
	mutable std::mutex mutex;
	std::optional<Endpoint> endpoint;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TcpGatewayDiscovery)
};
