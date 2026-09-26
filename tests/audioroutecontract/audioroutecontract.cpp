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


std::string Read_Miniaudio_Header(void)
{
	std::ifstream source(std::string(OPENTS_SOURCE_DIR) + "/thirdparty/miniaudio/miniaudio.h");
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


void Test_Silent_Switch_Session_Policy(void)
{
	std::string const source = Read_Audio_Device_Source();
	Check(!source.empty(), "the production miniaudio device source is readable");
	std::string const miniaudio = Read_Miniaudio_Header();
	Check(!miniaudio.empty(), "the vendored miniaudio header is readable");

	std::string const config_init = "ma_context_config contextconfig = ma_context_config_init();";
	std::string const category_assignment = "contextconfig.coreaudio.sessionCategory = ";
	std::string const ios_assignment =
		"contextconfig.coreaudio.sessionCategory = ma_ios_session_category_solo_ambient;";
	std::string const context_init = "ma_context_init(";

	size_t const config_position = source.find(config_init);
	size_t const assignment_position = source.find(ios_assignment);
	size_t const context_position = source.find(context_init);
	Check(config_position != std::string::npos, "explicit context configuration remains present");
	Check(Count(source, category_assignment) == 1, "only one iOS session category is selected");
	Check(Count(source, ios_assignment) == 1, "one iOS solo-ambient category assignment is present");
	Check(context_position != std::string::npos, "explicit context initialization remains present");
	Check(config_position < assignment_position && assignment_position < context_position,
		"iOS silent-switch category is set after config init and before context init");

	size_t const ios_guard = source.rfind("#ifdef OPENTS_IOS", assignment_position);
	size_t const ios_guard_end = source.find("#endif", assignment_position);
	Check(ios_guard != std::string::npos && ios_guard < assignment_position
		&& assignment_position < ios_guard_end && ios_guard_end < context_position,
		"the session-category assignment is scoped only to OPENTS_IOS");

	size_t const category_start = miniaudio.find("/* iOS/tvOS/watchOS session categories. */");
	size_t const category_end = miniaudio.find("} ma_ios_session_category;", category_start);
	std::string const categories = category_start == std::string::npos || category_end == std::string::npos
		? std::string()
		: miniaudio.substr(category_start, category_end - category_start);
	size_t const solo_ambient = categories.find("ma_ios_session_category_solo_ambient");
	size_t const solo_ambient_line_start = categories.rfind('\n', solo_ambient);
	size_t const solo_ambient_line_end = categories.find('\n', solo_ambient);
	std::string const solo_ambient_line = solo_ambient == std::string::npos
		? std::string()
		: categories.substr(solo_ambient_line_start == std::string::npos ? 0 : solo_ambient_line_start,
			solo_ambient_line_end == std::string::npos
				? std::string::npos
				: solo_ambient_line_end - (solo_ambient_line_start == std::string::npos ? 0 : solo_ambient_line_start));
	Check(solo_ambient_line.find("AVAudioSessionCategorySoloAmbient") != std::string::npos,
		"vendored miniaudio exposes solo_ambient as AVAudioSessionCategorySoloAmbient");
	Check(miniaudio.find("case ma_ios_session_category_solo_ambient:    return AVAudioSessionCategorySoloAmbient;")
		!= std::string::npos,
		"vendored miniaudio maps solo_ambient to Apple's SoloAmbient session");

	size_t const policy_end = context_position == std::string::npos ? source.size() : context_position;
	std::string const context_setup = source.substr(
		config_position == std::string::npos ? 0 : config_position,
		policy_end - (config_position == std::string::npos ? 0 : config_position));
	Check(context_setup.find("sessionCategoryOptions") == std::string::npos,
		"CoreAudio session-category options remain at miniaudio's zero/default value");

	Check(Count(source, "ma_device_config_init(ma_device_type_playback)") == 1,
		"the device remains playback-only");
	Check(source.find("overrideOutputAudioPort") == std::string::npos,
		"output routing remains under iOS system control for speaker and headphones");
	Check(source.find("ma_device_type_duplex") == std::string::npos,
		"no duplex device type is introduced");
	Check(source.find("ma_device_type_capture") == std::string::npos,
		"no capture device type is introduced");

	for (char const * forbidden : {
		"AVAudioSessionCategoryPlayAndRecord",
		"AVAudioSessionCategoryRecord",
		"ma_ios_session_category_default",
		"ma_ios_session_category_record",
		"ma_ios_session_category_play_and_record",
		"ma_ios_session_category_multi_route",
		"AVAudioSessionCategoryOptionDefaultToSpeaker",
		"ma_ios_session_category_option_default_to_speaker",
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
	Test_Silent_Switch_Session_Policy();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
