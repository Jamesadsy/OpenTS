/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

// Synthetic, no-owner-data proof for the modal RmlUi input ordering contract.

#include "ui/uimodalinput.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/EventListener.h>
#include <RmlUi/Core/Input.h>
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/Types.h>

#include <cstdio>


namespace
{

int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-66s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}


class NullRender final : public Rml::RenderInterface
{
public:
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>,
		Rml::Span<const int>) override
	{
		return(1);
	}

	void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override
	{
	}

	void ReleaseGeometry(Rml::CompiledGeometryHandle) override
	{
	}

	Rml::TextureHandle LoadTexture(Rml::Vector2i & dimensions, Rml::String const &) override
	{
		dimensions = Rml::Vector2i(1, 1);
		return(1);
	}

	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override
	{
		return(1);
	}

	void ReleaseTexture(Rml::TextureHandle) override
	{
	}

	void EnableScissorRegion(bool) override
	{
	}

	void SetScissorRegion(Rml::Rectanglei) override
	{
	}
};


class ReturnListener final : public Rml::EventListener
{
public:
	void ProcessEvent(Rml::Event & event) override
	{
		if (event == Rml::EventId::Keydown
			&& event.GetParameter<int>("key_identifier", Rml::Input::KI_UNKNOWN)
				== Rml::Input::KI_RETURN) {
			ReturnDown++;
		}
	}

	int ReturnDown = 0;
};

}


int main(void)
{
	NullRender renderer;
	Rml::SetRenderInterface(&renderer);
	Check(Rml::Initialise(), "RmlUi initialises for the modal input contract");
	if (Failures != 0) {
		return(1);
	}

	Rml::Context * context = Rml::CreateContext("modal-input-contract", Rml::Vector2i(640, 480));
	Check(context != NULL, "modal input contract creates an RmlUi context");
	if (context == NULL) {
		Rml::Shutdown();
		return(1);
	}

	Rml::ElementDocument * document = context->LoadDocumentFromMemory(
		"<rml><body><input type='text' id='field' /></body></rml>");
	Check(document != NULL, "modal input contract loads a text field document");
	if (document == NULL) {
		Rml::RemoveContext("modal-input-contract");
		Rml::Shutdown();
		return(1);
	}
	document->Show();
	Rml::ElementFormControlInput * field = rmlui_dynamic_cast<Rml::ElementFormControlInput *>(
		document->GetElementById("field"));
	Check(field != NULL, "modal input contract finds the text field");
	if (field == NULL) {
		Rml::RemoveContext("modal-input-contract");
		Rml::Shutdown();
		return(1);
	}
	Check(field->Focus(), "modal input contract focuses the text field");

	ReturnListener return_listener;
	document->AddEventListener(Rml::EventId::Keydown, &return_listener, true);

	for (char const character : {'A', 'B', 'C', '1', '2', '3'}) {
		Check(UI_Modal_Input::Text_Consumed(*context, (Rml::Character)character, true, false),
			"modal text input is consumed after RmlUi receives it");
	}
	Check(field->GetValue() == "ABC123", "modal field receives ABC123 exactly once");

	Check(UI_Modal_Input::Key_Down_Consumed(*context, Rml::Input::KI_BACK, 0, true, false),
		"modal Backspace is dispatched before modal consumption");
	Check(field->GetValue() == "ABC12", "modal Backspace edits the focused field once");

	field->SetSelectionRange(5, 5);
	Check(UI_Modal_Input::Key_Down_Consumed(*context, Rml::Input::KI_LEFT, 0, true, false),
		"modal Left navigation is dispatched before modal consumption");
	int selection_start = 0;
	int selection_end = 0;
	Rml::String selected_text;
	field->GetSelection(&selection_start, &selection_end, &selected_text);
	Check(selection_start == 4 && selection_end == 4,
		"modal Left moves the focused field caret once");

	Check(UI_Modal_Input::Key_Down_Consumed(*context, Rml::Input::KI_RIGHT, 0, true, false),
		"modal Right navigation is dispatched before modal consumption");
	field->GetSelection(&selection_start, &selection_end, &selected_text);
	Check(selection_start == 5 && selection_end == 5,
		"modal Right moves the focused field caret once");

	Check(UI_Modal_Input::Key_Down_Consumed(*context, Rml::Input::KI_RETURN, 0, true, false),
		"modal Return is dispatched before modal consumption");
	Check(return_listener.ReturnDown == 1, "modal Return reaches the document once");
	Check(UI_Modal_Input::Key_Up_Consumed(*context, Rml::Input::KI_RETURN, 0, true, false),
		"modal Return key-up is dispatched before modal consumption");
	Check(UI_Modal_Input::Key_Down_Consumed(*context, Rml::Input::KI_RETURN, 0, false, false),
		"nonmodal RmlUi-consumed Return remains shell-consumed");
	Check(return_listener.ReturnDown == 2, "nonmodal RmlUi-consumed Return reaches the document once");

	Rml::String const before_capture = field->GetValue();
	Check(UI_Modal_Input::Text_Consumed(*context, Rml::Character('Z'), true, true),
		"developer keyboard capture still consumes text before RmlUi");
	Check(field->GetValue() == before_capture,
		"developer keyboard capture does not mutate the modal field");

	Rml::Context * unconsumed_context = Rml::CreateContext(
		"modal-input-contract-unconsumed", Rml::Vector2i(640, 480));
	Check(unconsumed_context != NULL, "modal input contract creates the unconsumed RmlUi fixture");
	if (unconsumed_context != NULL) {
		Check(unconsumed_context->ProcessKeyDown(Rml::Input::KI_F1, 0),
			"empty RmlUi context leaves KI_F1 genuinely unconsumed");
		Check(!UI_Modal_Input::Key_Down_Consumed(
			*unconsumed_context, Rml::Input::KI_F1, 0, false, false),
			"nonmodal genuinely unconsumed key keeps the legacy propagation result");
		Check(UI_Modal_Input::Key_Down_Consumed(
			*unconsumed_context, Rml::Input::KI_F1, 0, true, false),
			"modal override consumes after RmlUi receives an unconsumed key");
		Rml::RemoveContext("modal-input-contract-unconsumed");
	}

	Rml::RemoveContext("modal-input-contract");
	Rml::Shutdown();
	return(Failures == 0 ? 0 : 1);
}
