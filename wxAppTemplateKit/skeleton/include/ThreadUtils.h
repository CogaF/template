/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

#include <wx/app.h>
#include <wx/event.h>
#include <wx/timer.h>
#include <wx/weakref.h>

/*!
 * \file ThreadUtils.h
 * \brief Helpers for background threads and the GUI thread.
 *
 * Rules the template follows (and that keep a wxWidgets application stable):
 *  - Only the GUI thread touches windows. A worker hands results back with
 *    wxEvtHandler::CallAfter() (thread-safe; the callback runs on the GUI thread).
 *  - Every worker has a stop request it checks often, and is joined before the objects it uses are
 *    destroyed (std::jthread joins in its destructor).
 *  - Blocking waits have a timeout (std::timed_mutex, condition variables with wait_for), so a stuck
 *    device can never freeze shutdown.
 */
namespace ThreadUtils {

	/*!
	 * \brief Sets a flag to true for the lifetime of a scope and back to false when it ends, even
	 * through an exception - lets other threads see whether a worker loop is still alive.
	 */
	class AliveGuard {
	public:
		/*! \brief Sets flag to true. */
		explicit AliveGuard(std::atomic<bool>& flag) : flag_(flag) { flag_.store(true, std::memory_order_release); }
		/*! \brief Sets the flag back to false. */
		~AliveGuard() { flag_.store(false, std::memory_order_release); }
		AliveGuard(const AliveGuard&) = delete;
		AliveGuard& operator=(const AliveGuard&) = delete;
	private:
		std::atomic<bool>& flag_;
	};

	/*!
	 * \brief Runs fn on the GUI thread after delayMs, without blocking anything. If owner is
	 * destroyed before then, fn is skipped (so fn may safely use the owner). Call from the GUI thread.
	 */
	inline void CallLater(wxEvtHandler* owner, int delayMs, std::function<void()> fn) {
		/*! \brief One-shot timer that deletes itself after firing. */
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

	/*!
	 * \brief A background thread that runs posted tasks one after the other, in order.
	 *
	 *     ThreadUtils::WorkerQueue exporter;
	 *     exporter.start();
	 *     exporter.post([file] { writeBigExport(file); });                 // runs on the worker
	 *     exporter.post([this] { ... }, [this] { statusText->SetLabel("Export done"); }); // then on the GUI
	 *
	 * stop() (also called by the destructor) lets the running task finish, drops the ones not started
	 * and joins the thread. Tasks must not block forever - check stopRequested() in long loops.
	 */
	class WorkerQueue {
	public:
		WorkerQueue() = default;
		~WorkerQueue() { stop(); }
		WorkerQueue(const WorkerQueue&) = delete;
		WorkerQueue& operator=(const WorkerQueue&) = delete;

		/*! \brief Starts the worker thread (does nothing if it runs already). */
		void start() {
			if (thread_.joinable()) return;
			thread_ = std::jthread([this](std::stop_token st) { run(st); });
		}

		/*! \brief Finishes the running task, discards the others and joins the thread. */
		void stop() {
			if (!thread_.joinable()) return;
			thread_.request_stop();
			cv_.notify_all();
			thread_.join();
			std::lock_guard<std::mutex> lock(mutex_);
			tasks_.clear();
		}

		/*!
		 * \brief Queues task to run on the worker; then, if given, onDone runs on the GUI thread
		 * (through wxTheApp->CallAfter). \return false if the worker is not running.
		 */
		bool post(std::function<void()> task, std::function<void()> onDone = {}) {
			{
				std::lock_guard<std::mutex> lock(mutex_);
				if (!thread_.joinable()) return false;
				tasks_.push_back({ std::move(task), std::move(onDone) });
			}
			cv_.notify_one();
			return true;
		}

		/*! \brief Tasks waiting (not counting the one running). */
		size_t pending() const {
			std::lock_guard<std::mutex> lock(mutex_);
			return tasks_.size();
		}

		/*! \brief true once stop() was called - long tasks should check it and return early. */
		bool stopRequested() const { return thread_.get_stop_token().stop_requested(); }

	private:
		/*! \brief One queued task and its optional GUI-thread continuation. */
		struct Task { std::function<void()> work; std::function<void()> onDone; };

		void run(std::stop_token stop) {
			while (!stop.stop_requested()) {
				Task task;
				{
					std::unique_lock<std::mutex> lock(mutex_);
					if (!cv_.wait(lock, stop, [this] { return !tasks_.empty(); })) break;
					task = std::move(tasks_.front());
					tasks_.pop_front();
				}
				if (task.work) task.work();
				if (task.onDone && wxTheApp) wxTheApp->CallAfter(std::move(task.onDone));
			}
		}

		mutable std::mutex mutex_;
		std::condition_variable_any cv_;
		std::deque<Task> tasks_;
		std::jthread thread_;
	};
}
