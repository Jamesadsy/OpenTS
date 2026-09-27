/*******************************************************************************
 *                                O P E N T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 *******************************************************************************/

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace
{

int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-76s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) {
		Failures++;
	}
}


std::string Read_Source(char const * path)
{
	std::ifstream source(std::string(OPENTS_SOURCE_DIR) + path);
	return(std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()));
}


std::string Value_For_Key(std::string const & plist, char const * key)
{
	std::string const key_tag = std::string("<key>") + key + "</key>";
	size_t const key_position = plist.find(key_tag);
	if (key_position == std::string::npos) {
		return(std::string());
	}

	size_t const value_start = plist.find("<string>", key_position + key_tag.size());
	if (value_start == std::string::npos) {
		return(std::string());
	}
	size_t const content_start = value_start + std::string("<string>").size();
	size_t const value_end = plist.find("</string>", content_start);
	if (value_end == std::string::npos) {
		return(std::string());
	}
	return(plist.substr(content_start, value_end - content_start));
}


void Test_Display_Name_And_Stable_Identity(void)
{
	std::string const plist = Read_Source("/cmake/ios/Info.plist.in");
	std::string const cmake = Read_Source("/code/CMakeLists.txt");
	std::string const root_cmake = Read_Source("/CMakeLists.txt");
	std::string const discovery_header = Read_Source("/code/ios_lan_discovery.h");
	std::string const discovery_source = Read_Source("/code/ios_lan_discovery.mm");
	std::string const ipx_manager = Read_Source("/code/ipxmgr.cpp");
	std::string const init = Read_Source("/code/init.cpp");
	std::string const stats = Read_Source("/code/stats.cpp");
	std::string const posix_socket = Read_Source("/code/netsocket_posix.cpp");
	std::string const win32_socket = Read_Source("/code/netsocket_win32.cpp");
	std::string const package = Read_Source("/scripts/build/ios/package-ios.sh");
	Check(!plist.empty() && !cmake.empty() && !package.empty() && !root_cmake.empty()
		&& !discovery_header.empty() && !discovery_source.empty()
		&& !ipx_manager.empty() && !init.empty() && !stats.empty()
		&& !posix_socket.empty() && !win32_socket.empty(), "iOS bundle, networking, and packaging sources are readable");

	Check(Value_For_Key(plist, "CFBundleDisplayName") == "CnC TS",
		"the iOS CFBundleDisplayName resolves to CnC TS");
	Check(Value_For_Key(plist, "CFBundleName") == "${MACOSX_BUNDLE_BUNDLE_NAME}"
		&& cmake.find("MACOSX_BUNDLE_BUNDLE_NAME \"OpenTS\"") != std::string::npos,
		"the internal bundle name remains OpenTS");
	Check(Value_For_Key(plist, "CFBundleIdentifier") == "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
		&& cmake.find("set(OPENTS_IOS_BUNDLE_ID \"org.opents.OpenTS\" CACHE STRING \"iOS bundle identifier\")")
			!= std::string::npos
		&& cmake.find("MACOSX_BUNDLE_GUI_IDENTIFIER \"${OPENTS_IOS_BUNDLE_ID}\"") != std::string::npos
		&& cmake.find("XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER \"${OPENTS_IOS_BUNDLE_ID}\"")
			!= std::string::npos,
		"the bundle identifier source remains org.opents.OpenTS");
	Check(Value_For_Key(plist, "CFBundleExecutable") == "${MACOSX_BUNDLE_EXECUTABLE_NAME}"
		&& cmake.find("add_executable(OpenTS WIN32 ${OPENTS_SRC})") != std::string::npos
		&& cmake.find("OUTPUT_NAME \"OpenTS\"\n        OUTPUT_NAME_DEBUG \"OpenTS\"\n        OUTPUT_NAME_RELEASE \"OpenTS\"")
			!= std::string::npos,
		"the target, executable, and iOS output names remain OpenTS");
	Check(package.find("--destination \"Documents/OpenTS\"") != std::string::npos,
		"the app data destination remains Documents/OpenTS");

	Check(Value_For_Key(plist, "NSLocalNetworkUsageDescription")
		== "Second Sun uses your local Wi-Fi network to find and join multiplayer games hosted on the same private network.",
		"the Local Network purpose string explains same-Wi-Fi multiplayer");
	Check(Value_For_Key(plist, "NSBonjourServices") == "_secondsun._udp"
		&& plist.find("<key>NSBonjourServices</key>\n  <array>\n    <string>_secondsun._udp</string>\n  </array>") != std::string::npos,
		"the plist declares only the stable Second Sun Bonjour service type");
	Check(plist.find("com.apple.developer.networking.multicast") == std::string::npos,
		"the iOS plist introduces no restricted multicast entitlement");
	Check(cmake.find("ios_lan_discovery.mm") != std::string::npos
		&& cmake.find("FOUNDATION_FRAMEWORK Foundation") != std::string::npos
		&& root_cmake.find("enable_language(OBJCXX)") != std::string::npos
		&& cmake.find("find_library(NETWORK_FRAMEWORK") == std::string::npos
		&& cmake.find("${NETWORK_FRAMEWORK}") == std::string::npos
		&& discovery_source.find("Network/Network.h") == std::string::npos,
		"the iOS profile links Foundation for discovery without adding Network.framework");
	Check(discovery_header.find("SERVICE_TYPE[] = \"_secondsun._udp\"") != std::string::npos
		&& discovery_source.find("searchForServicesOfType:Bonjour_Service_Type()") != std::string::npos
		&& discovery_source.find("type:Bonjour_Service_Type()") != std::string::npos,
		"the Bonjour browser and host both use the one Second Sun UDP service type");
	size_t const direct_start = ipx_manager.find("void IPXManagerClass::Configure_Direct_Peers");
	size_t const add_peer_start = ipx_manager.find("void IPXManagerClass::Add_Peer", direct_start);
	size_t const shutdown_start = ipx_manager.find("void IPXManagerClass::Shutdown", add_peer_start);
	bool direct_peer_core = false;
	if (direct_start != std::string::npos && add_peer_start != std::string::npos
		&& shutdown_start != std::string::npos) {
		std::string const configure = ipx_manager.substr(direct_start, add_peer_start - direct_start);
		std::string const add_peer = ipx_manager.substr(add_peer_start, shutdown_start - add_peer_start);
		direct_peer_core = configure.find("Set_Local_Port(listen_port)") != std::string::npos
			&& configure.find("Set_Destination_Port(listen_port)") != std::string::npos
			&& configure.find("Enable_Broadcast") == std::string::npos
			&& add_peer.find("Set_Broadcast_Address(address)") != std::string::npos;
	}
	size_t const ios_switch = init.find("#if defined(OPENTS_IOS)",
		init.find("Session.CommProtocol = COMM_PROTOCOL_MULTI_E_COMP;"));
	size_t const ios_switch_end = init.find("#endif", ios_switch);
	bool ios_selects_direct_peers = false;
	if (ios_switch != std::string::npos && ios_switch_end != std::string::npos) {
		std::string const setup = init.substr(ios_switch, ios_switch_end - ios_switch);
		ios_selects_direct_peers = setup.find("Configure_Direct_Peers") != std::string::npos
			&& setup.find("Configure_LAN()") != std::string::npos;
	}
	Check(direct_peer_core && ios_selects_direct_peers,
		"iOS selects OpenTS direct peers on the configured UDP port without enabling broadcast");
	Check(stats.find("int WestwoodOnline_PortNumber = 1234;") != std::string::npos,
		"the resolved OpenTS default UDP port is 1234");
	Check(posix_socket.find("#if !defined(_WIN32)") != std::string::npos
		&& win32_socket.find("#if defined(_WIN32)") != std::string::npos
		&& win32_socket.find("#include <winsock2.h>") != std::string::npos,
		"Apple networking selects the existing POSIX backend while Winsock remains Win32-guarded");
}

}


int main(void)
{
	Test_Display_Name_And_Stable_Identity();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
