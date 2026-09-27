/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "browsercoord.h"

namespace
{

OpenTS_Browser::BrowserProcessSession Session;

constexpr int FaultInvalidPhase = 1;
constexpr int FaultReentrantStep = 2;

struct StepActivityGuard
{
	explicit StepActivityGuard(OpenTS_Browser::BrowserProcessSession & session) noexcept
		: Session(session)
	{
		Session.StepActive = true;
	}

	~StepActivityGuard() noexcept
	{
		Session.StepActive = false;
	}

	OpenTS_Browser::BrowserProcessSession & Session;
};

void SetFault(OpenTS_Browser::BrowserProcessSession & session, int code) noexcept
{
	session.Phase = OpenTS_Browser::BrowserProcessPhase::Fault;
	session.FaultCode = code;
}

} // namespace

namespace OpenTS_Browser
{

bool Begin(BrowserProcessSession & session) noexcept
{
	if (session.BeginOwned || session.FinishOwned || session.Phase != BrowserProcessPhase::Idle) {
		return false;
	}

	session.BeginOwned = true;
	session.StepCount = 0;
	session.FaultCode = 0;
	session.Phase = BrowserProcessPhase::ControlProbe;
	return true;
}


CooperativeResult Step(BrowserProcessSession & session) noexcept
{
	if (!session.BeginOwned) {
		return CooperativeResult::Fault;
	}

	if (session.StepActive) {
		SetFault(session, FaultReentrantStep);
		return CooperativeResult::Fault;
	}

	if (session.Phase == BrowserProcessPhase::Complete) {
		return CooperativeResult::Complete;
	}
	if (session.Phase == BrowserProcessPhase::Fault) {
		return CooperativeResult::Fault;
	}
	if (session.Phase != BrowserProcessPhase::ControlProbe) {
		SetFault(session, FaultInvalidPhase);
		return CooperativeResult::Fault;
	}

	StepActivityGuard active(session);
	++session.StepCount;
	if (session.StepCount >= ControlProbeStepLimit) {
		session.Phase = BrowserProcessPhase::Complete;
		return CooperativeResult::Complete;
	}

	return CooperativeResult::Active;
}


BrowserProcessPhase Status(BrowserProcessSession const & session) noexcept
{
	return session.Phase;
}


bool Finish(BrowserProcessSession & session) noexcept
{
	if (session.FinishOwned) {
		return true;
	}
	if (!session.BeginOwned || session.Phase != BrowserProcessPhase::Complete) {
		return false;
	}

	session.FinishOwned = true;
	return true;
}

} // namespace OpenTS_Browser

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin(void)
{
	return OpenTS_Browser::Begin(Session) ? 1 : 0;
}


extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Step(void)
{
	return static_cast<int>(OpenTS_Browser::Step(Session));
}


extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Status(void)
{
	return static_cast<int>(OpenTS_Browser::Status(Session));
}


extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Finish(void)
{
	return OpenTS_Browser::Finish(Session) ? 1 : 0;
}
