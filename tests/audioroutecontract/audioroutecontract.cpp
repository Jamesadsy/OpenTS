/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Source contract for the bounded iOS playback-session repair. This deliberately proves the
// policy seam and its exclusions; it does not pretend to simulate an iPhone's physical route.

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


std::string Read_Audio_Device_Source(void)
{
	std::ifstream source(std::string(OPENTS_SOURCE_DIR) + "/code/audio/audiodevice_ma.cpp");
	return(std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()));
}


size_t Count(std::string const & text, std::string const & needle)
{
	size_t count = 0;
	size_t position = 0;
	while ((position = text.find(needle, position)) != std::string::npos) {
		count++;
		position += needle.size();
	}
	return(count);
}


void Test_Playback_Session_Policy(void)
{
	std::string const source = Read_Audio_Device_Source();
	Check(!source.empty(), "the production miniaudio device source is readable");

	std::string const config_init = "ma_context_config contextconfig = ma_context_config_init();";
	std::string const ios_assignment =
		"contextconfig.coreaudio.sessionCategory = ma_ios_session_category_playback;";
	std::string const context_init = "ma_context_init(";

	size_t const config_position = source.find(config_init);
	size_t const assignment_position = source.find(ios_assignment);
	size_t const context_position = source.find(context_init);
	Check(config_position != std::string::npos, "explicit context configuration remains present");
	Check(Count(source, ios_assignment) == 1, "one iOS playback-category assignment is present");
	Check(context_position != std::string::npos, "explicit context initialization remains present");
	Check(config_position < assignment_position && assignment_position < context_position,
		"iOS playback category is set after config init and before context init");

	size_t const ios_guard = source.rfind("#ifdef OPENTS_IOS", assignment_position);
	size_t const ios_guard_end = source.find("#endif", assignment_position);
	Check(ios_guard != std::string::npos && ios_guard < assignment_position
		&& assignment_position < ios_guard_end && ios_guard_end < context_position,
		"the playback-category assignment is scoped only to OPENTS_IOS");

	size_t const policy_end = context_position == std::string::npos ? source.size() : context_position;
	std::string const context_setup = source.substr(
		config_position == std::string::npos ? 0 : config_position,
		policy_end - (config_position == std::string::npos ? 0 : config_position));
	Check(context_setup.find("sessionCategoryOptions") == std::string::npos,
		"CoreAudio session-category options remain at miniaudio's zero/default value");

	Check(Count(source, "ma_device_config_init(ma_device_type_playback)") == 1,
		"the device remains playback-only");
	Check(source.find("ma_device_type_duplex") == std::string::npos,
		"no duplex device type is introduced");
	Check(source.find("ma_device_type_capture") == std::string::npos,
		"no capture device type is introduced");

	for (char const * forbidden : {
		"AVAudioSessionCategoryPlayAndRecord",
		"AVAudioSessionCategoryOptionDefaultToSpeaker",
		"AVAudioSessionCategoryOptionAllowBluetooth",
		"AVAudioSessionCategoryOptionAllowBluetoothA2DP",
		"overrideOutputAudioPort",
		"ma_device_type_duplex",
		"ma_device_type_capture"
	}) {
		Check(source.find(forbidden) == std::string::npos,
			(std::string("production source excludes ") + forbidden).c_str());
	}
}

}


int main(void)
{
	Test_Playback_Session_Policy();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
