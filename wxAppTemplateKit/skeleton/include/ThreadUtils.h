// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <atomic>
#include <functional>
#include <wx/app.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/weakref.h>

/*!
 * \file ThreadUtils.h
 * \brief Small helpers for background threads and the GUI thread.
 *
 * Rules the template follows (and that keep a wxWidgets app stable):
 *  - Only the GUI thread touches windows. A worker hands results back with
 *    wxEvtHandler::CallAfter() (thread-safe, the callback runs on the GUI thread).
 *  - Every worker has a stop request it checks often, and is joined before the objects it uses are
 *    destroyed (std::jthread joins in its destructor).
 *  - Blocking waits have a timeout (std::timed_mutex, condition variables with wait_for), so a stuck
 *    device can never freeze shutdown.
 */
namespace ThreadUtils {

	/*!
	 * \brief Sets a flag true for the lifetime of a scope and false when it ends, even through an
	 * exception - lets other threads see whether a worker loop is still alive.
	 */
	class AliveGuard {
	public:
		explicit AliveGuard(std::atomic<bool>& flag) : flag_(flag) { flag_.store(true, std::memory_order_release); }
		~AliveGuard() { flag_.store(false, std::memory_order_release); }
		AliveGuard(const AliveGuard&) = delete;
		AliveGuard& operator=(const AliveGuard&) = delete;
	private:
		std::atomic<bool>& flag_;
	};

	/*!
	 * \brief Runs fn on the GUI thread after delayMs without blocking anything. If `owner` is
	 * destroyed before then, fn is skipped (so fn may safely use the owner). Call from the GUI thread.
	 */
	inline void CallLater(wxEvtHandler* owner, int delayMs, std::function<void()> fn) {
		class OneShot : public wxTimer {
		public:
			OneShot(wxEvtHandler* owner, std::function<void()> fn) : owner_(owner), fn_(std::move(fn)) {}
			void Notify() override {
				std::function<void()> fn = std::move(fn_);
				const bool ownerAlive = owner_.get() != nullptr;
				wxTheApp->CallAfter([this] { delete this; }); // never delete a timer inside its own Notify()
				if (ownerAlive && fn) fn();
			}
		private:
			wxWeakRef<wxEvtHandler> owner_;
			std::function<void()> fn_;
		};
		(new OneShot(owner, std::move(fn)))->StartOnce(delayMs);
	}
}
