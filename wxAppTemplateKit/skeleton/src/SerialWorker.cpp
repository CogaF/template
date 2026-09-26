/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file SerialWorker.cpp
 * \brief Implementation of SerialWorker.h.
 */

#include "SerialWorker.h"
#include "Log.h"

SerialWorker::SerialWorker(SerialLink& link, wxEvtHandler* resultTarget) : link_(link), target_(resultTarget) {}

SerialWorker::~SerialWorker() { stop(); }

void SerialWorker::start() {
	if (thread_.joinable()) return;
	link_.clearAbort(); // a previous stop() aborted the link
	thread_ = std::jthread([this](std::stop_token st) { run(st); });
}

void SerialWorker::stop() {
	if (!thread_.joinable()) return;
	thread_.request_stop();
	link_.requestAbort(); // a transaction in progress returns now instead of at its timeout
	cv_.notify_all();
	thread_.join();
	std::lock_guard<std::mutex> lock(mutex_);
	queue_.clear();
}

bool SerialWorker::submit(Job job) {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!thread_.joinable() || queue_.size() >= kMaxQueuedJobs) return false;
		queue_.push_back(std::move(job));
	}
	cv_.notify_one();
	return true;
}

size_t SerialWorker::pending() const {
	std::lock_guard<std::mutex> lock(mutex_);
	return queue_.size();
}

void SerialWorker::run(std::stop_token stop) {
	Log::debug("SerialWorker: started.");
	while (!stop.stop_requested()) {
		Job job;
		{
			std::unique_lock<std::mutex> lock(mutex_);
			if (!cv_.wait(lock, stop, [this] { return !queue_.empty(); })) break; // stop requested
			job = std::move(queue_.front());
			queue_.pop_front();
		}
		std::vector<uint8_t> reply;
		const SerialLink::Result result = link_.transact(job.request, reply, job.timeoutMs, job.complete);
		if (!job.done) continue;
		wxEvtHandler* target = target_.get();
		if (!target) continue; // the window is gone
		target->CallAfter([done = std::move(job.done), result, reply = std::move(reply)] { done(result, reply); });
	}
	Log::debug("SerialWorker: stopped.");
}
