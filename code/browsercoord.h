/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

#include <cstdint>

namespace OpenTS_Browser
{

enum class CooperativeResult : int
{
	Active = 0,
	Waiting = 1,
	Complete = 2,
	Fault = 3
};

enum class BrowserProcessPhase : int
{
	Idle = 0,
	Bootstrap = 1,
	WaitFocus = 2,
	MainGame = 3,
	ShutdownRequest = 4,
	ShutdownDrain = 5,
	Complete = 6,
	Fault = 7
};

enum class StartupAction : std::uint8_t
{
	Waiting,
	Complete,
	Boundary,
	NativeChild,
	Fault
};

struct BrowserProcessOperations
{
	void * Context = nullptr;
	StartupAction (*Bootstrap)(void *) = nullptr;
	StartupAction (*WaitFocus)(void *) = nullptr;
	StartupAction (*BeginMainGame)(void *) = nullptr;
	StartupAction (*StepMainGame)(void *) = nullptr;
	StartupAction (*ShutdownRequest)(void *) = nullptr;
	StartupAction (*ShutdownDrain)(void *) = nullptr;
	void (*Finish)(void *) = nullptr;
};

struct BrowserProcessSession
{
	BrowserProcessPhase Phase = BrowserProcessPhase::Idle;
	BrowserProcessOperations Operations;
	std::uint32_t StepCount = 0;
	std::uint32_t WaitFocusYields = 0;
	std::uint32_t MainGameSteps = 0;
	std::uint32_t ShutdownDrainPasses = 0;
	int FaultCode = 0;
	bool BeginOwned = false;
	bool StepActive = false;
	bool MainGameBeginOwned = false;
	bool FinishOwned = false;
};

bool Begin(BrowserProcessSession & session, BrowserProcessOperations const & operations) noexcept;
CooperativeResult Step(BrowserProcessSession & session) noexcept;
BrowserProcessPhase Status(BrowserProcessSession const & session) noexcept;
bool Finish(BrowserProcessSession & session) noexcept;

} // namespace OpenTS_Browser

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define OPENTS_BROWSER_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define OPENTS_BROWSER_EXPORT
#endif

extern "C"
{
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Step(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Status(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Finish(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin_Count(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin_Result(void);
OPENTS_BROWSER_EXPORT void OpenTS_Browser_Main_Entry_Returned(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Main_Returned(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Step_Count(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_WaitFocus_Yields(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_MainGame_Began(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_MainGame_Steps(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Shutdown_Drain_Passes(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_OwnerData_Boundary(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Fault_Code(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Finish_Count(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Renderer_Initialized(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Bootstrap_Stage(void);
OPENTS_BROWSER_EXPORT char const * OpenTS_Browser_Renderer_Name(void);
}
