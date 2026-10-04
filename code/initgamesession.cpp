/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "initgamesession.h"

bool InitGameSession::Begin(InitGameSessionOps const & ops) noexcept
{
	if (State != Phase::Idle || !ops.Prepare || !ops.BeginMovie || !ops.StepMovie
		|| !ops.FinishMovie || !ops.Complete) return(false);
	Ops = ops;
	State = Phase::Prepare;
	return(true);
}


InitGameStepResult InitGameSession::Step(void) noexcept
{
	using Result = InitGameStepResult;
	switch (State) {
		case Phase::Prepare:
			State = Ops.Prepare(Ops.Context) == 0 ? Phase::BeginMovie : Phase::Fault;
			break;
		case Phase::BeginMovie: {
			MovieOwned = true;
			Result const result = Ops.BeginMovie(Ops.Context, MovieIndex);
			if (result == Result::Fault) { State = Phase::Fault; break; }
			if (result == Result::Complete) {
				Ops.FinishMovie(Ops.Context);
				MovieOwned = false;
				State = ++MovieIndex == 3 ? Phase::CompleteInit : Phase::BeginMovie;
				break;
			}
			State = Phase::Movie;
			return(Result::Waiting);
		}
		case Phase::Movie: {
			Result const result = Ops.StepMovie(Ops.Context);
			if (result == Result::Waiting) return(result);
			Ops.FinishMovie(Ops.Context);
			MovieOwned = false;
			State = result == Result::Fault ? Phase::Fault
				: (++MovieIndex == 3 ? Phase::CompleteInit : Phase::BeginMovie);
			break;
		}
		case Phase::CompleteInit:
			State = Ops.Complete(Ops.Context) == 0 ? Phase::Complete : Phase::Fault;
			break;
		case Phase::Complete: return(Result::Complete);
		case Phase::Idle:
		case Phase::Fault: return(Result::Fault);
	}
	return(State == Phase::Fault ? Result::Fault
		: State == Phase::Complete ? Result::Complete : Result::Waiting);
}


void InitGameSession::Finish(void) noexcept
{
	if (MovieOwned) {
		Ops.FinishMovie(Ops.Context);
		MovieOwned = false;
	}
	if (State != Phase::Complete) State = Phase::Fault;
}
