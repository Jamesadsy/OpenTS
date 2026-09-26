/*******************************************************************************
 *                                O P E N T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "ui/savebrowsernavigation.h"
#include "ui/uimodalinput.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>


namespace
{

class NullRender final : public Rml::RenderInterface
{
public:
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override { return(1); }
	void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override {}
	void ReleaseGeometry(Rml::CompiledGeometryHandle) override {}
	Rml::TextureHandle LoadTexture(Rml::Vector2i & dimensions, Rml::String const &) override
	{
		dimensions = Rml::Vector2i(1, 1);
		return(1);
	}
	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override { return(1); }
	void ReleaseTexture(Rml::TextureHandle) override {}
	void EnableScissorRegion(bool) override {}
	void SetScissorRegion(Rml::Rectanglei) override {}
};


struct SaveScreen
{
	struct Entry
	{
		std::string Description;
		std::string Date;
		std::string Time;
	};

	std::string Description;
	std::vector<std::string> PendingDescriptions;
	std::vector<Entry> Entries;
	int Selected = -1;
	bool CanAct = false;
};


bool Check(bool condition, char const * description)
{
	if (condition) return(true);
	std::fprintf(stderr, "FAIL: %s\n", description);
	return(false);
}


class TextFixture
{
	public:
		TextFixture(bool two_way, std::string initial = "") : TwoWay(two_way)
		{
			Screen.Description = std::move(initial);
			Context = Rml::CreateContext("save-text-contract", Rml::Vector2i(640, 480));
			if (Context == nullptr) return;

			Rml::DataModelConstructor constructor = Context->CreateDataModel("missionsave");
			if (!constructor) return;
			if (auto entry = constructor.RegisterStruct<SaveScreen::Entry>()) {
				entry.RegisterMember("description", &SaveScreen::Entry::Description);
				entry.RegisterMember("date", &SaveScreen::Entry::Date);
				entry.RegisterMember("time", &SaveScreen::Entry::Time);
			}
			constructor.RegisterArray<std::vector<SaveScreen::Entry>>();
			if (!constructor.Bind("description", &Screen.Description)
				|| !constructor.Bind("entries", &Screen.Entries)
				|| !constructor.Bind("selected", &Screen.Selected)
				|| !constructor.Bind("canact", &Screen.CanAct)) return;
			if (!TwoWay && !constructor.BindEventCallback("describe",
				[this](Rml::DataModelHandle, Rml::Event & event, Rml::VariantList const &) {
					Screen.PendingDescriptions.push_back(event.GetParameter<Rml::String>("value", Rml::String()));
				})) return;
			Model = constructor.GetModelHandle();

			std::ifstream file(SAVE_TEXT_RML_PATH, std::ios::binary);
			if (!file) return;
			std::string rml((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			if (!TwoWay) {
				std::string const binding = "data-value=\"description\"";
				size_t const binding_start = rml.find(binding);
				if (binding_start == std::string::npos) return;
				rml.replace(binding_start, binding.size(),
					"data-attr-value=\"description\" data-event-change=\"describe()\"");
			}

			// Resolve the production stylesheet links just as the app's file interface does.
			Document = Context->LoadDocumentFromMemory(rml, "missionsave.rml");
			if (Document == nullptr) return;
			Document->Show();
			Field = rmlui_dynamic_cast<Rml::ElementFormControlInput *>(Document->GetElementById("description"));
			if (Field == nullptr) return;

			// Match the first UI_Run_Modal update/sync/render pass before the field is used.
			Context->Update();
			SyncDescription();
		}

		~TextFixture()
		{
			if (Context != nullptr) Rml::RemoveContext("save-text-contract");
		}

		bool Ready() const { return(Context != nullptr && Document != nullptr && Field != nullptr); }

		void SyncDescription()
		{
			Model.DirtyVariable("description");
			Context->Render();
		}

		void DrainDescriptions()
		{
			for (std::string const & value : Screen.PendingDescriptions) Screen.Description = value;
			Screen.PendingDescriptions.clear();
		}

		void TypeBatch(std::string const & text)
		{
			for (unsigned char character : text) {
				UI_Modal_Input::Text_Consumed(*Context, (Rml::Character)character, true, false);
			}
		}

		void TypeWithFramePerCharacter(std::string const & text)
		{
			for (unsigned char character : text) {
				UI_Modal_Input::Text_Consumed(*Context, (Rml::Character)character, true, false);
				Context->Update();
				DrainDescriptions();
				SyncDescription();
			}
		}

		void SetDescription(std::string const & value)
		{
			Screen.Description = value;
			Field->SetValue(value);
			Context->Update();
			SyncDescription();
		}

		bool TwoWay;
		SaveScreen Screen;
		Rml::Context * Context = nullptr;
		Rml::ElementDocument * Document = nullptr;
		Rml::ElementFormControlInput * Field = nullptr;
		Rml::DataModelHandle Model;
};


void Print_State(char const * when, TextFixture const & fixture)
{
	int selection_start = -1;
	int selection_end = -1;
	fixture.Field->GetSelection(&selection_start, &selection_end, nullptr);
	std::printf("%s: field='%s' model='%s' reported-selection=(%d,%d)\n", when,
		fixture.Field->GetValue().c_str(), fixture.Screen.Description.c_str(), selection_start, selection_end);
}


bool Prove_Legacy_Rotation(std::string const & typed, std::string const & rotated)
{
	TextFixture fixture(false);
	if (!Check(fixture.Ready(), "legacy save RML fixture initializes")) return(false);
	fixture.Field->Focus();

	// The modal pump handles native text before Context::Update, then drains presenter
	// intents and synchronizes the view. The old one-way binding first writes the stale
	// empty model value over T, then an idle frame restores T after the layout has clamped
	// WidgetTextInput::absolute_cursor_index to zero.
	UI_Modal_Input::Text_Consumed(*fixture.Context, (Rml::Character)typed[0], true, false);
	if (!Check(fixture.Field->GetValue() == typed.substr(0, 1)
		&& fixture.Screen.Description.empty() && fixture.Screen.PendingDescriptions.size() == 1,
		"first WM_CHAR changes the RmlUi field and queues the presenter description")) return(false);
	fixture.Context->Update();
	if (!Check(fixture.Field->GetValue().empty(),
		"Context::Update writes the stale empty model value into the focused input")) return(false);
	fixture.DrainDescriptions();
	if (!Check(fixture.Screen.Description == typed.substr(0, 1),
		"presenter drain catches up with the first input character")) return(false);
	fixture.SyncDescription();
	fixture.Context->Update();
	fixture.SyncDescription();
	Print_State("legacy after first-character model writeback", fixture);
	int selection_start = -1;
	fixture.Field->GetSelection(&selection_start, nullptr, nullptr);
	if (!Check(fixture.Field->GetValue() == typed.substr(0, 1) && selection_start == 1,
		"reported RmlUi selection still claims the caret is after the restored first glyph")) return(false);

	UI_Modal_Input::Text_Consumed(*fixture.Context, (Rml::Character)typed[1], true, false);
	std::string const insertion_probe = typed.substr(1, 1) + typed.substr(0, 1);
	Print_State("legacy after second WM_CHAR", fixture);
	if (!Check(fixture.Field->GetValue() == insertion_probe,
		"actual insertion caret is zero even though GetSelection reports one")) return(false);
	fixture.Context->Update();
	fixture.DrainDescriptions();
	fixture.SyncDescription();
	fixture.Context->Update();
	fixture.SyncDescription();
	for (unsigned char character : typed.substr(2)) {
		UI_Modal_Input::Text_Consumed(*fixture.Context, (Rml::Character)character, true, false);
		fixture.Context->Update();
		fixture.DrainDescriptions();
		fixture.SyncDescription();
		fixture.Context->Update();
		fixture.SyncDescription();
	}
	Print_State("legacy settled", fixture);
	return(Check(fixture.Field->GetValue() == rotated && fixture.Screen.Description == rotated,
		"one-way RmlUi model synchronization reproduces the observed first-character rotation"));
}


bool Prove_Two_Way_Name(std::string const & typed)
{
	TextFixture fixture(true);
	if (!Check(fixture.Ready(), "production two-way save RML fixture initializes")) return(false);
	fixture.Field->Focus();

	for (size_t index = 0; index < typed.size(); ++index) {
		UI_Modal_Input::Text_Consumed(*fixture.Context, (Rml::Character)typed[index], true, false);
		if (index == 0 && !Check(fixture.Field->GetValue() == typed.substr(0, 1)
			&& fixture.Screen.Description == typed.substr(0, 1),
			"RmlUi data-value controller writes the first typed character into the model immediately")) return(false);
		fixture.Context->Update();
		fixture.DrainDescriptions();
		fixture.SyncDescription();
		if (index + 1 < typed.size()) {
			// Model/view update on idle frames between physical keypresses.
			fixture.Context->Update();
			fixture.SyncDescription();
		}
	}
	Print_State("two-way exact result", fixture);
	return(Check(fixture.Field->GetValue() == typed && fixture.Screen.Description == typed,
		"two-way sync preserves the exact visible field and model text"));
}


bool Check_Editing_And_Save_Text(void)
{
	TextFixture fixture(true);
	if (!Check(fixture.Ready(), "editing save RML fixture initializes")) return(false);
	fixture.Field->Focus();
	fixture.TypeWithFramePerCharacter("RmlUi-Order 42");
	if (!Check(fixture.Field->GetValue() == "RmlUi-Order 42"
		&& fixture.Screen.Description == "RmlUi-Order 42",
		"ordinary alphanumeric and punctuation order survives frame-separated input")) return(false);

	fixture.Field->SetSelectionRange((int)fixture.Field->GetValue().size(), (int)fixture.Field->GetValue().size());
	fixture.Context->ProcessKeyDown(Rml::Input::KI_BACK, 0);
	fixture.Context->ProcessKeyUp(Rml::Input::KI_BACK, 0);
	fixture.Context->Update();
	fixture.SyncDescription();
	if (!Check(fixture.Field->GetValue() == "RmlUi-Order 4" && fixture.Screen.Description == "RmlUi-Order 4",
		"Backspace deletes the character before the caret and synchronizes the model")) return(false);

	fixture.SetDescription("RmlUi-Order 42");
	fixture.Field->SetSelectionRange(4, 4);
	fixture.TypeBatch("X");
	fixture.Context->Update();
	fixture.SyncDescription();
	if (!Check(fixture.Field->GetValue() == "RmlUXi-Order 42"
		&& fixture.Screen.Description == "RmlUXi-Order 42",
		"caret editing inserts at the selected position without moving prior characters")) return(false);

	fixture.SetDescription("Old save");
	fixture.Field->SetSelectionRange(0, (int)fixture.Field->GetValue().size());
	fixture.TypeBatch("New");
	fixture.Context->Update();
	fixture.SyncDescription();
	if (!Check(fixture.Field->GetValue() == "New" && fixture.Screen.Description == "New",
		"replacing selected text leaves only the replacement")) return(false);

	fixture.SetDescription("Existing save");
	fixture.Field->SetSelectionRange((int)fixture.Field->GetValue().size(), (int)fixture.Field->GetValue().size());
	fixture.TypeWithFramePerCharacter(" + edit");
	if (!Check(fixture.Field->GetValue() == "Existing save + edit"
		&& fixture.Screen.Description == "Existing save + edit",
		"appending to an existing description preserves the whole value")) return(false);

	// Mirror SaveBrowserView::Sync when a selected existing row supplies new text: assign the
	// external value before focusing/selecting, so the selection applies to the new description.
	fixture.Screen.Description = "Existing mission description";
	fixture.Model.DirtyVariable("description");
	fixture.Field->SetValue(fixture.Screen.Description);
	fixture.Field->Focus();
	fixture.Field->Select();
	fixture.Context->Update();
	fixture.SyncDescription();
	fixture.TypeBatch("Edited");
	fixture.Context->Update();
	fixture.SyncDescription();
	if (!Check(fixture.Field->GetValue() == "Edited" && fixture.Screen.Description == "Edited",
		"selecting an existing save then replacing its description remains coherent")) return(false);

	fixture.Screen.Description = "stale presenter text";
	fixture.Model.DirtyVariable("description");
	fixture.Field->SetValue("visible save description");
	std::string const text_used_by_save = Save_Browser_Field_Text(*fixture.Field);
	if (!Check(text_used_by_save == "visible save description",
		"Save's field-text accessor returns the exact visible input text")) return(false);
	fixture.Screen.PendingDescriptions.push_back(text_used_by_save);
	fixture.DrainDescriptions();
	return(Check(fixture.Screen.Description == "visible save description",
		"Save queues visible field text ahead of its accept action"));
}

}


int main(void)
{
	std::filesystem::current_path(SAVE_TEXT_UI_PATH);
	NullRender renderer;
	Rml::SetRenderInterface(&renderer);
	if (!Rml::Initialise()) return(1);
	if (!Rml::LoadFontFace(SAVE_TEXT_FONT_PATH)) {
		std::fprintf(stderr, "Could not load the test font: %s\n", SAVE_TEXT_FONT_PATH);
		Rml::Shutdown();
		return(1);
	}

	bool const passed = Prove_Legacy_Rotation("Test", "estT")
		&& Prove_Legacy_Rotation("Save1", "ave1S")
		&& Prove_Two_Way_Name("Test")
		&& Prove_Two_Way_Name("Save1")
		&& Check_Editing_And_Save_Text();

	Rml::Shutdown();
	return(passed ? 0 : 1);
}
