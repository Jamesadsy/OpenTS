/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
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
	if (!condition) ++Failures;
}

std::string ReadSource(char const * relative_path)
{
	std::string const path = std::string(OPENTS_SOURCE_ROOT) + "/" + relative_path;
	std::ifstream file(path, std::ios::binary);
	return(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
}

struct Fixture
{
	int BootstrapCalls = 0;
	int FocusCalls = 0;
	int BeginMainGameCalls = 0;
	int MainGameStepCalls = 0;
	int ShutdownRequestCalls = 0;
	int ShutdownDrainCalls = 0;
	int FinishCalls = 0;
	bool FaultAtBootstrap = false;
};

OpenTS_Browser::StartupAction Bootstrap(void * context)
{
	Fixture & fixture = *static_cast<Fixture *>(context);
	++fixture.BootstrapCalls;
	return(fixture.FaultAtBootstrap
		? OpenTS_Browser::StartupAction::Fault
		: OpenTS_Browser::StartupAction::Complete);
}

OpenTS_Browser::StartupAction WaitFocus(void * context)
{
	Fixture & fixture = *static_cast<Fixture *>(context);
	return(++fixture.FocusCalls == 1
		? OpenTS_Browser::StartupAction::Waiting
		: OpenTS_Browser::StartupAction::Complete);
}

OpenTS_Browser::StartupAction BeginMainGame(void * context)
{
	++static_cast<Fixture *>(context)->BeginMainGameCalls;
	return(OpenTS_Browser::StartupAction::Boundary);
}

OpenTS_Browser::StartupAction StepMainGame(void * context)
{
	++static_cast<Fixture *>(context)->MainGameStepCalls;
	return(OpenTS_Browser::StartupAction::Complete);
}

OpenTS_Browser::StartupAction ShutdownRequest(void * context)
{
	++static_cast<Fixture *>(context)->ShutdownRequestCalls;
	return(OpenTS_Browser::StartupAction::Complete);
}

OpenTS_Browser::StartupAction ShutdownDrain(void * context)
{
	Fixture & fixture = *static_cast<Fixture *>(context);
	return(++fixture.ShutdownDrainCalls == 1
		? OpenTS_Browser::StartupAction::Waiting
		: OpenTS_Browser::StartupAction::Complete);
}

void Finish(void * context)
{
	++static_cast<Fixture *>(context)->FinishCalls;
}

OpenTS_Browser::BrowserProcessOperations Operations(Fixture & fixture)
{
	OpenTS_Browser::BrowserProcessOperations operations;
	operations.Context = &fixture;
	operations.Bootstrap = &Bootstrap;
	operations.WaitFocus = &WaitFocus;
	operations.BeginMainGame = &BeginMainGame;
	operations.StepMainGame = &StepMainGame;
	operations.ShutdownRequest = &ShutdownRequest;
	operations.ShutdownDrain = &ShutdownDrain;
	operations.Finish = &Finish;
	return(operations);
}

void Test_Process_Phases(void)
{
	using namespace OpenTS_Browser;
	Fixture fixture;
	BrowserProcessSession session;
	BrowserProcessOperations const operations = Operations(fixture);

	Check(Step(session) == CooperativeResult::Fault
		&& Status(session) == BrowserProcessPhase::Idle,
		"Step before Begin is rejected without corrupting the idle session");
	Check(Begin(session, operations) && Status(session) == BrowserProcessPhase::Bootstrap,
		"Begin owns a real startup session and enters Bootstrap");
	Check(!Begin(session, operations), "duplicate Begin is rejected");
	Check(Step(session) == CooperativeResult::Waiting
		&& fixture.BootstrapCalls == 1 && Status(session) == BrowserProcessPhase::WaitFocus,
		"one Bootstrap action advances to WaitFocus");
	Check(Step(session) == CooperativeResult::Waiting
		&& fixture.FocusCalls == 1 && session.WaitFocusYields == 1
		&& Status(session) == BrowserProcessPhase::WaitFocus,
		"WaitFocus returns Waiting after one message-handler pass");
	Check(Step(session) == CooperativeResult::Waiting
		&& fixture.FocusCalls == 2 && Status(session) == BrowserProcessPhase::MainGame,
		"WaitFocus advances only after its next bounded pass observes focus");
	Check(Step(session) == CooperativeResult::Waiting
		&& fixture.BeginMainGameCalls == 1 && session.MainGameBeginOwned
		&& Status(session) == BrowserProcessPhase::ShutdownRequest,
		"MainGame begins once and records the owner-data boundary");
	Check(fixture.MainGameStepCalls == 0,
		"the boundary prevents a child gameplay or menu Step");
	Check(Step(session) == CooperativeResult::Waiting
		&& fixture.ShutdownRequestCalls == 1 && Status(session) == BrowserProcessPhase::ShutdownDrain,
		"ShutdownRequest runs once before drain");
	Check(Step(session) == CooperativeResult::Waiting
		&& fixture.ShutdownDrainCalls == 1 && session.ShutdownDrainPasses == 1
		&& Status(session) == BrowserProcessPhase::ShutdownDrain,
		"ShutdownDrain yields after one pending message-handler pass");
	Check(Step(session) == CooperativeResult::Complete
		&& fixture.ShutdownDrainCalls == 2 && Status(session) == BrowserProcessPhase::Complete,
		"ShutdownDrain completes after its next bounded message-handler pass");
	Check(session.StepCount == 7 && Finish(session) && Finish(session)
		&& fixture.FinishCalls == 1 && session.FinishOwned,
		"Complete owns one idempotent Finish after seven native Steps");
	Check(!Begin(session, operations), "a finished process session cannot be restarted");
}

void Test_Fault_And_Reentry(void)
{
	using namespace OpenTS_Browser;
	Fixture fixture;
	fixture.FaultAtBootstrap = true;
	BrowserProcessSession faulted;
	Check(Begin(faulted, Operations(fixture))
		&& Step(faulted) == CooperativeResult::Fault
		&& Status(faulted) == BrowserProcessPhase::Fault,
		"Bootstrap faults enter the terminal Fault process phase");
	Check(Finish(faulted) && fixture.FinishCalls == 1,
		"Fault also releases the browser process through Finish");

	Fixture reentry_fixture;
	BrowserProcessSession reentrant;
	Check(Begin(reentrant, Operations(reentry_fixture)), "re-entry fixture begins");
	reentrant.StepActive = true;
	Check(Step(reentrant) == CooperativeResult::Fault
		&& Status(reentrant) == BrowserProcessPhase::Fault
		&& reentrant.FaultCode != 0,
		"a nested Step is rejected and recorded as a fault");
}

void Test_Entry_And_Boundary_Contracts(void)
{
	std::string const entry = ReadSource("platform/win32compat/src/entry.cpp");
	std::size_t const main = entry.find("int main(int argc, char ** argv)");
	std::size_t const emscripten = entry.find("#ifdef __EMSCRIPTEN__", main);
	std::size_t const native = entry.find("#else", emscripten);
	std::string const browser_route = entry.substr(emscripten, native - emscripten);
	std::string const native_route = entry.substr(native);
	Check(browser_route.find("Win32_Record_Arguments(argc, argv)") != std::string::npos
		&& browser_route.find("OpenTS_Browser_Begin()") != std::string::npos
		&& browser_route.find("OpenTS_Browser_Main_Entry_Returned()") != std::string::npos
		&& browser_route.find("WinMain(") == std::string::npos
		&& browser_route.find("SDL_Quit") == std::string::npos,
		"Emscripten main copies arguments, begins once, and returns without WinMain or teardown");
	Check(native_route.find("SDL_Init(") != std::string::npos
		&& native_route.find("WinMain((HINSTANCE)") != std::string::npos
		&& native_route.find("Win32_Gamepad_Shutdown()") != std::string::npos
		&& native_route.find("SDL_Quit()") != std::string::npos,
		"native main retains SDL, WinMain, gamepad shutdown, and SDL_Quit ordering");

	std::string const init = ReadSource("code/init.cpp");
	std::size_t const reach = init.find("OpenTS_Browser_Startup_Reach_OwnerData_Boundary();");
	std::size_t const boundary_return = init.find("return(false);", reach);
	std::size_t const mixfiles = init.find("Init_Bootstrap_Mixfiles())", reach);
	Check(reach != std::string::npos && boundary_return != std::string::npos
		&& mixfiles != std::string::npos && reach < boundary_return && boundary_return < mixfiles,
		"browser startup records the no-owner-data frontier before the first MIX archive access");
	std::string const startup = ReadSource("code/startup.cpp");
	Check(startup.find("OpenTS_Browser_Startup_WaitFocus") != std::string::npos
		&& startup.find("Windows_Message_Handler();\n\treturn(GameInFocus ? 1 : 0);") != std::string::npos
		&& startup.find("OpenTS_Browser_Startup_Shutdown_Drain") != std::string::npos,
		"browser focus and shutdown each use one native message-handler pass per action");

	std::string const host_clock = ReadSource("code/hostclock.h");
	std::size_t const browser_step = host_clock.find("if (OpenTS_Browser_Step_Active())");
	std::size_t const sleep_counter = host_clock.find("OPENTS_TELEMETRY_HOST_SLEEP", browser_step);
	std::size_t const browser_return = host_clock.find("return;", sleep_counter);
	std::size_t const zero_wait = host_clock.find("if (milliseconds == 0)", browser_return);
	std::size_t const real_sleep = host_clock.find("std::this_thread::sleep_for", zero_wait);
	Check(browser_step != std::string::npos && sleep_counter != std::string::npos
		&& browser_return != std::string::npos && zero_wait != std::string::npos
		&& real_sleep != std::string::npos && browser_step < sleep_counter
		&& sleep_counter < browser_return && browser_return < zero_wait
		&& zero_wait < real_sleep,
		"browser-facing Step records Host_Sleep requests and returns before any blocking wait");
}

void Test_Harness_And_Build_Contracts(void)
{
	std::string const cmake = ReadSource("code/CMakeLists.txt");
	Check(cmake.find("-sNO_EXIT_RUNTIME=1") != std::string::npos
		&& cmake.find("OUTPUT_NAME OpenTS") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Begin") != std::string::npos
		&& cmake.find("_OpenTS_Browser_Renderer_Name") != std::string::npos
		&& cmake.find("OpenTS-browser-proof.html") != std::string::npos,
		"Emscripten emits OpenTS.js/Wasm and stages the browser process harness");
	Check(cmake.find("ASYNCIFY") == std::string::npos
		&& cmake.find("JSPI") == std::string::npos,
		"Emscripten process build keeps Asyncify and JSPI disabled");

	std::string const harness = ReadSource("tests/browserstartup/harness.js");
	Check(harness.find("nativeValue(\"OpenTS_Browser_Step\")") != std::string::npos
		&& harness.find("requestAnimationFrame(onFrame)") != std::string::npos
		&& harness.find("heartbeatCount += 1;") != std::string::npos
		&& harness.find("live.heartbeatCount = heartbeatCount;") != std::string::npos
		&& harness.find("OpenTS_Browser_OwnerData_Boundary") != std::string::npos,
		"browser harness records process callbacks, independent heartbeat, and owner-data boundary");
	Check(harness.find("Select_Game") == std::string::npos
		&& harness.find("Main_Loop") == std::string::npos
		&& harness.find("Init_Game(") == std::string::npos,
		"JavaScript observes process metrics without carrying game semantics");
}

} // namespace

int main(void)
{
	Test_Process_Phases();
	Test_Fault_And_Reentry();
	Test_Entry_And_Boundary_Contracts();
	Test_Harness_And_Build_Contracts();
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
