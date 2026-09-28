/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "uimodalsession.h"
#include "uiscreen.h"

#include <memory>
#include <utility>

class UIRmlViewClass;

bool UI_Modal_Session_Begin_Mode(
	OpenTS_UIModal::UIModalSession & session,
	UIPresenterClass & presenter,
	UIRmlViewClass & view,
	bool native_blocking_adapter);
OpenTS_UIModal::UIModalStepResult UI_Modal_Session_Step(
	OpenTS_UIModal::UIModalSession & session);
bool UI_Modal_Session_Finish(OpenTS_UIModal::UIModalSession & session);


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


// A presenter with a native child exposes the child factory and the native result
// application here. The shell owns neither the child nor the meaning of its result.
class UIChildContinuationProvider
{
	public:
		virtual ~UIChildContinuationProvider(void) = default;
		virtual std::unique_ptr<UIFrontendContinuation> Start_Child_Continuation(bool native_blocking_adapter) = 0;
		virtual void Complete_Child_Continuation(UIResult const & result) = 0;
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
		using FinishView = void (*)(View &);

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
			if (m_Prepared && m_View != nullptr) {
				if (m_FinishView != nullptr) m_FinishView(*m_View);
				m_View->Close();
			}
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

		void SetFinishView(FinishView finish_view) noexcept { m_FinishView = finish_view; }

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
		FinishView m_FinishView = nullptr;
};


// Owns the parent modal lifetime and the one native child requested by its presenter.
// The parent document is hidden and its modal lease is finished before the child starts;
// after the child is closed, the same typed presenter/view are shown and resumed.
template <typename Presenter, typename View>
class UINestedTypedFrontendContinuation final : public UIFrontendContinuation
{
	public:
		using ParentType = UITypedFrontendContinuation<Presenter, View>;
		using SettleView = typename ParentType::SettleView;
		using FinishView = typename ParentType::FinishView;

		template <typename... ViewArgs>
		UINestedTypedFrontendContinuation(
			Presenter & presenter,
			bool native_blocking_adapter,
			SettleView settle_view,
			ViewArgs &&... view_args) :
			m_Parent(std::make_unique<ParentType>(presenter, native_blocking_adapter, settle_view,
				std::forward<ViewArgs>(view_args)...)),
			m_NativeBlockingAdapter(native_blocking_adapter)
		{
		}

		~UINestedTypedFrontendContinuation(void) override
		{
			if (!m_Finished) Finish();
		}

		OpenTS_UIModal::UIModalStepResult Step(void) noexcept override
		{
			using OpenTS_UIModal::UIModalStepResult;
			if (m_Finished) return(m_LastStep);
			if (m_Faulted) return(UIModalStepResult::Fault);

			if (m_Child != nullptr) {
				UIModalStepResult const child_step = m_Child->Step();
				if (child_step == UIModalStepResult::Active
					|| child_step == UIModalStepResult::Waiting
					|| child_step == UIModalStepResult::Suspended) {
					return(child_step == UIModalStepResult::Suspended ? UIModalStepResult::Waiting : child_step);
				}
				if (child_step == UIModalStepResult::Fault) return(Fault());

				UIResult const child_result = m_Child->Result();
				if (!m_Child->Finish()) return(Fault());
				m_Child.reset();

				UIChildContinuationProvider * const provider = Provider();
				if (provider == nullptr) return(Fault());
				provider->Complete_Child_Continuation(child_result);
				if (m_Parent == nullptr || m_Parent->ViewPtr() == nullptr
					|| m_Parent->PresenterPtr() == nullptr) return(Fault());
				m_Parent->ViewPtr()->Show();
				m_Parent->ViewPtr()->Sync();
				if (!m_Parent->BeginModalAgain(m_NativeBlockingAdapter)) return(Fault());
				return(UIModalStepResult::Waiting);
			}

			if (m_Parent == nullptr) return(Fault());
			UIModalStepResult const parent_step = m_Parent->Step();
			if (parent_step != UIModalStepResult::Suspended) {
				m_LastStep = parent_step;
				return(parent_step);
			}

			UIChildContinuationProvider * const provider = Provider();
			if (provider == nullptr || m_Parent->ViewPtr() == nullptr) return(Fault());
			m_Parent->ViewPtr()->Hide();
			if (!m_Parent->FinishModalOnly()) return(Fault());
			m_Child = provider->Start_Child_Continuation(m_NativeBlockingAdapter);
			if (m_Child == nullptr) {
				UIResult failed;
				failed.Outcome = UIResult::OUTCOME_FAILED_TO_OPEN;
				provider->Complete_Child_Continuation(failed);
				m_Parent->ViewPtr()->Show();
				m_Parent->ViewPtr()->Sync();
				if (!m_Parent->BeginModalAgain(m_NativeBlockingAdapter)) return(Fault());
			}
			return(UIModalStepResult::Waiting);
		}

		UIResult const & Result(void) const noexcept override
		{
			return(m_Faulted || m_Finished ? m_Result : m_Parent->Result());
		}

		void SetFinishView(FinishView finish_view) noexcept
		{
			if (m_Parent != nullptr) m_Parent->SetFinishView(finish_view);
		}

		bool Finish(void) noexcept override
		{
			if (m_Finished) return(true);
			bool success = true;
			if (m_Child != nullptr) {
				success = m_Child->Finish() && success;
				m_Child.reset();
			}
			if (m_Parent != nullptr) {
				if (!m_Faulted) m_Result = m_Parent->Result();
				success = m_Parent->Finish() && success;
			}
			m_Parent.reset();
			m_Finished = true;
			return(success);
		}

	private:
		UIChildContinuationProvider * Provider(void) const noexcept
		{
			return(m_Parent != nullptr && m_Parent->PresenterPtr() != nullptr
				? static_cast<UIChildContinuationProvider *>(m_Parent->PresenterPtr())
				: nullptr);
		}

		OpenTS_UIModal::UIModalStepResult Fault(void) noexcept
		{
			m_Faulted = true;
			m_Result.Outcome = UIResult::OUTCOME_SESSION_ENDED;
			m_LastStep = OpenTS_UIModal::UIModalStepResult::Fault;
			return(m_LastStep);
		}

		std::unique_ptr<ParentType> m_Parent;
		std::unique_ptr<UIFrontendContinuation> m_Child;
		UIResult m_Result;
		OpenTS_UIModal::UIModalStepResult m_LastStep = OpenTS_UIModal::UIModalStepResult::Active;
		bool m_NativeBlockingAdapter = false;
		bool m_Faulted = false;
		bool m_Finished = false;
};


// Native legacy wrappers may drive a continuation synchronously; the wait stays outside
// Step. Cooperative callers retain and step the continuation directly.
UIResult UI_Run_Frontend_Continuation(UIFrontendContinuation & continuation);
