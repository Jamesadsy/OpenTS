/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "tooltip.h"

#include <cstdarg>
#include <cstdint>
#include <iostream>
#include <string>


void __cdecl DebugString(char const *, ...)
{
}


char const * Fetch_String(int)
{
	return("");
}


namespace
{
	int Failures = 0;


	void Check(std::string const & name, bool condition)
	{
		if (!condition) {
			std::cerr << name << " failed\n";
			Failures++;
		}
	}


	class TestToolTip final : public ToolTipManager
	{
	public:
		int ResetCount = 0;
		int DrawCount = 0;
		ToolTipText LastDraw{};

		virtual bool Update(ToolTipText * text) override
		{
			text->TextWidth = 10;
			text->TextHeight = 5;
			return(true);
		}

		virtual void Reset(ToolTipText const * text) override
		{
			ResetCount++;
			ToolTipManager::Reset(text);
		}

		virtual void Draw(ToolTipText const * text) override
		{
			DrawCount++;
			LastDraw = *text;
		}

		virtual char const * ToolTip_Text(int id) override
		{
			return(id == 1 ? "first" : "second");
		}
	};


	void Add_Regions(TestToolTip & tooltips)
	{
		ToolTip first;
		first.ID = 1;
		first.Region.Set(0, 0, 10, 10);
		Check("register first region", tooltips.Add(&first));

		ToolTip second;
		second.ID = 2;
		second.Region.Set(20, 0, 10, 10);
		Check("register second region", tooltips.Add(&second));
		Check("reject duplicate region", !tooltips.Add(&second));
	}


	void Test_Hover_And_Placement(TestToolTip & tooltips)
	{
		ToolTip found;
		Check("find registered region", tooltips.Find(1, &found) && found.Region == Rect(0, 0, 10, 10));
		Check("hit lookup", tooltips.Find_From_Pos(Point2D(4, 5))->ID == 1);
		Check("miss lookup", tooltips.Find_From_Pos(Point2D(40, 5)) == NULL);

		tooltips.Activate(true);
		tooltips.Set_Timer_Delay(500);
		tooltips.Pointer_Moved(Point2D(4, 5), 100);
		Check("entering region creates candidate", tooltips.Has_Candidate());
		tooltips.Tick(599);
		Check("hover does not activate early", !tooltips.Has_Current());
		tooltips.Tick(600);
		Check("hover activates at threshold", tooltips.Has_Current() && tooltips.Current_ID() == 1);
		tooltips.Draw_Current();
		Check("logical placement is retained", tooltips.DrawCount == 1
			&& tooltips.LastDraw.Pos == Point2D(4, 5));
	}


	void Test_Cancellation_And_Rearm(TestToolTip & tooltips)
	{
		tooltips.Pointer_Moved(Point2D(24, 5), 601);
		Check("movement cancels active tooltip", !tooltips.Has_Current() && tooltips.Has_Candidate());
		tooltips.Tick(1101);
		Check("re-regioning activates replacement", tooltips.Has_Current() && tooltips.Current_ID() == 2);

		tooltips.Pointer_Activity();
		Check("pointer activity cancels tooltip", !tooltips.Has_Current() && !tooltips.Has_Candidate());
		tooltips.Pointer_Moved(Point2D(4, 5), 1200);
		tooltips.Keyboard_Activity();
		Check("keyboard activity cancels candidate", !tooltips.Has_Current() && !tooltips.Has_Candidate());

		tooltips.Pointer_Moved(Point2D(4, 5), 1300);
		tooltips.Tick(1800);
		Check("candidate rearm activates", tooltips.Has_Current());
		tooltips.Focus_Changed(false);
		Check("focus loss cancels active tooltip", !tooltips.Has_Current());
		tooltips.Focus_Changed(true);
		tooltips.Tick(2800);
		Check("focus restore requires a new pointer candidate", !tooltips.Has_Current());

		tooltips.Pointer_Moved(Point2D(4, 5), 2900);
		tooltips.Tick(3400);
		Check("focus restore rearms with pointer movement", tooltips.Has_Current());
		tooltips.Presentation_Changed(false);
		Check("presentation loss cancels active tooltip", !tooltips.Has_Current());
		tooltips.Presentation_Changed(true);

		tooltips.Pointer_Moved(Point2D(24, 5), 3500);
		tooltips.Remove(2);
		Check("region removal cancels candidate", !tooltips.Has_Candidate());
		tooltips.Tick(4000);
		Check("removed region does not activate", !tooltips.Has_Current());
	}


	void Test_Lifetime_And_Dirty_Reset(TestToolTip & tooltips)
	{
		tooltips.Pointer_Moved(Point2D(4, 5), 5000);
		tooltips.Tick(5500);
		Check("active tooltip returns after cancellation", tooltips.Has_Current());
		int const resets_before_lifetime = tooltips.ResetCount;
		tooltips.Tick(15500);
		Check("lifetime dismisses tooltip", !tooltips.Has_Current());
		Check("dismissal requests reset redraw", tooltips.ResetCount == resets_before_lifetime + 1);
	}


	void Test_Semantic_Host_Events(TestToolTip & tooltips)
	{
		OpenTSHostEvent event;
		event.Type = OPENTS_HOST_EVENT_MOUSE_MOVE;
		event.X = 4;
		event.Y = 5;
		tooltips.Host_Event(event, 16000);
		tooltips.Tick(16500);
		Check("semantic host movement activates tooltip", tooltips.Has_Current());

		event.Type = OPENTS_HOST_EVENT_MOUSE_BUTTON;
		tooltips.Host_Event(event, 16501);
		Check("semantic host activity cancels tooltip", !tooltips.Has_Current());
	}
}


int main(void)
{
	TestToolTip tooltips;
	Add_Regions(tooltips);
	Test_Hover_And_Placement(tooltips);
	Test_Cancellation_And_Rearm(tooltips);
	Test_Lifetime_And_Dirty_Reset(tooltips);
	Test_Semantic_Host_Events(tooltips);

	if (Failures != 0) {
		std::cerr << Failures << " tooltip semantic checks failed\n";
		return(1);
	}

	std::cout << "All tooltip semantic checks passed\n";
	return(0);
}
