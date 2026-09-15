/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "progress_dialog.h"

#include "progress.h"
#include "language/language.h"
#include "ownrdraw.h"
#include "win.h"
#include "windlg.h"


namespace ProgressDialog
{

namespace {

HWND Dialog = NULL;


INT_PTR CALLBACK Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	INT_PTR result = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (result == 0 && message == WM_PAINT) {
		Progress.Display_Progress();
	}
	return(result);
}

} // namespace


void Begin(void)
{
	if (Dialog == NULL) {
		Dialog = OwnerDraw::Begin_Dialog(IDD_PROGRESS_WAIT, Dialog_Proc);
		if (Dialog != NULL) {
			SetWindowLongPtr(Dialog, DWLP_USER, (LONG_PTR)&Progress);
			OwnerDraw::Display_Dialog(Dialog);
			SendMessage(Dialog, WM_PAINT, 0, 0);
		}
	}
}


void End(void)
{
	if (Dialog != NULL) {
		OwnerDraw::End_Dialog(Dialog);
		Dialog = NULL;
	}
}


bool Is_Active(void)
{
	return(Dialog != NULL);
}


void Paint(void)
{
	if (Dialog != NULL) {
		SendMessage(Dialog, WM_PAINT, 0, 0);
	}
}


bool Bar_Center(Point2D & center)
{
	if (Dialog == NULL) {
		return(false);
	}

	HWND frame = GetDlgItem(Dialog, IDC_PROGRESS_BAR_FRAME);
	if (frame == NULL) {
		return(false);
	}

	RECT rect;
	if (!Get_Display_Rect(frame, &rect)) {
		return(false);
	}

	center = Point2D(rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2);
	return(true);
}

} // namespace ProgressDialog
