/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "desyncdlg.h"

#ifndef _WIN32

DesyncDialogClass::OutcomeType DesyncDialogClass::Run(void)
{
	return(OutcomeType::Continue);
}


void DesyncDialogClass::Service(void)
{
}


void DesyncDialogClass::Notify_Chat(char const *, char const *)
{
}


void DesyncDialogClass::Notify_Player_Left(int, char const *)
{
}


void DesyncDialogClass::Notify_Continue(void)
{
}


void DesyncDialogClass::Notify_Heartbeat(int)
{
}


void DesyncDialogClass::Notify_Master_Changed(void)
{
}

#endif
