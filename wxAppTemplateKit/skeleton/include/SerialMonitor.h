/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <wx/event.h>
#include <wx/weakref.h>

#include "SerialLink.h"
#include "SerialMessage.h"

/*!
 * \file SerialMonitor.h
 * \brief A terminal-style serial port: always listening, sending on request, answering automatically.
 *
 * One background thread owns the traffic of an open SerialLink:
 *  - it reads continuously; bytes that arrive close together (less than the gap apart, by default
 *    the port's inter-byte timeout) are reported as one received message;
 *  - it writes the messages given to send() (the GUI never waits for the port);
 *  - it answers the auto reply rules itself, as soon as their pattern arrives (a pattern split over
 *    several reads is still recognised), after the rule's delay; each rule has its own counters.
 * Everything that crosses the line is reported ON THE GUI THREAD, in order, to the Sink - several
 * events at a time, so a fast device cannot flood the GUI with calls.
 *
 * Use it OR SerialWorker's request/reply transactions on a link, not both: a transaction flushes the
 * input first and would take bytes the monitor has not reported yet.
 */
/*!
 * \brief Always-listening serial port with send queue and auto replies (see the file comment).
 */
class SerialMonitor {
public:
	/*! \brief What an event is about. */
	enum class Kind {
		Received,  /*!< bytes received */
		Sent,      /*!< a message given to send() was written */
		AutoReply, /*!< an auto reply was written (source = the rule's name) */
		Periodic,  /*!< a periodic message was written (source = its name, given to send()) */
		Error      /*!< a write failed (result says why) or an auto reply could not be built */
	};
	/*! \brief One thing that crossed the line. */
	struct Event {
		Kind kind = Kind::Received;                           /*!< what */
		std::vector<uint8_t> bytes;                           /*!< the bytes */
		uint64_t epochMs = 0;                                 /*!< when (ms since 1970, for display) */
		std::string source;                                   /*!< rule / periodic message name, if any */
		SerialLink::Result result = SerialLink::Result::Ok;   /*!< for writes */
	};
	/*! \brief Receives the events on the GUI thread, oldest first. */
	using Sink = std::function<void(std::vector<Event>& events)>;

	/*!
	 * \param link   the port (must outlive the monitor).
	 * \param target window whose CallAfter() delivers the events; nothing is delivered once it is gone.
	 * \param sink   called with the events on the GUI thread.
	 */
	SerialMonitor(SerialLink& link, wxEvtHandler* target, Sink sink);
	/*! \brief Stops the thread. */
	~SerialMonitor();
	SerialMonitor(const SerialMonitor&) = delete;
	SerialMonitor& operator=(const SerialMonitor&) = delete;

	/*!
	 * \brief Starts listening (does nothing if running).
	 * \param gapMs a received message ends when no byte arrives for this long (at least 1 ms).
	 */
	void start(int gapMs);
	/*! \brief Stops listening; queued messages and pending auto replies are discarded. */
	void stop();
	/*! \brief true while listening. */
	bool isRunning() const { return thread_.joinable(); }

	/*!
	 * \brief Queues bytes to write. kind is Sent or Periodic (source: its name, for the console).
	 * \return false if not running or too many messages are waiting.
	 */
	bool send(std::vector<uint8_t> bytes, Kind kind = Kind::Sent, std::string source = {});
	/*!
	 * \brief Replaces the auto reply rules (thread-safe; disabled rules are ignored). Their counters
	 * start again. Takes effect for the bytes read from now on.
	 */
	void setAutoReplies(const std::vector<SerialData::AutoReplyRule>& rules);

	/*! \brief send() refuses new messages beyond this many waiting. */
	static constexpr size_t kMaxQueued = 1000;
	/*! \brief A received message is reported at the latest when it reaches this size. */
	static constexpr size_t kMaxMessageBytes = 4096;

private:
	struct Outgoing { std::vector<uint8_t> bytes; Kind kind; std::string source; uint64_t dueMs; };
	struct Mailbox;

	void run(std::stop_token stop, int gapMs);
	void post(Event event);
	void write(const Outgoing& out);

	SerialLink& link_;
	std::shared_ptr<Mailbox> mailbox_; // shared with the CallAfter lambdas, which may run after stop()

	std::mutex mutex_;                 // guards queue_, rules_, rulesChanged_
	std::condition_variable_any cv_;
	std::deque<Outgoing> queue_;
	std::vector<SerialData::AutoReplyRule> rules_;
	bool rulesChanged_ = true;

	std::jthread thread_;
};
