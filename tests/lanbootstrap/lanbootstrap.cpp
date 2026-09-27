/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "always.h"

#include "ipxgconn.h"
#include "lanbootstrap.h"
#include "landiagnostics.h"
#include "netsocket.h"
#include "netglobal.h"
#include "wspudp.h"
#include "_wsproto.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <memory>

int WestwoodOnline_PortNumber = 1234;
WinsockInterfaceClass *PacketTransport = nullptr;

namespace
{
int Failures = 0;

void Check(bool condition, char const * message)
{
	std::printf("%-78s %s\n", message, condition ? "ok" : "FAILED");
	if (!condition) ++Failures;
}

bool Is_Query_Game_Datagram(NullSocketClass::Datagram const & datagram)
{
	std::size_t const payload_offset = sizeof(std::uint32_t) + sizeof(GlobalHeaderType);
	if (datagram.Bytes.size() < payload_offset + sizeof(GlobalPacketType)) return(false);
	GlobalPacketType packet = {};
	std::memcpy(&packet, datagram.Bytes.data() + payload_offset, sizeof(packet));
	return(packet.Command == NET_QUERY_GAME);
}

struct PeerManagerFixture
{
	explicit PeerManagerFixture(UDPInterfaceClass &transport) : Transport(transport) {}
	void Add_Peer(IPXAddressClass const &address)
	{
		++Calls;
		Transport.Set_Broadcast_Address(address);
	}
	UDPInterfaceClass &Transport;
	int Calls = 0;
};

void Test_Manual_Address_Validation(void)
{
	LANBootstrap::Endpoint endpoint;
	LANBootstrap::ParseError error = LANBootstrap::ParseError::NONE;
	bool const accepted = LANBootstrap::Parse_Private_IPv4("192.168.4.25", 1234, endpoint, &error);
	Check(accepted && endpoint.Port == 1234 && LANBootstrap::Address_Class(endpoint.Address) == 192,
		"a canonical RFC1918 IPv4 address becomes a concrete UDP endpoint");
	IPXAddressClass const address = LANBootstrap::To_IPX_Address(endpoint);
	Check(address.Get_IP() == endpoint.Address && address.Get_Port() == Socket_Network_Port(1234),
		"the endpoint enters the existing IPX address representation without changing the protocol");

	std::array<char const *, 7> const rejected = {
		"", "8.8.8.8", "127.0.0.1", "169.254.1.5", "192.168.4.256",
		"192.168.04.25", "192.168.4.25 trailing"
	};
	bool all_rejected = true;
	for (char const *candidate : rejected) {
		all_rejected = all_rejected
			&& !LANBootstrap::Parse_Private_IPv4(candidate, 1234, endpoint, &error);
	}
	all_rejected = all_rejected && !LANBootstrap::Parse_Private_IPv4("10.0.0.1", 0, endpoint, &error);
	Check(all_rejected, "public, loopback, link-local, malformed, non-canonical, and zero-port inputs are rejected");
}

void Test_Direct_Peer_Bootstrap_And_Query(void)
{
	auto socket = std::make_unique<NullSocketClass>();
	NullSocketClass * const observed = socket.get();
	UDPInterfaceClass transport;
	transport.Set_Socket(std::move(socket));
	transport.Set_Local_Port(50000);
	transport.Set_Destination_Port(1234);
	Check(transport.Open_Socket(), "the deterministic UDP transport opens without using the host network");
	PacketTransport = &transport;

	LANBootstrap::Endpoint endpoint;
	bool const parsed = LANBootstrap::Parse_Private_IPv4("192.168.4.25", 1234, endpoint);
	PeerManagerFixture manager(transport);
	if (parsed) {
		LANBootstrap::Inject_Direct_Peer(manager, endpoint);
		LANBootstrap::Inject_Direct_Peer(manager, endpoint);
	}
	Check(parsed && manager.Calls == 2,
		"the resolved/manual endpoint uses the same Add_Peer injection seam; duplicate injection is safe");

	GlobalPacketType query = {};
	query.Command = NET_QUERY_GAME;
	int const global_packet_capacity = static_cast<int>(sizeof(query) + sizeof(GlobalHeaderType));
	IPXGlobalConnClass global(8, 8, global_packet_capacity,
		IPXGlobalConnClass::COMMAND_AND_CONQUER2);
	global.Init();
	global.Set_Retry_Delta(0);
	global.Set_Max_Retries(4);
	int const queued = global.Send_Packet(&query, sizeof(query), nullptr, 0);
	global.Service();
	transport.Service();
	bool const one_query = queued != 0 && observed->Sent().size() == 1
		&& observed->Sent()[0].Address.Get_IP() == endpoint.Address
		&& observed->Sent()[0].Address.Get_Port() == Socket_Network_Port(1234)
		&& Is_Query_Game_Datagram(observed->Sent()[0]);
	Check(one_query,
		"NET_QUERY_GAME fans out only to the one injected concrete peer, with no raw UDP broadcast");
	Check(!observed->Broadcast_Enabled(),
		"direct-peer discovery succeeds while the UDP socket broadcast option remains disabled");

	observed->Clear_Sent();
	IPXAddressClass peer = LANBootstrap::To_IPX_Address(endpoint);
	int const reliable_queued = global.Send_Packet(&query, sizeof(query), &peer, 1);
	global.Service();
	global.Service();
	transport.Service();
	bool preserved = reliable_queued != 0 && observed->Sent().size() >= 2;
	for (NullSocketClass::Datagram const &datagram : observed->Sent()) {
		preserved = preserved && datagram.Address.Get_IP() == endpoint.Address
			&& datagram.Address.Get_Port() == Socket_Network_Port(1234)
			&& Is_Query_Game_Datagram(datagram);
	}
	Check(preserved,
		"the explicit peer destination survives global queueing, reliable send, retry, and UDP dispatch");

	PacketTransport = nullptr;
}

void Test_Diagnostic_Ring_Is_Bounded(void)
{
	LANDiagnostics::Reset_For_Test();
	for (int index = 0; index < 70; ++index) {
		LANDiagnostics::Record(LANDiagnostics::Layer::DISCOVERY,
			LANDiagnostics::Code::READY, index);
	}
	std::array<LANDiagnostics::Event, LANDiagnostics::CAPACITY> events{};
	std::size_t const count = LANDiagnostics::Copy_Last(events.data(), events.size());
	Check(count == LANDiagnostics::CAPACITY && events.front().Detail0 == 6
		&& events.back().Detail0 == 69 && events.front().Sequence == 7,
		"structured diagnostics retain only the newest 64 numeric events in order");
	Check(std::strcmp(LANDiagnostics::Layer_Name(LANDiagnostics::Layer::SOCKET_BIND), "SOCKET_BIND") == 0
		&& std::strcmp(LANDiagnostics::Layer_Name(LANDiagnostics::Layer::DESYNC), "DESYNC") == 0,
		"diagnostic layer names are stable and contain no device identifier or secret");
}
}

int main(void)
{
	Test_Manual_Address_Validation();
	Test_Direct_Peer_Bootstrap_And_Query();
	Test_Diagnostic_Ring_Is_Bounded();
	return(Failures == 0 ? 0 : 1);
}
