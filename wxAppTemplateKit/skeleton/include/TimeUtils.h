/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

/*!
 * \file TimeUtils.h
 * \brief Clocks, timestamps and durations.
 *
 * Two clocks, for two different jobs:
 *  - monotonic (steady_clock): measuring intervals and timeouts - never jumps when the PC clock is
 *    changed or daylight saving starts. Use nowMonotonicMs() / Stopwatch.
 *  - wall clock (system_clock): what the user sees and what goes into files. Use the *String()
 *    functions; they use local time and are thread-safe (localtime_s / localtime_r).
 *
 * A timestamp can also be packed into a uint64_t whose decimal digits read as the date:
 * 20260926140301123 = 2026-09-26 14:03:01.123 (packTimestamp()/unpackTimestamp()) - compact, sortable,
 * readable in a hex dump or a database column.
 */
namespace Utils::Time {

	/*! \brief Milliseconds of a monotonic clock (arbitrary origin) - for intervals and timeouts only. */
	uint64_t nowMonotonicMs();
	/*! \brief Microseconds of a monotonic clock (arbitrary origin). */
	uint64_t nowMonotonicUs();
	/*! \brief Milliseconds since 1970-01-01 00:00:00 UTC (Unix epoch) - for storing points in time. */
	uint64_t nowEpochMs();

	/*! \brief Local date and time of an epoch-ms instant, e.g. "2026-09-26 14:03:01.123". */
	std::string toString(uint64_t epochMs, bool withMilliseconds = true);
	/*! \brief Current local date and time, e.g. "2026-09-26 14:03:01.123". */
	std::string nowString(bool withMilliseconds = true);
	/*! \brief Current local time of day, e.g. "14:03:01.123". */
	std::string timeOfDayString(bool withMilliseconds = true);
	/*! \brief Current local date, e.g. "2026-09-26". */
	std::string dateString();
	/*! \brief Current year, e.g. "2026" (copyright lines). */
	std::string yearString();
	/*!
	 * \brief Current local time in a form that is safe in file names and sorts correctly,
	 * e.g. "2026-09-26_14-03-01".
	 */
	std::string fileNameStamp();

	/*!
	 * \brief Formats a duration: "1d 02:03:04.005" (days only when > 0), or "02:03:04" without
	 * milliseconds. Negative values are shown with a leading '-'.
	 */
	std::string durationString(int64_t milliseconds, bool withMilliseconds = true);

	/*!
	 * \brief Packs a local date/time into the digits of a uint64_t: YYYYMMDDhhmmssmmm (17 digits),
	 * e.g. 20260926140301123. Sorting the numbers sorts the dates.
	 */
	uint64_t packTimestamp(uint64_t epochMs);
	/*! \brief packTimestamp() of the current time. */
	uint64_t packNow();
	/*!
	 * \brief Reads a packTimestamp() value back as "YYYY-MM-DD hh:mm:ss.mmm". Also accepts the short
	 * 15-digit form YYMMDDhhmmssmmm (year 20YY). nullopt if the digits are not a valid date.
	 */
	std::optional<std::string> unpackTimestamp(uint64_t packed);

	/*! \brief Blocks the calling thread for ms milliseconds (never call it on the GUI thread). */
	void sleepMs(uint32_t ms);
	/*! \brief Blocks the calling thread for us microseconds (resolution depends on the OS scheduler). */
	void sleepUs(uint32_t us);

	/*!
	 * \brief Measures elapsed time on the monotonic clock.
	 *
	 *     Utils::Time::Stopwatch sw;
	 *     doWork();
	 *     Log::debug("took " + std::to_string(sw.elapsedMs()) + " ms");
	 */
	class Stopwatch {
	public:
		Stopwatch() { restart(); }
		/*! \brief Starts measuring again from now. */
		void restart() { start_ = std::chrono::steady_clock::now(); }
		/*! \brief Milliseconds since construction or the last restart(). */
		uint64_t elapsedMs() const {
			return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_).count());
		}
		/*! \brief Microseconds since construction or the last restart(). */
		uint64_t elapsedUs() const {
			return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_).count());
		}
		/*! \brief true once at least ms milliseconds have passed - handy for polling loops with a deadline. */
		bool hasElapsed(uint64_t ms) const { return elapsedMs() >= ms; }
	private:
		std::chrono::steady_clock::time_point start_;
	};
}
