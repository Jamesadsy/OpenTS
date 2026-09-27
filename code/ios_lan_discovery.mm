/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#import <Foundation/Foundation.h>

#include "ios_lan_discovery.h"

#include "landiagnostics.h"

#include <arpa/inet.h>
#include <dns_sd.h>
#include <algorithm>
#include <cerrno>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

namespace
{
constexpr NSTimeInterval NO_HOST_WINDOW_SECONDS = 4.0;
constexpr std::size_t MAX_PENDING_ENDPOINTS = 16;

std::mutex StateMutex;
IOSLanDiscovery::State DiscoveryState = IOSLanDiscovery::State::PERMISSION_NOT_REQUESTED;
std::vector<LANBootstrap::Endpoint> PendingEndpoints;

void Set_State(IOSLanDiscovery::State state, LANDiagnostics::Code code, int detail = 0)
{
	{
		std::lock_guard<std::mutex> lock(StateMutex);
		DiscoveryState = state;
	}
	LANDiagnostics::Record(LANDiagnostics::Layer::DISCOVERY, code, detail);
}

NSString * Bonjour_Service_Type()
{
	return([NSString stringWithFormat:@"%s.", IOSLanDiscovery::SERVICE_TYPE]);
}
}

@interface SecondSunBonjour : NSObject <NSNetServiceBrowserDelegate, NSNetServiceDelegate>
{
	NSNetServiceBrowser *Browser;
	NSNetService *Publication;
	NSMutableSet<NSNetService *> *Resolving;
	NSTimer *NoHostTimer;
}
- (void)startBrowsing;
- (void)stopBrowsing;
- (void)startAdvertising:(std::uint16_t)port;
- (void)stopAdvertising;
- (void)markAllowedWithoutHost:(NSTimer *)timer;
@end

@implementation SecondSunBonjour

- (void)startBrowsing
	{
		[self stopBrowsing];
		Resolving = [[NSMutableSet alloc] init];
		Browser = [[NSNetServiceBrowser alloc] init];
		Browser.delegate = self;
		Set_State(IOSLanDiscovery::State::REQUESTED,
			LANDiagnostics::Code::STARTED);
		[Browser searchForServicesOfType:Bonjour_Service_Type() inDomain:@"local."];
	}

- (void)stopBrowsing
	{
		[NoHostTimer invalidate];
		NoHostTimer = nil;
		if (Browser != nil) {
			[Browser stop];
			Browser.delegate = nil;
			Browser = nil;
		}
		for (NSNetService *service in Resolving) {
			[service stop];
			service.delegate = nil;
		}
		[Resolving removeAllObjects];
	}

- (void)startAdvertising:(std::uint16_t)port
	{
		[self stopAdvertising];
		if (port == 0) {
			Set_State(IOSLanDiscovery::State::DISCOVERY_ERROR,
				LANDiagnostics::Code::DISCOVERY_ERROR, EINVAL);
			return;
		}
		Publication = [[NSNetService alloc] initWithDomain:@"local."
			type:Bonjour_Service_Type() name:@"Second Sun" port:port];
		Publication.delegate = self;
		[Publication publish];
		LANDiagnostics::Record(LANDiagnostics::Layer::DISCOVERY,
			LANDiagnostics::Code::STARTED, port);
	}

- (void)stopAdvertising
	{
		if (Publication != nil) {
			[Publication stop];
			Publication.delegate = nil;
			Publication = nil;
		}
	}

	- (void)netServiceBrowserWillSearch:(NSNetServiceBrowser *)browser
	{
		(void)browser;
		Set_State(IOSLanDiscovery::State::SEARCHING, LANDiagnostics::Code::READY);
		[NoHostTimer invalidate];
		NoHostTimer = [NSTimer scheduledTimerWithTimeInterval:NO_HOST_WINDOW_SECONDS
			target:self selector:@selector(markAllowedWithoutHost:) userInfo:nil repeats:NO];
	}

	- (void)markAllowedWithoutHost:(NSTimer *)timer
	{
		(void)timer;
		bool no_endpoint = false;
		{
			std::lock_guard<std::mutex> lock(StateMutex);
			no_endpoint = DiscoveryState == IOSLanDiscovery::State::SEARCHING;
			if (no_endpoint) DiscoveryState = IOSLanDiscovery::State::ALLOWED_NO_HOST;
		}
		if (no_endpoint) {
			LANDiagnostics::Record(LANDiagnostics::Layer::DISCOVERY,
				LANDiagnostics::Code::ALLOWED_NO_HOST);
		}
	}

	- (void)netServiceBrowser:(NSNetServiceBrowser *)browser
		didFindService:(NSNetService *)service moreComing:(BOOL)moreComing
	{
		(void)browser;
		(void)moreComing;
		[NoHostTimer invalidate];
		NoHostTimer = nil;
		service.delegate = self;
		[Resolving addObject:service];
		[service resolveWithTimeout:5.0];
	}

	- (void)netServiceBrowser:(NSNetServiceBrowser *)browser
		didNotSearch:(NSDictionary<NSString *, NSNumber *> *)errorInfo
	{
		(void)browser;
		NSInteger const code = [errorInfo[NSNetServicesErrorCode] integerValue];
		bool const denied = code == kDNSServiceErr_PolicyDenied;
		Set_State(denied ? IOSLanDiscovery::State::DENIED : IOSLanDiscovery::State::DISCOVERY_ERROR,
			denied ? LANDiagnostics::Code::DENIED : LANDiagnostics::Code::DISCOVERY_ERROR,
			static_cast<int>(code));
	}

	- (void)netService:(NSNetService *)service didNotResolve:(NSDictionary<NSString *, NSNumber *> *)errorInfo
	{
		NSInteger const code = [errorInfo[NSNetServicesErrorCode] integerValue];
		Set_State(IOSLanDiscovery::State::DISCOVERY_ERROR, LANDiagnostics::Code::DISCOVERY_ERROR,
			static_cast<int>(code));
		[Resolving removeObject:service];
	}

	- (void)netServiceDidResolveAddress:(NSNetService *)service
	{
		int resolved = 0;
		for (NSData *data in service.addresses) {
			if (data.length < sizeof(sockaddr)) continue;
			sockaddr const *sa = static_cast<sockaddr const *>(data.bytes);
			if (sa->sa_family != AF_INET || data.length < sizeof(sockaddr_in)) continue;
			sockaddr_in const *address = static_cast<sockaddr_in const *>(sa);
			std::uint16_t const port = address->sin_port == 0
				? static_cast<std::uint16_t>(service.port) : ntohs(address->sin_port);
			LANBootstrap::Endpoint endpoint{address->sin_addr.s_addr, port};
			if (port == 0 || !LANBootstrap::Is_Private_IPv4(endpoint.Address)) continue;

			bool added = false;
			{
				std::lock_guard<std::mutex> lock(StateMutex);
				bool const duplicate = std::any_of(PendingEndpoints.begin(), PendingEndpoints.end(),
					[&endpoint](LANBootstrap::Endpoint const &found) {
						return(LANBootstrap::Same_Endpoint(found, endpoint));
					});
				if (!duplicate && PendingEndpoints.size() < MAX_PENDING_ENDPOINTS) {
					PendingEndpoints.push_back(endpoint);
					DiscoveryState = IOSLanDiscovery::State::ENDPOINT_FOUND;
					added = true;
				}
			}
			if (added) {
				LANDiagnostics::Record(LANDiagnostics::Layer::DISCOVERY,
					LANDiagnostics::Code::ENDPOINT_FOUND,
					LANBootstrap::Address_Class(endpoint.Address), endpoint.Port);
				LANDiagnostics::Record(LANDiagnostics::Layer::ADDRESS_SELECTION,
					LANDiagnostics::Code::ENDPOINT_FOUND,
					LANBootstrap::Address_Class(endpoint.Address), endpoint.Port);
				++resolved;
			}
		}
		if (resolved == 0) {
			LANDiagnostics::Record(LANDiagnostics::Layer::ADDRESS_SELECTION,
				LANDiagnostics::Code::ENDPOINT_REJECTED);
		}
		[Resolving removeObject:service];
	}

	- (void)netService:(NSNetService *)service didNotPublish:(NSDictionary<NSString *, NSNumber *> *)errorInfo
	{
		NSInteger const code = [errorInfo[NSNetServicesErrorCode] integerValue];
		Set_State(IOSLanDiscovery::State::DISCOVERY_ERROR, LANDiagnostics::Code::DISCOVERY_ERROR,
			static_cast<int>(code));
		(void)service;
	}
@end

namespace
{
SecondSunBonjour * Service = nil;
}

namespace IOSLanDiscovery
{
void Start_Browsing()
{
	if (Service == nil) Service = [[SecondSunBonjour alloc] init];
	[Service startBrowsing];
}

void Stop_Browsing()
{
	[Service stopBrowsing];
	std::lock_guard<std::mutex> lock(StateMutex);
	PendingEndpoints.clear();
}

void Start_Advertising(std::uint16_t udp_port)
{
	if (Service == nil) Service = [[SecondSunBonjour alloc] init];
	[Service startAdvertising:udp_port];
}

void Stop_Advertising()
{
	[Service stopAdvertising];
}

bool Pop_Endpoint(LANBootstrap::Endpoint & endpoint)
{
	std::lock_guard<std::mutex> lock(StateMutex);
	if (PendingEndpoints.empty()) return(false);
	endpoint = PendingEndpoints.front();
	PendingEndpoints.erase(PendingEndpoints.begin());
	return(true);
}

State Current_State()
{
	std::lock_guard<std::mutex> lock(StateMutex);
	return(DiscoveryState);
}

char const * Status_Text()
{
	switch (Current_State()) {
		case State::PERMISSION_NOT_REQUESTED: return("Local network permission has not been requested.");
		case State::REQUESTED: return("Checking local network access…");
		case State::SEARCHING: return("Looking for Second Sun games on this Wi-Fi network…");
		case State::ALLOWED_NO_HOST: return("Local network access is available; no host was found.");
		case State::DENIED: return("Local network access is denied. Check iOS Settings for Second Sun.");
		case State::DISCOVERY_ERROR: return("Bonjour discovery failed. You can enter a host IPv4 address.");
		case State::ENDPOINT_FOUND: return("Second Sun host found; querying its game list.");
		case State::UDP_ERROR: return("UDP transport could not start. Check the local network diagnostic.");
		default: return("Bonjour discovery status is unavailable.");
	}
}

void Record_UDP_Error(int error_code)
{
	Set_State(State::UDP_ERROR, LANDiagnostics::Code::UDP_TRANSPORT_ERROR, error_code);
}
}
