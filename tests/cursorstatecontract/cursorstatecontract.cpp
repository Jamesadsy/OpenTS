/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

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
#include "options.h"
#include "pointerscrollpolicy.hh"

namespace
{

constexpr int SCROLL_RATE_SETTING_COUNT = 8;
static_assert(OptionsClass::MAX_SCROLL_SETTING == SCROLL_RATE_SETTING_COUNT);
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
	static double const expected[] = { 1.000, 0.906, 0.820, 0.743, 0.673, 0.610, 0.552, 0.500 };
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
	Check(table_matches, "all eight controller ScrollRate settings match the accepted geometric curve");
	Check(monotonic && scales[0] == 1.0 && scales[7] == 0.5,
		"controller ScrollRate is monotonic with 1.0 and 0.5 endpoint scales");
	Check(Controller_ScrollRate_Scale(-1, SCROLL_RATE_SETTING_COUNT) == scales[0]
		&& Controller_ScrollRate_Scale(8, SCROLL_RATE_SETTING_COUNT) == scales[7],
		"controller speed remains bounded outside the slider's ScrollRate values");

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
	Check(controller_travel == 3 && controller_remainder > 0.3 && controller_remainder < 0.4,
		"controller pan keeps useful fractional travel at a low/mid ScrollRate setting");
	Check(Pointer_Edge_Scroll_Allowed(false) && !Pointer_Edge_Scroll_Allowed(true),
		"controller-owned pointer suppresses native edge scroll while hardware pointer ownership permits it");

	std::ifstream file(OPENTS_SCROLL_SOURCE);
	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::size_t const auto_scroll = source.find("if (Options.AutoScroll && !Debug_Map");
	std::size_t const edge_scroll = source.find("Scroll_Edge(point);", auto_scroll);
	std::string const gate = auto_scroll == std::string::npos || edge_scroll == std::string::npos
		? std::string()
		: source.substr(auto_scroll, edge_scroll - auto_scroll);
	Check(!gate.empty()
		&& gate.find("Pointer_Edge_Scroll_Allowed(Win_Pointer_Is_Controller_Owner())") != std::string::npos
		&& Pointer_Edge_Scroll_Allowed(false),
		"Scroll_AI gates only controller-owned pointer edge scroll and retains the hardware path");
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
	char shape_set = 0;
	CursorContentGeneration generation;
	CursorContentSelection const normal{ &shape_set, 10, 2, 3, 1 };
	Check(generation.Select(normal) && generation.Current() != 0,
		"initial selected cursor image receives a content generation");

	unsigned char rgba[16] = {};
	void const * const same_pixels_address = rgba;
	rgba[0] = 0x11;
	CursorTextureUploadState texture{ 16, 16, generation.Current(), true };
	CursorContentSelection const selection{ &shape_set, 11, 2, 3, 1 };
	Check(generation.Select(selection),
		"a same-dimension native cursor frame changes the content generation");
	rgba[0] = 0x22;
	Check(same_pixels_address == rgba
		&& Cursor_Texture_Needs_Upload(texture, 16, 16, generation.Current()),
		"same-size RGBA content is uploaded when frame A and frame B reuse one pixel address");

	texture.Generation = generation.Current();
	Check(!generation.Select(selection)
		&& !Cursor_Texture_Needs_Upload(texture, 16, 16, generation.Current()),
		"position-only movement with unchanged cursor content does not request an upload");

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

	CursorContentSelection const scaled{ &shape_set, 11, 2, 3, 2 };
	Check(generation.Select(scaled), "cursor scale rebuild changes the content generation");
}


void Test_Cursor_Sequences_Reach_Presentation(void)
{
	char loaded_shapes = 0;
	bool startup = false;
	MouseType current = MOUSE_NORMAL;
	bool current_small = false;
	CursorContentGeneration generation;
	CursorTextureUploadState texture;
	Point2D const hotspot(3, 4);
	int uploads = 0;
	int frame = 0;

	auto apply = [&](MouseType requested) {
		int const selected_frame = ++frame;
		bool const changed = Mouse_Override_Shape_If_Changed(startup, &loaded_shapes, requested,
			current, false, current_small, hotspot,
			[]() {}, [&]() { return(selected_frame); },
			[&](Point2D const & selected_hotspot, char const * shapes, int selected_frame_for_cursor) {
				bool const content_changed = generation.Select(CursorContentSelection{
					shapes, selected_frame_for_cursor, selected_hotspot.X, selected_hotspot.Y, 1 });
				if (content_changed && Cursor_Texture_Needs_Upload(texture, 32, 32, generation.Current())) {
					texture = CursorTextureUploadState{ 32, 32, generation.Current(), true };
					uploads++;
				}
			});
		return(changed);
	};

	bool const normal_selected = apply(Action_Cursor_Shape(ACTION_NONE, false, false, false));
	bool const selection_selected = apply(Action_Cursor_Shape(ACTION_SELECT, false, false, false));
	bool const move_selected = apply(Action_Cursor_Shape(ACTION_MOVE, false, false, false));
	Check(normal_selected && selection_selected && move_selected && uploads == 3,
		"normal, selection and move cursors each reach the texture upload contract");

	int const before_repair = uploads;
	MouseType const repair_seed = RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Repair);
	MouseType const sell_seed = RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Sell);
	apply(repair_seed);
	apply(sell_seed);
	apply(RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Normal));
	Check(uploads == before_repair + 3,
		"Repair and Sell immediate seeds use the same native presentation contract");

	int const before_square = uploads;
	RepairSellModeCursor square = RepairSellModeCursor::Normal;
	square = Next_RepairSell_Mode_Cursor(square);
	apply(RepairSell_Mode_Cursor_Seed(square));
	square = Next_RepairSell_Mode_Cursor(square);
	apply(RepairSell_Mode_Cursor_Seed(square));
	square = Next_RepairSell_Mode_Cursor(square);
	apply(RepairSell_Mode_Cursor_Seed(square));
	Check(square == RepairSellModeCursor::Normal && uploads == before_square + 3,
		"Square Normal to Repair to Sell to Normal propagates cursor content changes");
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
	Test_Mouse_Override_Frontier();
	Test_Cursor_Content_Presentation();
	Test_Cursor_Sequences_Reach_Presentation();
	Test_Legacy_Controller_Service_Order();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
