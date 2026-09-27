/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "browsercoord.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#ifndef OPENTS_SOURCE_ROOT
#error OPENTS_SOURCE_ROOT must point at the OpenTS source tree.
#endif

namespace
{

int Failures = 0;

void Check(bool condition, char const * description)
{
	std::printf("%-76s %s\n", description, condition ? "ok" : "FAILED");
	if (!condition) {
		++Failures;
	}
}

std::string ReadSource(char const * relative_path)
{
	std::string const path = std::string(OPENTS_SOURCE_ROOT) + "/" + relative_path;
	std::ifstream file(path, std::ios::binary);
	return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::size_t Count(std::string const & text, std::string const & needle)
{
	std::size_t count = 0;
	std::size_t offset = 0;
	while ((offset = text.find(needle, offset)) != std::string::npos) {
		++count;
		offset += needle.size();
	}
	return count;
}

void Test_Entry_Routes(void)
{
	std::string const entry = ReadSource("platform/win32compat/src/entry.cpp");
	std::size_t const main = entry.find("int main(int argc, char ** argv)");
	std::size_t const emscripten = entry.find("#ifdef __EMSCRIPTEN__", main);
	std::size_t const native = entry.find("#else", emscripten);
	if (main == std::string::npos || emscripten == std::string::npos
		|| native == std::string::npos) {
		Check(false, "entry.cpp contains separate Emscripten and native main routes");
		return;
	}

	std::string const browser_route = entry.substr(emscripten, native - emscripten);
	std::string const native_route = entry.substr(native);
	Check(true, "entry.cpp contains separate Emscripten and native main routes");
	Check(browser_route.find("WinMain(") == std::string::npos
		&& browser_route.find("SDL_Init(") == std::string::npos
		&& browser_route.find("return(0)") != std::string::npos,
		"Emscripten entry returns to the browser without WinMain or engine host startup");
	Check(native_route.find("Win32_Record_Arguments(argc, argv)") != std::string::npos
		&& native_route.find("SDL_Init(") != std::string::npos
		&& native_route.find("WinMain((HINSTANCE)") != std::string::npos,
		"native entry retains its synchronous main-to-WinMain route");
}

void Test_Native_Session_Contract(void)
{
	using namespace OpenTS_Browser;
	BrowserProcessSession session;

	Check(Step(session) == CooperativeResult::Fault
		&& Status(session) == BrowserProcessPhase::Idle,
		"Step before Begin is rejected without corrupting the idle session");
	Check(Begin(session) && Status(session) == BrowserProcessPhase::ControlProbe,
		"Begin acquires native ownership and enters ControlProbe once");
	Check(!Begin(session) && Status(session) == BrowserProcessPhase::ControlProbe,
		"double Begin is rejected while the original session stays deterministic");
	Check(Step(session) == CooperativeResult::Active && session.StepCount == 1
		&& Status(session) == BrowserProcessPhase::ControlProbe,
		"one bounded Step returns Active and advances exactly one native step");
	Check(!Finish(session) && !session.FinishOwned,
		"Finish cannot take ownership while the native probe is still active");

	bool intermediate_steps_are_active = true;
	for (std::uint32_t index = 1; index < ControlProbeStepLimit - 1; ++index) {
		if (Step(session) != CooperativeResult::Active) {
			intermediate_steps_are_active = false;
			break;
		}
	}
	Check(intermediate_steps_are_active && session.StepCount == ControlProbeStepLimit - 1
		&& Status(session) == BrowserProcessPhase::ControlProbe,
		"the first 299 bounded steps stay in the native control probe");
	Check(Step(session) == CooperativeResult::Complete
		&& session.StepCount == ControlProbeStepLimit
		&& Status(session) == BrowserProcessPhase::Complete,
		"the 300th bounded step deterministically completes without game semantics");
	Check(Step(session) == CooperativeResult::Complete
		&& session.StepCount == ControlProbeStepLimit,
		"Step after completion returns Complete without advancing state");
	Check(Finish(session) && session.FinishOwned
		&& Status(session) == BrowserProcessPhase::Complete,
		"Finish claims the completed native session once");
	Check(Finish(session) && session.FinishOwned,
		"Finish is idempotent for the single completed session owner");
	Check(!Begin(session), "a finished session cannot be begun a second time");

	BrowserProcessSession reentrant;
	Check(Begin(reentrant), "re-entry contract fixture begins natively");
	reentrant.StepActive = true;
	Check(Step(reentrant) == CooperativeResult::Fault
		&& Status(reentrant) == BrowserProcessPhase::Fault
		&& reentrant.FaultCode != 0,
		"Step re-entry is rejected and recorded as a native fault");
}

void Test_Exported_ABI_Contract(void)
{
	Check(OpenTS_Browser_Step() == static_cast<int>(OpenTS_Browser::CooperativeResult::Fault)
		&& OpenTS_Browser_Status() == static_cast<int>(OpenTS_Browser::BrowserProcessPhase::Idle),
		"exported Step before Begin is rejected while exported Status remains queryable");
	Check(OpenTS_Browser_Begin() == 1 && OpenTS_Browser_Begin() == 0,
		"exported Begin succeeds once and safely rejects a duplicate owner");
	Check(OpenTS_Browser_Step() == static_cast<int>(OpenTS_Browser::CooperativeResult::Active),
		"exported Step returns a bounded native result through the C ABI");
	for (std::uint32_t index = 1; index < OpenTS_Browser::ControlProbeStepLimit; ++index) {
		OpenTS_Browser_Step();
	}
	Check(OpenTS_Browser_Status() == static_cast<int>(OpenTS_Browser::BrowserProcessPhase::Complete),
		"exported Status reports deterministic native completion");
	Check(OpenTS_Browser_Finish() == 1 && OpenTS_Browser_Finish() == 1,
		"exported Finish is idempotent for the completed native ABI owner");
}

void Test_Browser_Contract_Surface(void)
{
	std::string const cmake = ReadSource("code/CMakeLists.txt");
	Check(cmake.find("-sNO_EXIT_RUNTIME=1") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Begin") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Step") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Status") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Finish") != std::string::npos,
		"Emscripten keeps the runtime alive and exports the four coordinator ABI functions");
	Check(cmake.find("ASYNCIFY") == std::string::npos
		&& cmake.find("JSPI") == std::string::npos,
		"Emscripten coordinator configuration contains no Asyncify or JSPI setting");

	std::string const harness = ReadSource("tests/browsercoord/harness.js");
	Check(Count(harness, "Module._OpenTS_Browser_Step()") == 1,
		"the browser callback harness contains exactly one native Step call site");
	Check(harness.find("BrowserProcessPhase") == std::string::npos
		&& harness.find("ControlProbe") == std::string::npos
		&& harness.find("MainGame") == std::string::npos
		&& harness.find("Select_Game") == std::string::npos,
		"the JavaScript harness carries diagnostics only and no semantic phase model");
}

} // namespace

int main(void)
{
	Test_Entry_Routes();
	Test_Native_Session_Contract();
	Test_Exported_ABI_Contract();
	Test_Browser_Contract_Surface();
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return Failures == 0 ? 0 : 1;
}
