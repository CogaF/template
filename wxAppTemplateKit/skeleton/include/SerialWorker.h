// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include <wx/event.h>
#include <wx/weakref.h>

#include "SerialLink.h"

/*!
 * \file SerialWorker.h
 * \brief A background thread that runs serial transactions one after the other.
 *
 * The GUI (or any thread) submit()s jobs; the worker runs them in order on its own thread through a
 * SerialLink, and hands each result back ON THE GUI THREAD (via CallAfter on the target handler),
 * so the callback may update windows directly. If the target window is gone, results are dropped.
 * stop() (also called by the destructor) aborts the transaction in progress, discards the queue and
 * joins the thread - it never waits for a device timeout.
 */
class SerialWorker {
public:
	using Done = std::function<void(SerialLink::Result result, const std::vector<uint8_t>& reply)>;
	struct Job {
		std::vector<uint8_t> request;
		int timeoutMs = 500;
		SerialLink::FrameComplete complete; //!< protocol framing, see SerialLink::FrameComplete
		Done done;                          //!< runs on the GUI thread; may be empty
	};

	SerialWorker(SerialLink& link, wxEvtHandler* resultTarget);
	~SerialWorker();
	SerialWorker(const SerialWorker&) = delete;
	SerialWorker& operator=(const SerialWorker&) = delete;

	void start();
	void stop();
	bool isRunning() const { return thread_.joinable(); }

	//! Queues a job; false if the worker is not running or the queue is full.
	bool submit(Job job);
	size_t pending() const;
	static constexpr size_t kMaxQueuedJobs = 1000;

private:
	void run(std::stop_token stop);

	SerialLink& link_;
	wxWeakRef<wxEvtHandler> target_;
	mutable std::mutex mutex_;
	std::condition_variable_any cv_;
	std::deque<Job> queue_;
	std::jthread thread_;
};
