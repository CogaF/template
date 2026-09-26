/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "SerialConfig.h"

/*!
 * \file SerialLink.h
 * \brief One open serial port, safe to share between threads.
 *
 * transact() is the request/reply primitive for a master polling devices on a bus (RS-232/RS-485):
 *  1. takes the port lock with a timeout (never blocks forever behind a stuck thread);
 *  2. flushes the input TWICE with a short pause (busSettleMs) in between - on a shared RS-485 bus
 *     the previous device can still be sending its last bytes, which would otherwise be read as the
 *     start of the next reply;
 *  3. writes the request;
 *  4. reads until the reply is COMPLETE - decided by the caller's FrameComplete function (the
 *     protocol knows its own framing: a length byte, an end marker, ...) - or, without one, until
 *     the line is quiet for the inter-byte timeout. A reply split over several USB packets is
 *     therefore never cut short;
 *  5. can be aborted from another thread (requestAbort()) so shutdown never waits for a timeout.
 * readAvailable() is the passive side (sniffing / devices that talk unprompted).
 */
/*!
 * \brief One open serial port shared safely between threads (see the file comment).
 */
class SerialLink {
public:
	/*!
	 * \brief Given the bytes received so far, returns the total length of the frame once it can be
	 * told (e.g. from a length field), or 0 while more bytes are needed.
	 */
	using FrameComplete = std::function<size_t(const std::vector<uint8_t>& received)>;

	/*! \brief Outcome of transact() and write(). */
	enum class Result {
		Ok,          /*!< complete reply received (or written) */
		NotOpen,     /*!< the port is not open */
		Busy,        /*!< another thread held the port longer than kLockTimeoutMs */
		WriteFailed, /*!< not all bytes could be written */
		Timeout,     /*!< no (complete) reply within the timeout */
		Aborted,     /*!< requestAbort() was called */
		Error        /*!< the serial library threw (details in the log) */
	};
	/*! \brief "Ok", "Timeout", ... - for logs. */
	static const char* resultName(Result r);

	SerialLink() = default;
	/*! \brief Closes the port. */
	~SerialLink();
	SerialLink(const SerialLink&) = delete;
	SerialLink& operator=(const SerialLink&) = delete;

	/*! \brief Opens (or re-opens with new settings). On failure returns false and fills error. */
	bool open(const SerialConfig& config, std::string* error = nullptr);
	/*! \brief Closes the port (a transaction in progress is aborted first). */
	void close();
	/*! \brief true while the port is open. */
	bool isOpen() const;
	/*! \brief The settings the port was opened with. */
	SerialConfig config() const;

	/*!
	 * \brief Sends tx and collects the reply into rx.
	 * \param tx         the request bytes.
	 * \param rx         receives the reply (cleared first; cut to the frame length when complete is given).
	 * \param timeoutMs  total time allowed for the reply to start AND complete.
	 * \param complete   protocol framing (see FrameComplete); empty = "until the line goes quiet".
	 * \param busSettleMs pause between the two input flushes (see the class comment); 0 = one flush.
	 * \return Result::Ok with the reply in rx, or why not.
	 */
	Result transact(const std::vector<uint8_t>& tx, std::vector<uint8_t>& rx, int timeoutMs,
		const FrameComplete& complete = {}, int busSettleMs = 2);

	/*! \brief Writes without waiting for a reply. */
	Result write(const std::vector<uint8_t>& tx);
	/*! \brief Appends whatever bytes are waiting (non-blocking). Returns how many. */
	size_t readAvailable(std::vector<uint8_t>& out);

	/*! \brief Makes a transact() in progress on another thread return Aborted soon (e.g. at shutdown). */
	void requestAbort() { abort_.store(true); }
	/*! \brief Allows transactions again after requestAbort() (open()/close() do it too). */
	void clearAbort() { abort_.store(false); }

	/*! \brief How long transact()/write() wait for another thread's transaction before returning Busy. */
	static constexpr int kLockTimeoutMs = 2000;

private:
	mutable std::timed_mutex mutex_;
	std::unique_ptr<serial::Serial> port_;
	SerialConfig config_;
	std::atomic<bool> abort_{ false };
};
