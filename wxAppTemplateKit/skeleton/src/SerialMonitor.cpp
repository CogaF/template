/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file SerialMonitor.cpp
 * \brief Implementation of SerialMonitor.h.
 */

#include "SerialMonitor.h"
#include "Log.h"
#include "TimeUtils.h"

#include <algorithm>
#include <chrono>

/*! \brief Events waiting for the GUI thread; shared with the CallAfter lambdas. */
struct SerialMonitor::Mailbox {
	std::mutex mutex;           /*!< guards pending */
	std::vector<Event> pending; /*!< not delivered yet */
	wxWeakRef<wxEvtHandler> target; /*!< the window that delivers them */
	Sink sink;                  /*!< called on the GUI thread */
};

SerialMonitor::SerialMonitor(SerialLink& link, wxEvtHandler* target, Sink sink)
	: link_(link), mailbox_(std::make_shared<Mailbox>()) {
	mailbox_->target = target;
	mailbox_->sink = std::move(sink);
}

SerialMonitor::~SerialMonitor() { stop(); }

void SerialMonitor::start(int gapMs) {
	if (thread_.joinable()) return;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		rulesChanged_ = true; // counters and matching start again
	}
	thread_ = std::jthread([this, gap = (std::max)(gapMs, 1)](std::stop_token st) { run(st, gap); });
}

void SerialMonitor::stop() {
	if (!thread_.joinable()) return;
	thread_.request_stop();
	cv_.notify_all();
	thread_.join();
	std::lock_guard<std::mutex> lock(mutex_);
	queue_.clear();
}

bool SerialMonitor::send(std::vector<uint8_t> bytes, Kind kind, std::string source) {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!thread_.joinable() || queue_.size() >= kMaxQueued) return false;
		queue_.push_back({ std::move(bytes), kind, std::move(source), 0 });
	}
	cv_.notify_one();
	return true;
}

void SerialMonitor::setAutoReplies(const std::vector<SerialData::AutoReplyRule>& rules) {
	std::lock_guard<std::mutex> lock(mutex_);
	rules_ = rules;
	rulesChanged_ = true;
}

void SerialMonitor::post(Event event) {
	std::shared_ptr<Mailbox> box = mailbox_;
	bool first = false;
	{
		std::lock_guard<std::mutex> lock(box->mutex);
		first = box->pending.empty();
		box->pending.push_back(std::move(event));
	}
	if (!first) return; // a delivery is already on its way and will take this one too
	wxEvtHandler* target = box->target.get();
	if (!target) return;
	target->CallAfter([box] {
		std::vector<Event> events;
		{
			std::lock_guard<std::mutex> lock(box->mutex);
			events.swap(box->pending);
		}
		if (!events.empty() && box->sink) box->sink(events);
	});
}

void SerialMonitor::write(const Outgoing& out) {
	Event e;
	e.epochMs = Utils::Time::nowEpochMs();
	e.result = link_.write(out.bytes);
	e.kind = e.result == SerialLink::Result::Ok ? out.kind : Kind::Error;
	e.bytes = out.bytes;
	e.source = out.source;
	post(std::move(e));
}

void SerialMonitor::run(std::stop_token stop, int gapMs) {
	Log::debug("SerialMonitor: listening.");
	using SerialData::AutoReplyRule;
	std::vector<AutoReplyRule> rules;                 // the enabled rules with a valid pattern
	std::vector<SerialData::MessageGenerator> replies; // one per rule: its counters
	SerialData::PatternMatcher matcher;
	std::vector<Outgoing> delayed;                    // auto replies waiting for their delay

	std::vector<uint8_t> rx;       // the received message being collected
	uint64_t rxEpochMs = 0;        // when its first byte arrived
	uint64_t lastRxMs = 0;         // monotonic time of its last byte
	bool readFailing = false;      // the port stopped working (e.g. USB adapter unplugged)
	uint64_t retryAtMs = 0;

	auto flushRx = [&] {
		if (rx.empty()) return;
		Event e;
		e.kind = Kind::Received;
		e.epochMs = rxEpochMs;
		e.bytes.swap(rx);
		post(std::move(e));
	};

	std::vector<uint8_t> chunk;
	while (!stop.stop_requested()) {
		std::deque<Outgoing> toWrite;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			toWrite.swap(queue_);
			if (rulesChanged_) {
				rulesChanged_ = false;
				rules.clear();
				replies.clear();
				delayed.clear();
				std::vector<std::vector<uint8_t>> patterns;
				const uint64_t now = Utils::Time::nowMonotonicMs();
				for (const AutoReplyRule& r : rules_) {
					if (!r.enabled) continue;
					SerialData::ParseResult p = SerialData::parse(r.patternText, r.patternFormat);
					if (!p.ok() || p.bytes.empty()) continue;
					rules.push_back(r);
					replies.emplace_back(r.reply);
					replies.back().restart(now);
					patterns.push_back(std::move(p.bytes));
				}
				matcher.setPatterns(std::move(patterns));
			}
		}
		bool busy = !toWrite.empty();
		for (const Outgoing& out : toWrite) {
			flushRx(); // keep the console in the order things happened on the line
			write(out);
		}

		uint64_t now = Utils::Time::nowMonotonicMs();
		for (auto it = delayed.begin(); it != delayed.end();) {
			if (it->dueMs > now) { ++it; continue; }
			flushRx();
			write(*it);
			it = delayed.erase(it);
			busy = true;
		}

		if (!readFailing || now >= retryAtMs) {
			chunk.clear();
			bool failed = false;
			link_.readAvailable(chunk, &failed);
			if (failed != readFailing) {
				readFailing = failed;
				if (failed) Log::warning("SerialMonitor: the port cannot be read; retrying every second.");
				else Log::info("SerialMonitor: the port can be read again.");
			}
			if (failed) retryAtMs = now + 1000;
		}
		now = Utils::Time::nowMonotonicMs();
		if (!chunk.empty()) {
			busy = true;
			if (rx.empty()) rxEpochMs = Utils::Time::nowEpochMs();
			rx.insert(rx.end(), chunk.begin(), chunk.end());
			lastRxMs = now;
			for (size_t ruleIndex : matcher.feed(chunk.data(), chunk.size())) {
				SerialData::BuildResult reply = replies[ruleIndex].next(now);
				if (!reply.ok()) {
					Event e;
					e.kind = Kind::Error;
					e.epochMs = Utils::Time::nowEpochMs();
					e.source = rules[ruleIndex].name;
					post(std::move(e));
					continue;
				}
				Outgoing out{ std::move(reply.bytes), Kind::AutoReply, rules[ruleIndex].name, now + static_cast<uint64_t>((std::max)(rules[ruleIndex].delayMs, 0)) };
				if (rules[ruleIndex].delayMs > 0) {
					delayed.push_back(std::move(out));
				}
				else {
					flushRx(); // the pattern first, then the reply
					write(out);
				}
			}
			chunk.clear();
		}
		if (!rx.empty() && (now - lastRxMs >= static_cast<uint64_t>(gapMs) || rx.size() >= kMaxMessageBytes)) flushRx();

		if (!busy) {
			std::unique_lock<std::mutex> lock(mutex_);
			cv_.wait_for(lock, stop, std::chrono::milliseconds(1), [this] { return !queue_.empty(); });
		}
	}
	flushRx();
	Log::debug("SerialMonitor: stopped.");
}
