/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "uirmlview.h"

#include <memory>
#include <utility>


// A caller-owned, typed front-end screen that can be resumed by a host callback. The
// implementation owns the view and may also own its presenter; JavaScript sees neither.
class UIFrontendContinuation
{
	public:
		virtual ~UIFrontendContinuation(void) = default;
		virtual OpenTS_UIModal::UIModalStepResult Step(void) noexcept = 0;
		virtual UIResult const & Result(void) const noexcept = 0;
		virtual bool Finish(void) noexcept = 0;
};


class UIImmediateFrontendContinuation final : public UIFrontendContinuation
{
	public:
		explicit UIImmediateFrontendContinuation(UIResult result) : m_Result(result) {}
		OpenTS_UIModal::UIModalStepResult Step(void) noexcept override
		{
			return(OpenTS_UIModal::UIModalStepResult::Complete);
		}
		UIResult const & Result(void) const noexcept override { return(m_Result); }
		bool Finish(void) noexcept override { m_Finished = true; return(true); }
		bool Is_Finished(void) const noexcept { return(m_Finished); }
	private:
		UIResult m_Result;
		bool m_Finished = false;
};


// Shared typed lifetime owner for the simple modal callers. Presenter is declared and
// destroyed before View, so a self-owned presenter outlives the view's data model.
template <typename Presenter, typename View>
class UITypedFrontendContinuation final : public UIFrontendContinuation
{
	public:
		using SettleView = void (*)(View &);

		template <typename... ViewArgs>
		UITypedFrontendContinuation(
			Presenter & presenter,
			bool native_blocking_adapter,
			SettleView settle_view,
			ViewArgs &&... view_args) :
			m_Presenter(&presenter),
			m_View(std::make_unique<View>(presenter, std::forward<ViewArgs>(view_args)...)),
			m_NativeBlockingAdapter(native_blocking_adapter)
		{
			Begin(settle_view);
		}

		template <typename... ViewArgs>
		UITypedFrontendContinuation(
			std::unique_ptr<Presenter> presenter,
			bool native_blocking_adapter,
			SettleView settle_view,
			ViewArgs &&... view_args) :
			m_OwnedPresenter(std::move(presenter)),
			m_Presenter(m_OwnedPresenter.get()),
			m_View(std::make_unique<View>(*m_Presenter, std::forward<ViewArgs>(view_args)...)),
			m_NativeBlockingAdapter(native_blocking_adapter)
		{
			Begin(settle_view);
		}

		~UITypedFrontendContinuation(void) override
		{
			if (!m_Finished) Finish();
		}

		OpenTS_UIModal::UIModalStepResult Step(void) noexcept override
		{
			if (m_Finished) return(m_LastStep);
			if (!m_Begun) return(OpenTS_UIModal::UIModalStepResult::Complete);
			m_LastStep = UI_Modal_Session_Step(m_Modal);
			return(m_LastStep);
		}

		UIResult const & Result(void) const noexcept override
		{
			return(m_Begun ? m_Modal.Result : m_Result);
		}

		bool Finish(void) noexcept override
		{
			if (m_Finished) return(true);
			bool success = true;
			if (m_Begun) success = FinishModalOnly();
			if (m_Prepared && m_View != nullptr) m_View->Close();
			m_View.reset();
			m_Presenter = nullptr;
			m_Finished = true;
			return(success);
		}

		Presenter * PresenterPtr(void) const noexcept { return(m_Presenter); }
		View * ViewPtr(void) const noexcept { return(m_View.get()); }
		OpenTS_UIModal::UIModalSession & Modal(void) noexcept { return(m_Modal); }

		bool FinishModalOnly(void) noexcept
		{
			if (!m_Begun) return(true);
			bool const success = UI_Modal_Session_Finish(m_Modal);
			m_Result = m_Modal.Result;
			m_Begun = false;
			m_Modal = OpenTS_UIModal::UIModalSession{};
			return(success);
		}

		bool BeginModalAgain(bool native_blocking_adapter) noexcept
		{
			if (m_Finished || !m_Prepared || m_View == nullptr || m_Presenter == nullptr || m_Begun) return(false);
			m_NativeBlockingAdapter = native_blocking_adapter;
			m_Modal = OpenTS_UIModal::UIModalSession{};
			m_Result = UIResult{};
			m_Begun = UI_Modal_Session_Begin_Mode(m_Modal, *m_Presenter, *m_View, m_NativeBlockingAdapter);
			return(m_Begun);
		}

	private:
		void Begin(SettleView settle_view)
		{
			if (m_Presenter == nullptr || m_View == nullptr || !m_View->Prepare(true)) {
				m_Result.Outcome = UIResult::OUTCOME_FAILED_TO_OPEN;
				m_LastStep = OpenTS_UIModal::UIModalStepResult::Complete;
				return;
			}
			m_Prepared = true;
			if (settle_view != nullptr) settle_view(*m_View);
			m_Begun = UI_Modal_Session_Begin_Mode(m_Modal, *m_Presenter, *m_View, m_NativeBlockingAdapter);
			if (!m_Begun) {
				m_Result.Outcome = UIResult::OUTCOME_FAILED_TO_OPEN;
				m_LastStep = OpenTS_UIModal::UIModalStepResult::Complete;
			}
		}

		std::unique_ptr<Presenter> m_OwnedPresenter;
		Presenter * m_Presenter = nullptr;
		std::unique_ptr<View> m_View;
		OpenTS_UIModal::UIModalSession m_Modal;
		UIResult m_Result;
		OpenTS_UIModal::UIModalStepResult m_LastStep = OpenTS_UIModal::UIModalStepResult::Active;
		bool m_NativeBlockingAdapter = false;
		bool m_Prepared = false;
		bool m_Begun = false;
		bool m_Finished = false;
};


// Native legacy wrappers may drive a continuation synchronously; the wait stays outside
// Step. Cooperative callers retain and step the continuation directly.
UIResult UI_Run_Frontend_Continuation(UIFrontendContinuation & continuation);
