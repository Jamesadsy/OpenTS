/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "cursorpresentationpolicy.hh"
#include "cursorstate.hh"
#include "legacyinputpolicy.hh"
#include "mouseoverridepolicy.hh"
#include "point.h"
#include "pointerscrollpolicy.hh"
#include "controlleredgescrollpolicy.hh"
#include "rect.h"
#include "sidebarlayoutpolicy.hh"
#include "vidscalepolicy.hh"

namespace
{

constexpr int SCROLL_RATE_SETTING_COUNT = 8;
int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-74s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}


void Test_Action_Cursor_Resolution(void)
{
	Check(Action_Cursor_Shape(ACTION_NONE, false, false, false) == MOUSE_NORMAL,
		"neutral menus and empty tactical ground use the ordinary arrow");
	Check(Action_Cursor_Shape(ACTION_SELECT, false, false, false) == MOUSE_CAN_SELECT
		&& Action_Cursor_Shape(ACTION_TOGGLE_SELECT, false, false, false) == MOUSE_CAN_SELECT,
		"native friendly selection actions use the selection cursor");
	Check(Action_Cursor_Shape(ACTION_MOVE, false, false, false) == MOUSE_CAN_MOVE,
		"native valid movement uses the move cursor");
	Check(Action_Cursor_Shape(ACTION_NOMOVE, false, false, false) == MOUSE_NO_MOVE,
		"native invalid movement uses the no-move cursor");
	Check(Action_Cursor_Shape(ACTION_ATTACK, false, false, true) == MOUSE_STAY_ATTACK
		&& Action_Cursor_Shape(ACTION_ATTACK, false, false, false) == MOUSE_CAN_ATTACK,
		"native enemy attack actions keep their range-specific attack cursor");
	Check(Action_Cursor_Shape(ACTION_MOVE, false, false, false) == MOUSE_CAN_MOVE,
		"a selected MCV over ordinary move ground is not forced into deploy mode");
	Check(Action_Cursor_Shape(ACTION_SELF, false, false, false) == MOUSE_DEPLOY
		&& Action_Cursor_Shape(ACTION_NO_DEPLOY, false, false, false) == MOUSE_NO_DEPLOY,
		"deploy cursors appear only for native self or no-deploy actions");
	Check(Action_Cursor_Shape(ACTION_NOMOVE, true, true, false) == MOUSE_CAN_MOVE,
		"native move-to-shroud units retain their valid hidden-ground cursor");
}


void Test_Repair_Sell_Seeds_And_Hover(void)
{
	Check(RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Repair) == MOUSE_REPAIR
		&& Action_Cursor_Shape(ACTION_REPAIR, false, false, false) == MOUSE_REPAIR
		&& Action_Cursor_Shape(ACTION_GREPAIR, false, false, false) == MOUSE_GREPAIR,
		"Repair seeds immediately and native repair hover selects the matching valid cursor");
	Check(Action_Cursor_Shape(ACTION_NO_REPAIR, false, false, false) == MOUSE_NO_REPAIR
		&& Action_Cursor_Shape(ACTION_NO_GREPAIR, false, false, false) == MOUSE_NO_REPAIR,
		"invalid repair hover selects the native no-repair cursor");
	Check(RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Sell) == MOUSE_SELL_BACK
		&& Action_Cursor_Shape(ACTION_SELL, false, false, false) == MOUSE_SELL_BACK
		&& Action_Cursor_Shape(ACTION_SELL_UNIT, false, false, false) == MOUSE_SELL_UNIT,
		"Sell seeds immediately and native hover distinguishes structures from units");
	Check(Action_Cursor_Shape(ACTION_NO_SELL, false, false, false) == MOUSE_NO_SELL_BACK,
		"invalid sell hover selects the native no-sell cursor");
	Check(RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Normal) == MOUSE_NORMAL,
		"cancelling Repair or Sell seeds the ordinary native pointer state");
}


void Test_Square_Mode_Cycle(void)
{
	RepairSellModeCursor mode = RepairSellModeCursor::Normal;
	mode = Next_RepairSell_Mode_Cursor(mode);
	Check(mode == RepairSellModeCursor::Repair
		&& RepairSell_Mode_Cursor_Seed(mode) == MOUSE_REPAIR,
		"Square arms Repair and seeds the repair cursor");
	mode = Next_RepairSell_Mode_Cursor(mode);
	Check(mode == RepairSellModeCursor::Sell
		&& RepairSell_Mode_Cursor_Seed(mode) == MOUSE_SELL_BACK,
		"Square advances Repair to Sell and seeds the sell cursor");
	mode = Next_RepairSell_Mode_Cursor(mode);
	Check(mode == RepairSellModeCursor::Normal
		&& RepairSell_Mode_Cursor_Seed(mode) == MOUSE_NORMAL,
		"Square advances Sell to Normal and seeds the ordinary cursor");
}


void Test_Mode_Icon_Ownership(void)
{
	std::ifstream file(OPENTS_UI_MODE_ICON_SOURCE);
	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::size_t const service = source.find("void UI_Mode_Icon_Service(void)");
	std::size_t const end = source.find("void UI_Mode_Icon_Shutdown(void)", service);
	std::string const service_body = service == std::string::npos || end == std::string::npos
		? std::string()
		: source.substr(service, end - service);
	Check(!service_body.empty()
		&& service_body.find("Set_Default_Mouse") == std::string::npos
		&& service_body.find("Apply_Armed_Pointer_Fallback") == std::string::npos,
		"mode-icon service cannot overwrite a target-specific native hover cursor");
	Check(source.find("Can_Deploy_Now") == std::string::npos,
		"selected deployable units are not treated as a persistent deploy mode");
}


void Test_Controller_ScrollRate(void)
{
	static double const expected[] = { 1.250, 1.167, 1.083, 1.000, 0.917, 0.833, 0.750, 0.667 };
	double scales[SCROLL_RATE_SETTING_COUNT] = {};
	for (int rate = 0; rate < SCROLL_RATE_SETTING_COUNT; rate++) {
		scales[rate] = Controller_ScrollRate_Scale(rate, SCROLL_RATE_SETTING_COUNT);
	}
	bool table_matches = true;
	bool monotonic = true;
	for (int rate = 0; rate < SCROLL_RATE_SETTING_COUNT; rate++) {
		table_matches = table_matches && std::abs(scales[rate] - expected[rate]) < 0.0006;
		if (rate != 0) monotonic = monotonic && scales[rate] < scales[rate - 1];
	}
	Check(table_matches, "all eight controller ScrollRate settings use the accepted 600-to-320 px/s curve");
	Check(monotonic && std::abs(scales[0] * 480.0 - 600.0) < 0.001
		&& std::abs(scales[3] * 480.0 - 480.0) < 0.001
		&& std::abs(scales[7] * 480.0 - 320.0) < 0.001,
		"raw ScrollRate 3 is 480 px/s and the bounded endpoints are 600 and 320 px/s");
	Check(Controller_ScrollRate_Scale(-1, SCROLL_RATE_SETTING_COUNT) == scales[0]
		&& Controller_ScrollRate_Scale(8, SCROLL_RATE_SETTING_COUNT) == scales[7],
		"controller speed remains bounded outside the slider's ScrollRate values");

	std::ifstream options_file(OPENTS_OPTIONS_SOURCE);
	std::string options_source((std::istreambuf_iterator<char>(options_file)), std::istreambuf_iterator<char>());
	Check(options_source.find("MAX_SCROLL_SETTING=8") != std::string::npos,
		"the game controls expose all eight ScrollRate positions used by controller pan");
	std::ifstream options_cpp_file(OPENTS_OPTIONS_CPP_SOURCE);
	std::string options_cpp_source((std::istreambuf_iterator<char>(options_cpp_file)), std::istreambuf_iterator<char>());
	Check(options_cpp_source.find("ScrollRate(3)") != std::string::npos
		&& std::abs(Controller_ScrollRate_Scale(3, SCROLL_RATE_SETTING_COUNT) * 480.0 - 480.0) < 0.001,
		"the native raw ScrollRate default is 3 and preserves 480 px/s camera feel");

	int touch_travel[3] = {};
	int const rates[] = {0, 3, 7};
	double controller_scales[3] = {};
	for (int rate_index = 0; rate_index < 3; rate_index++) {
		controller_scales[rate_index] = Controller_ScrollRate_Scale(rates[rate_index],
			SCROLL_RATE_SETTING_COUNT);
		double remainder = 0.0;
		for (int index = 0; index < 10; index++) {
			touch_travel[rate_index] += Scale_Touch_Scroll_Offset(1, 0.5, remainder);
		}
	}
	Check(controller_scales[0] > controller_scales[1] && controller_scales[1] > controller_scales[2]
		&& touch_travel[0] == 5 && touch_travel[1] == touch_travel[0]
		&& touch_travel[2] == touch_travel[0],
		"touch pan retains screen scaling, subpixel travel, and independence from ScrollRate");

	double controller_remainder = 0.0;
	int controller_travel = 0;
	for (int index = 0; index < 10; index++) {
		controller_travel += Scale_Controller_Scroll_Offset(1, 0.5, 4,
			SCROLL_RATE_SETTING_COUNT, controller_remainder);
	}
	Check(controller_travel == 4 && controller_remainder > 0.57 && controller_remainder < 0.60,
		"controller pan keeps useful fractional travel at a low/mid ScrollRate setting");
	Check(Pointer_Edge_Scroll_Source(false, false, false) == PointerEdgeScrollSource::NativeImmediate
		&& Pointer_Edge_Scroll_Source(true, false, false) == PointerEdgeScrollSource::ControllerDelayed
		&& Pointer_Edge_Scroll_Source(false, true, false) == PointerEdgeScrollSource::Suppressed
		&& Pointer_Edge_Scroll_Source(false, false, true) == PointerEdgeScrollSource::Suppressed,
		"hardware pointer scrolls immediately, controller pointer waits, and direct touch or camera pan suppresses edges");

	ControllerEdgeScrollDwell dwell;
	Check(ControllerEdgeScrollDwell::DWELL_MS == 1000
		&& !dwell.Should_Scroll(3, 1000) && !dwell.Should_Scroll(3, 1999)
		&& dwell.Should_Scroll(3, 2000)
		&& !dwell.Should_Scroll(4, 2001) && !dwell.Should_Scroll(4, 3000)
		&& dwell.Should_Scroll(4, 3001),
		"controller edge dwell waits 1000 ms and restarts when the native direction changes");
	dwell.Reset();
	Check(!dwell.Should_Scroll(3, 10000) && !dwell.Should_Scroll(0, 10500)
		&& !dwell.Should_Scroll(3, 11999) && !dwell.Should_Scroll(3, 12998)
		&& dwell.Should_Scroll(3, 12999),
		"leaving the edge resets controller dwell and requires a fresh 1000 ms stay");
	dwell.Reset();
	Check(!dwell.Should_Scroll(5, 20000) && !dwell.Should_Scroll(0, 20500)
		&& !dwell.Should_Scroll(5, 21999) && !dwell.Should_Scroll(5, 22998)
		&& dwell.Should_Scroll(5, 22999),
		"after direct right-stick pan, a neutral pointer at the same edge starts a new dwell");

	std::ifstream file(OPENTS_SCROLL_SOURCE);
	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::ifstream controller_file(OPENTS_CONTROLLER_SOURCE);
	std::string controller_source((std::istreambuf_iterator<char>(controller_file)), std::istreambuf_iterator<char>());
	std::size_t const edge_start = source.find("void ScrollClass::Scroll_Edge(Point2D const & point)");
	std::size_t const edge_end = source.find("void ScrollClass::Reset_Edge_Scroll_State", edge_start);
	std::string const edge_body = edge_start == std::string::npos || edge_end == std::string::npos
		? std::string()
		: source.substr(edge_start, edge_end - edge_start);
	std::size_t const pointer_scroll = source.find("static bool Pointer_Scroll_AI(bool apply)");
	std::size_t const pointer_scroll_end = source.find("void ScrollClass::Scroll_AI(void)", pointer_scroll);
	std::string const camera_body = pointer_scroll == std::string::npos || pointer_scroll_end == std::string::npos
		? std::string()
		: source.substr(pointer_scroll, pointer_scroll_end - pointer_scroll);
	std::size_t const scroll_ai_start = source.find("void ScrollClass::Scroll_AI(void)");
	std::size_t const hover_start = source.find("void ScrollClass::Refresh_Hover_Action", scroll_ai_start);
	std::string const scroll_ai_body = scroll_ai_start == std::string::npos || hover_start == std::string::npos
		? std::string()
		: source.substr(scroll_ai_start, hover_start - scroll_ai_start);
	Check(!edge_body.empty()
		&& edge_body.find("Pointer_Edge_Scroll_Source(") != std::string::npos
		&& edge_body.find("Win_Pointer_Camera_Pan_Active()") != std::string::npos
		&& edge_body.find("Reset_Edge_Scroll_State();") != std::string::npos
		&& edge_body.find("ControllerDelayed && !at_screen_edge") < edge_body.find("if (Inertia || at_screen_edge)")
		&& edge_body.find("ControllerEdgeDwell.Should_Scroll(control, Win_Monotonic_Time_Ms())") != std::string::npos
		&& !camera_body.empty()
		&& camera_body.find("if (camera_pan_active)") != std::string::npos
		&& camera_body.find("touchx = 0;") != std::string::npos
		&& camera_body.find("Scale_Touch_Scroll_Offset(touchx") != std::string::npos
		&& !scroll_ai_body.empty()
		&& scroll_ai_body.find("bool const camera_pan_active = Pointer_Scroll_AI(!IgnoreInput);") != std::string::npos
		&& scroll_ai_body.find("KN_RMOUSE) && !camera_pan_active") != std::string::npos
		&& scroll_ai_body.find("!camera_pan_active && Options.AutoScroll") != std::string::npos
		&& controller_source.find("leftx * GAMEPAD_CURSOR_SPEED * elapsed * pointer_boost") != std::string::npos
		&& controller_source.find("rightx * GAMEPAD_CAMERA_SPEED * elapsed") != std::string::npos
		&& controller_source.find("rightx * GAMEPAD_CAMERA_SPEED * elapsed * pointer_boost") == std::string::npos,
		"edge dwell uses monotonic controller ownership, while right-stick camera pan owns its frame");
}


void Test_Sidebar_Safe_Fallback(void)
{
	std::ifstream sidebar_file(OPENTS_SIDEBAR_SOURCE);
	std::string sidebar_source((std::istreambuf_iterator<char>(sidebar_file)), std::istreambuf_iterator<char>());
	std::size_t const toggle_start = sidebar_source.find("void SidebarClass::Controller_Toggle_Sidebar(void)");
	std::size_t const toggle_end = sidebar_source.find("SidebarClass::StripClass::StripClass", toggle_start);
	std::string const toggle_body = toggle_start == std::string::npos || toggle_end == std::string::npos
		? std::string() : sidebar_source.substr(toggle_start, toggle_end - toggle_start);
	Check(!toggle_body.empty()
		&& toggle_body.find("Set_View_Dimensions") == std::string::npos
		&& toggle_body.find("Activate(0)") == std::string::npos
		&& toggle_body.find("VisibleSurface->Fill_Rect") == std::string::npos
		&& toggle_body.find("IsMobileUserCollapsed = true") == std::string::npos,
		"Triangle leaves sidebar and tactical buffer geometry intact");
}


void Test_Native_Cursor_And_Action_Continuity(void)
{
	std::ifstream unit_file(OPENTS_UNIT_SOURCE);
	std::string unit_source((std::istreambuf_iterator<char>(unit_file)), std::istreambuf_iterator<char>());
	std::size_t const action_start = unit_source.find("ActionType UnitClass::What_Action(ObjectClass const * object, bool disallow_force) const");
	std::size_t const action_end = unit_source.find("ActionType UnitClass::What_Action(Cell const & cell", action_start);
	std::string const action_body = action_start == std::string::npos || action_end == std::string::npos
		? std::string()
		: unit_source.substr(action_start, action_end - action_start);
	Check(!action_body.empty()
		&& action_body.find("bool deploying = object == this && (action == ACTION_SELF || action == ACTION_NO_DEPLOY);") != std::string::npos
		&& action_body.find("object->RTTI != RTTI_BUILDING && !deploying") != std::string::npos,
		"the narrow upstream deploy-cursor fix preserves only native self/no-deploy verdicts on a repairing unit");

	std::ifstream cursor_file(OPENTS_CURSOR_SOURCE);
	std::string cursor_source((std::istreambuf_iterator<char>(cursor_file)), std::istreambuf_iterator<char>());
	std::ifstream cursor_header_file(OPENTS_CURSOR_HEADER_SOURCE);
	std::string cursor_header_source((std::istreambuf_iterator<char>(cursor_header_file)), std::istreambuf_iterator<char>());
	std::ifstream mouse_file(OPENTS_MOUSE_HEADER_SOURCE);
	std::string mouse_source((std::istreambuf_iterator<char>(mouse_file)), std::istreambuf_iterator<char>());
	std::ifstream mouse_cpp_file(OPENTS_MOUSE_CPP_SOURCE);
	std::string mouse_cpp_source((std::istreambuf_iterator<char>(mouse_cpp_file)), std::istreambuf_iterator<char>());
	std::ifstream display_file(OPENTS_DISPLAY_SOURCE);
	std::string display_source((std::istreambuf_iterator<char>(display_file)), std::istreambuf_iterator<char>());
	std::size_t const cancel_start = display_source.find("void DisplayClass::Mouse_Right_Release(Point2D const & point)");
	std::size_t const cancel_end = display_source.find("void DisplayClass::Mouse_Left_Up", cancel_start);
	std::string const cancel_body = cancel_start == std::string::npos || cancel_end == std::string::npos
		? std::string()
		: display_source.substr(cancel_start, cancel_end - cancel_start);
	std::size_t const cycle_start = display_source.find("void DisplayClass::Controller_Repair_Sell_Cycle(void)");
	std::size_t const cycle_end = display_source.find("DisplayClass::Closest_Free_Spot", cycle_start);
	std::string const cycle_body = cycle_start == std::string::npos || cycle_end == std::string::npos
		? std::string()
		: display_source.substr(cycle_start, cycle_end - cycle_start);
	std::size_t const override_start = mouse_cpp_source.find("bool MouseClass::Override_Mouse_Shape(MouseType mouse, bool wsmall)");
	std::size_t const override_end = mouse_cpp_source.find("void MouseClass::AI", override_start);
	std::string const override_body = override_start == std::string::npos || override_end == std::string::npos
		? std::string()
		: mouse_cpp_source.substr(override_start, override_end - override_start);
	std::size_t const reset_start = mouse_cpp_source.find("void MouseClass::Init_Clear(void)");
	std::size_t const reset_end = mouse_cpp_source.find("bool MouseClass::Load", reset_start);
	std::string const reset_body = reset_start == std::string::npos || reset_end == std::string::npos
		? std::string()
		: mouse_cpp_source.substr(reset_start, reset_end - reset_start);
	std::size_t const overlay_start = cursor_source.find("bool Win_Cursor_Get_Overlay(WinCursorOverlay * overlay)");
	std::size_t const overlay_end = cursor_source.find("bool Win_Cursor_Is_Dirty", overlay_start);
	std::string const overlay_body = overlay_start == std::string::npos || overlay_end == std::string::npos
		? std::string()
		: cursor_source.substr(overlay_start, overlay_end - overlay_start);
	std::size_t const semantic_set_start = cursor_source.find("void Win_Cursor_Set_Semantic_Mouse_Type(MouseType semantic_mouse_type)");
	std::size_t const semantic_set_end = cursor_source.find("\n}", semantic_set_start);
	std::string const semantic_set_body = semantic_set_start == std::string::npos || semantic_set_end == std::string::npos
		? std::string()
		: cursor_source.substr(semantic_set_start, semantic_set_end - semantic_set_start + 2);
	Check(cursor_source.find("MFCD::Retrieve(\"MOUSE.SHP\")") == std::string::npos
		&& mouse_source.find("Get_Current_Mouse_Shape") == std::string::npos
		&& cursor_source.find("Map.Get_Current_Mouse_Shape()") == std::string::npos
		&& cursor_header_source.find("Win_Cursor_Set_Semantic_Mouse_Type(MouseType semantic_mouse_type)") != std::string::npos
		&& cursor_source.find("static MouseType _SemanticMouseType = MOUSE_NORMAL;") != std::string::npos
		&& !overlay_body.empty()
		&& overlay_body.find("front_end ? (int)MOUSE_NORMAL : (int)_SemanticMouseType") != std::string::npos
		&& overlay_body.find("front_end ? 0 : _CurrentFrame") != std::string::npos
		&& overlay_body.find("Map.") == std::string::npos
		&& !semantic_set_body.empty()
		&& semantic_set_body.find("_SemanticMouseType = semantic_mouse_type;") != std::string::npos
		&& semantic_set_body.find("_OverlayDirty = true;") != std::string::npos
		&& semantic_set_body.find("_CurrentFrame") == std::string::npos
		&& semantic_set_body.find("_CurrentShape") == std::string::npos
		&& semantic_set_body.find("_CurrentHot") == std::string::npos
		&& semantic_set_body.find("_ContentGeneration") == std::string::npos
		&& cursor_source.find("CursorSnapshot type=%d") != std::string::npos
		&& cursor_source.find("_LastDiagnosticSnapshot.ContentGeneration") != std::string::npos
		&& cursor_source.find("snapshot.DrawableAnchorX != _LastDiagnosticSnapshot") == std::string::npos
		&& !override_body.empty()
		&& override_body.find("Mouse_Override_Shape_If_Changed") < override_body.find("Win_Cursor_Set_Semantic_Mouse_Type(CurrentMouseShape)")
		&& override_body.find("return(changed);") != std::string::npos
		&& override_body.find("MouseControl[mouse]") != std::string::npos
		&& override_body.find("Get_Mouse_Hotspot(mouse)") != std::string::npos
		&& override_body.find("MouseCursor->Set_Cursor") != std::string::npos
		&& !reset_body.empty()
		&& reset_body.find("CurrentMouseShape = MOUSE_NORMAL;") != std::string::npos
		&& reset_body.find("_MouseOverrideStarted = false;") != std::string::npos
		&& reset_body.find("Win_Cursor_Set_Semantic_Mouse_Type(MOUSE_NORMAL);") != std::string::npos,
		"native MouseClass MouseType alone feeds diagnostic snapshots and reset returns it to MOUSE_NORMAL");
	Check(!cancel_body.empty()
		&& cancel_body.find("Refresh_Hover_Action") != std::string::npos
		&& cancel_body.find("Set_Default_Mouse(MOUSE_NORMAL") == std::string::npos
		&& !cycle_body.empty()
		&& cycle_body.find("if (IsRepairMode)") != std::string::npos
		&& cycle_body.find("else if (IsSellMode)") != std::string::npos
		&& cycle_body.find("Repair_Mode_Control(1)") != std::string::npos
		&& cycle_body.find("Sell_Mode_Control(1)") != std::string::npos,
		"Circle resolves the native target on cancel and Square cycles DisplayClass Repair/Sell state");
}


void Test_Mouse_Override_Frontier(void)
{
	char loaded_shapes = 0;
	bool startup = false;
	MouseType current = MOUSE_NORMAL;
	bool current_small = false;
	int received_frame = -1;
	Point2D const expected_hotspot(4, 6);
	char const * received_shapes = nullptr;
	bool state_was_uncommitted_when_set = false;
	int animation_frame = 37;
	int const base_frame = 12;

	bool const changed = Mouse_Override_Shape_If_Changed(startup, &loaded_shapes, MOUSE_REPAIR,
		current, false, current_small, expected_hotspot,
		[&]() { animation_frame = 0; },
		[&]() { return(base_frame + animation_frame); },
		[&](Point2D const & hotspot, char const * shapes, int frame) {
			received_shapes = shapes;
			received_frame = frame;
			state_was_uncommitted_when_set = current == MOUSE_NORMAL;
			Check(hotspot.X == expected_hotspot.X && hotspot.Y == expected_hotspot.Y,
				"MouseClass override forwards the native cursor hotspot");
		});

	Check(changed && startup && current == MOUSE_REPAIR && !current_small,
		"MouseClass override commits a changed requested MouseType after Set_Cursor");
	Check(received_shapes == &loaded_shapes && received_frame == base_frame && animation_frame == 0
		&& state_was_uncommitted_when_set,
		"the animation resets before the first frame and hotspot reach the cursor setter");

	bool repeated_set = false;
	bool const repeated = Mouse_Override_Shape_If_Changed(startup, &loaded_shapes, MOUSE_REPAIR,
		current, false, current_small, expected_hotspot,
		[]() {}, []() { return(0); },
		[&](Point2D const &, char const *, int) { repeated_set = true; });
	Check(!repeated && !repeated_set,
		"an unchanged MouseType does not send a redundant shape override");
}


void Test_Cursor_Content_Presentation(void)
{
	CursorContentGeneration generation;
	CursorContentSelection const normal{ 16, 16, 1, 0x1234 };
	Check(generation.Select(normal) && generation.Current() != 0,
		"initial selected cursor image receives a content generation");

	unsigned char rgba[16] = {};
	void const * const same_pixels_address = rgba;
	rgba[0] = 0x11;
	CursorTextureUploadState texture{ 16, 16, generation.Current(), true };
	CursorContentSelection const selection{ 16, 16, 1, 0x5678 };
	Check(generation.Select(selection),
		"different native MOUSE.SHP pixels change content generation at the same dimensions");
	rgba[0] = 0x22;
	Check(same_pixels_address == rgba
		&& Cursor_Texture_Needs_Upload(texture, 16, 16, generation.Current()),
		"same-size native cursor RGBA is uploaded when frame A and frame B reuse one pixel address");

	texture.Generation = generation.Current();
	Check(!generation.Select(selection)
		&& !Cursor_Texture_Needs_Upload(texture, 16, 16, generation.Current()),
		"position-only movement or hotspot changes with unchanged pixels do not request a content upload");

	Check(Cursor_Texture_Needs_Upload(texture, 24, 16, generation.Current())
		&& Cursor_Texture_Needs_Upload(CursorTextureUploadState{}, 16, 16, generation.Current()),
		"missing or resized cursor textures are created from current RGBA content");

	uint64_t const submitted = generation.Current();
	Check(generation.Select(selection, true)
		&& Cursor_Texture_Needs_Upload(texture, 16, 16, generation.Current()),
		"cache flush/rebuild advances the generation and refreshes a same-size texture");
	Check(!generation.Acknowledge(submitted) && generation.Needs_Present(),
		"present acknowledgement cannot clear a newer cursor generation");
	texture.Generation = generation.Current();
	Check(generation.Acknowledge(generation.Current()) && !generation.Needs_Present(),
		"the submitted current generation clears cursor content dirtiness");

	CursorContentSelection const scaled{ 32, 32, 2, 0x5678 };
	Check(generation.Select(scaled)
		&& Cursor_Texture_Needs_Upload(texture, 32, 32, generation.Current()),
		"cursor scale rebuild changes generation and resizes the presented texture");

	char shape_set = 0;
	CursorPresentationSnapshot const select = Make_Cursor_Presentation_Snapshot(
		MOUSE_CAN_SELECT, &shape_set, 10, 4, 6, 2, 32, 32, 0x1234, 8, 100, 80);
	CursorPresentationSnapshot const deploy = Make_Cursor_Presentation_Snapshot(
		MOUSE_DEPLOY, &shape_set, 11, 7, 9, 2, 32, 32, 0x5678, 9, 100, 80);
	CursorPresentationSnapshot const moved = Make_Cursor_Presentation_Snapshot(
		MOUSE_DEPLOY, &shape_set, 11, 7, 9, 2, 32, 32, 0x5678, 9, 137, 121);
	Check(select.DrawableAnchorX == deploy.DrawableAnchorX
		&& select.DrawableAnchorY == deploy.DrawableAnchorY
		&& select.DestinationX != deploy.DestinationX
		&& select.DestinationY != deploy.DestinationY,
		"native hotspot changes can move the sprite top-left while the click anchor stays fixed");
	Check(deploy.DrawableAnchorX == 100 && deploy.DrawableAnchorY == 80
		&& deploy.DestinationX == 86 && deploy.DestinationY == 62
		&& moved.DrawableAnchorX == 137 && moved.DrawableAnchorY == 121,
		"cursor destination is derived from the drawable click anchor and scaled native hotspot");
	Check(select.SemanticMouseType == MOUSE_CAN_SELECT && deploy.SemanticMouseType == MOUSE_DEPLOY
		&& select.Shape == deploy.Shape && select.Frame != deploy.Frame && select.ContentHash != deploy.ContentHash,
		"cursor presentation snapshots retain native semantic type, frame and pixel identity");
}


void Test_Window_To_Cursor_Anchor(void)
{
	VideoScaleInfo const scale{640, 440, 2532, 1170, 415, 0, 1702, 1170, 1702.0f / 640, 1170.0f / 440};
	VideoPoint const window_point{1315, 552};
	VideoPoint const click_game = Window_Pixels_To_Game(scale, window_point);
	VideoPoint const drag_origin_game = Window_Pixels_To_Game(scale, window_point);
	VideoPoint const anchor = Game_To_Drawable_Pixels(scale, click_game);
	CursorPresentationSnapshot const cursor = Make_Cursor_Presentation_Snapshot(
		MOUSE_CAN_MOVE, nullptr, 0, 8, 8, 3, 72, 72, 1, 1, anchor.X, anchor.Y);
	Check(click_game.X == drag_origin_game.X && click_game.Y == drag_origin_game.Y
		&& cursor.DestinationX + cursor.NativeHotX * cursor.DisplayScale == anchor.X
		&& cursor.DestinationY + cursor.NativeHotY * cursor.DisplayScale == anchor.Y
		&& std::abs(anchor.X - window_point.X) <= 3
		&& std::abs(anchor.Y - window_point.Y) <= 3,
		"visible hotspot and drag-selection origin share one drawable anchor under letterboxing and non-integer scale");
	VideoPoint const incorrectly_scaled = Game_To_Drawable_Pixels(scale, anchor);
	Check(std::abs(incorrectly_scaled.X - anchor.X) > 100,
		"a duplicate game-to-drawable transform is detected by the anchor contract");

	std::ifstream mouse_file(OPENTS_WWMOUSE_SOURCE);
	std::string mouse_source((std::istreambuf_iterator<char>(mouse_file)), std::istreambuf_iterator<char>());
	std::size_t const start = mouse_source.find("void WWMouseClass::Convert_Coordinate(int & x, int & y) const");
	std::size_t const end = mouse_source.find("void WWMouseClass::Get_Bounded_Position", start);
	std::string const conversion = start == std::string::npos || end == std::string::npos
		? std::string() : mouse_source.substr(start, end - start);
	std::ifstream window_file(OPENTS_WINDOW_SOURCE);
	std::string window_source((std::istreambuf_iterator<char>(window_file)), std::istreambuf_iterator<char>());
	Check(conversion.find("ScreenToClient(Window, &point)") != std::string::npos
		&& conversion.find("Window_Point_To_Game(point)") != std::string::npos
		&& conversion.find("ConfiningRect.X") == std::string::npos
		&& window_source.find("SDL_GetWindowPixelDensity(main->Handle)") != std::string::npos,
		"polled pointer uses the current window origin and backing density before the shared game transform");
}


void Test_Cursor_Sequences_Reach_Presentation(void)
{
	char loaded_shapes = 0;
	bool startup = false;
	MouseType current = MOUSE_NORMAL;
	bool current_small = false;
	CursorContentGeneration generation;
	CursorTextureUploadState texture;
	Point2D const pointer(211, 127);
	int uploads = 0;
	std::vector<CursorPresentationSnapshot> presented;

	auto apply = [&](MouseType requested) {
		int const selected_frame = 18 + (int)requested;
		int const hot_x = 1 + ((int)requested % 5);
		int const hot_y = 1 + ((int)requested % 4);
	char const * const shape_data = &loaded_shapes;
		uint64_t const hash = ((uint64_t)(shape_data != nullptr) << 32)
			| (uint32_t)selected_frame;
		CursorPresentationSnapshot result;
		bool const changed = Mouse_Override_Shape_If_Changed(startup, shape_data, requested,
			current, false, current_small, Point2D(0, 0),
			[]() {}, [&]() { return(selected_frame); },
			[&](Point2D const &, char const * shapes, int selected_frame_for_cursor) {
				bool const content_changed = generation.Select(CursorContentSelection{ 64, 64, 2, hash });
				if (content_changed && Cursor_Texture_Needs_Upload(texture, 64, 64, generation.Current())) {
					texture = CursorTextureUploadState{ 64, 64, generation.Current(), true };
					uploads++;
				}
				Check(shapes == &loaded_shapes && selected_frame_for_cursor == selected_frame,
					"MouseClass override forwards the actual native shape and selected frame");
			});
		result = Make_Cursor_Presentation_Snapshot(current, shape_data,
			selected_frame, hot_x, hot_y, 2, 64, 64, hash,
			generation.Current(), pointer.X, pointer.Y);
		presented.push_back(result);
		Check(changed && current == requested && !presented.empty()
			&& presented.back().SemanticMouseType == requested,
			"native MouseType reaches the cursor snapshot for each resolved tactical state");
		CursorPresentationSnapshot const presentation_only_change = Make_Cursor_Presentation_Snapshot(
			current, shape_data, selected_frame + 1, hot_x + 1, hot_y + 1, 2,
			64, 64, hash, generation.Current(), pointer.X + 17, pointer.Y - 9);
		Check(presentation_only_change.SemanticMouseType == result.SemanticMouseType
			&& presentation_only_change.Frame != result.Frame
			&& presentation_only_change.NativeHotX != result.NativeHotX
			&& presentation_only_change.NativeHotY != result.NativeHotY
			&& presentation_only_change.DrawableAnchorX != result.DrawableAnchorX
			&& presentation_only_change.DrawableAnchorY != result.DrawableAnchorY,
			"frame, hotspot and pointer-only presentation changes preserve the committed native MouseType");
		return(result);
	};

	MouseType const resolved_states[] = {
		Action_Cursor_Shape(ACTION_NONE, false, false, false),
		Action_Cursor_Shape(ACTION_SELECT, false, false, false),
		Action_Cursor_Shape(ACTION_MOVE, false, false, false),
		Action_Cursor_Shape(ACTION_ATTACK, false, false, false),
		Action_Cursor_Shape(ACTION_ENTER, false, false, false),
		Action_Cursor_Shape(ACTION_SELF, false, false, false),
		Action_Cursor_Shape(ACTION_NO_DEPLOY, false, false, false),
		Action_Cursor_Shape(ACTION_REPAIR, false, false, false),
		Action_Cursor_Shape(ACTION_NO_REPAIR, false, false, false),
		Action_Cursor_Shape(ACTION_SELL, false, false, false),
		Action_Cursor_Shape(ACTION_SELL_UNIT, false, false, false)
	};
	MouseType const expected_states[] = {
		MOUSE_NORMAL, MOUSE_CAN_SELECT, MOUSE_CAN_MOVE, MOUSE_CAN_ATTACK, MOUSE_ENTER,
		MOUSE_DEPLOY, MOUSE_NO_DEPLOY, MOUSE_REPAIR, MOUSE_NO_REPAIR, MOUSE_SELL_BACK, MOUSE_SELL_UNIT
	};
	bool semantic_resolution_matches = true;
	for (size_t index = 0; index < sizeof(resolved_states) / sizeof(resolved_states[0]); index++) {
		semantic_resolution_matches = semantic_resolution_matches && resolved_states[index] == expected_states[index];
		apply(resolved_states[index]);
	}
	Check(semantic_resolution_matches && presented.size() == 11 && uploads == 11
		&& std::all_of(presented.begin(), presented.end(), [&](CursorPresentationSnapshot const & snapshot) {
			return(snapshot.DrawableAnchorX == pointer.X && snapshot.DrawableAnchorY == pointer.Y);
		})
		&& presented[0].DestinationX != presented[5].DestinationX
		&& presented[0].DestinationY != presented[5].DestinationY,
		"Idle, select, move, attack, enter, deploy, Repair and Sell reach native rasters at one fixed click anchor");

	// Circle cancels the current mode and re-resolves the same point before presentation.
	size_t const before_circle = presented.size();
	CursorPresentationSnapshot const after_circle = apply(Action_Cursor_Shape(ACTION_MOVE, false, false, false));
	Check(presented.size() == before_circle + 1 && after_circle.SemanticMouseType == MOUSE_CAN_MOVE,
		"Circle cancellation presents the freshly resolved native hover cursor without an ordinary-cursor frame");

	// Square is driven from native Repair/Sell mode state; the pointer coordinate never changes.
	RepairSellModeCursor square = RepairSellModeCursor::Normal;
	square = Next_RepairSell_Mode_Cursor(square);
	CursorPresentationSnapshot const repair = apply(RepairSell_Mode_Cursor_Seed(square));
	square = Next_RepairSell_Mode_Cursor(square);
	CursorPresentationSnapshot const sell = apply(RepairSell_Mode_Cursor_Seed(square));
	square = Next_RepairSell_Mode_Cursor(square);
	size_t const before_square_cancel = presented.size();
	CursorPresentationSnapshot const square_cancel = apply(
		Action_Cursor_Shape(ACTION_ATTACK, false, false, true));
	Check(square == RepairSellModeCursor::Normal && repair.SemanticMouseType == MOUSE_REPAIR
		&& sell.SemanticMouseType == MOUSE_SELL_BACK
		&& square_cancel.SemanticMouseType == MOUSE_STAY_ATTACK
		&& presented.size() == before_square_cancel + 1
		&& square_cancel.DrawableAnchorX == pointer.X && square_cancel.DrawableAnchorY == pointer.Y,
		"Square cycles native Normal to Repair to Sell and immediately re-hovers without moving the pointer");
}


void Test_Legacy_Controller_Service_Order(void)
{
	std::vector<char> order;
	int services = 0;
	int discarded = 0;
	int drains = 0;
	for (int update = 0; update < 3; update++) {
		MSEngine_Service_Legacy_Input(
			[&]() { services++; order.push_back('S'); },
			[&]() { discarded++; order.push_back('D'); },
			[&]() {
				drains++;
				order.push_back('M');
				order.push_back('m');
			});
	}
	Check(services == 3 && discarded == 3 && drains == 3
		&& order == std::vector<char>({ 'S', 'D', 'M', 'm', 'S', 'D', 'M', 'm', 'S', 'D', 'M', 'm' }),
		"legacy controller input is serviced once before each message drain and Square/Triangle edges are discarded");

	std::ifstream file(OPENTS_MSENGINE_SOURCE);
	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::size_t const start = source.find("void MSEngine::Wait_Delay(int delay)");
	std::size_t const end = source.find("void MSEngine::Wait_For_Focus(void)", start);
	std::string const body = start == std::string::npos || end == std::string::npos
		? std::string()
		: source.substr(start, end - start);
	std::size_t const service = body.find("Win_Gamepad_Service();");
	std::size_t const discard = body.find("Win_Gamepad_Discard_Actions();");
	std::size_t const drain = body.find("Windows_Message_Handler();");
	Check(!body.empty() && service != std::string::npos && discard > service && drain > discard
		&& body.find("Win_Gamepad_Service();", service + 1) == std::string::npos
		&& body.find("PeekMessage") == std::string::npos,
		"MSEngine Wait_Delay keeps the one service opportunity outside Windows message draining");
}

}


int main(void)
{
	Test_Action_Cursor_Resolution();
	Test_Repair_Sell_Seeds_And_Hover();
	Test_Square_Mode_Cycle();
	Test_Mode_Icon_Ownership();
	Test_Controller_ScrollRate();
	Test_Sidebar_Safe_Fallback();
	Test_Native_Cursor_And_Action_Continuity();
	Test_Mouse_Override_Frontier();
	Test_Cursor_Content_Presentation();
	Test_Window_To_Cursor_Anchor();
	Test_Cursor_Sequences_Reach_Presentation();
	Test_Legacy_Controller_Service_Order();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
