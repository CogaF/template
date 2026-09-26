/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

/*!
 * \file Log.h
 * \brief Thread-safe application log.
 *
 * Any thread can call Log::write() (or the error()/warning()/... shortcuts). Each line goes to:
 *  - the log file (Log::enableFile(), e.g. "<data folder>/log.txt"), appended and flushed;
 *  - a bounded in-memory queue the GUI drains on a timer (Log::takePendingLines()) - the GUI is
 *    never flooded with one event per line, and nothing is lost before the main window exists
 *    (lines logged at startup simply wait in the queue);
 *  - the debugger output window (Windows).
 * Lines above the current level (Log::setLevel()) are dropped right away, at almost no cost.
 */
/*! \brief How much is written: every level includes the ones above it. */
enum class LogLevel {
	None = 0, /*!< nothing */
	Error,    /*!< failures */
	Warning,  /*!< unexpected but handled */
	Info,     /*!< normal milestones (default) */
	Debug,    /*!< details for troubleshooting */
	Verbose,  /*!< more details */
	Trace     /*!< everything, including serial traffic */
};

namespace Log {
	/*! \brief Sets the most detailed level that is written. */
	void setLevel(LogLevel level);
	/*! \brief The current level. */
	LogLevel level();
	/*! \brief "Error", "Warning", ... (English - used in the log itself and in settings). */
	const char* levelName(LogLevel level);
	/*! \brief true if a message of this level would be written (use it to skip building costly messages). */
	bool isEnabled(LogLevel level);

	/*! \brief Starts appending every line to this file (created if missing); an empty path stops it. */
	void enableFile(const std::filesystem::path& filePath);

	/*! \brief Writes one line: "2026-09-26 14:03:01.123 [E] [tid] message". Thread-safe. */
	void write(LogLevel level, const std::string& message);

	/*!
	 * \brief Lines written since the previous call (oldest first); at most kMaxPendingLines are kept -
	 * older ones are replaced by a single "... N line(s) dropped" marker.
	 */
	std::vector<std::string> takePendingLines();
	/*! \brief Lines kept for the GUI between two takePendingLines() calls. */
	inline constexpr size_t kMaxPendingLines = 5000;

	/*! \brief write(LogLevel::Error, m). */
	inline void error(const std::string& m) { write(LogLevel::Error, m); }
	/*! \brief write(LogLevel::Warning, m). */
	inline void warning(const std::string& m) { write(LogLevel::Warning, m); }
	/*! \brief write(LogLevel::Info, m). */
	inline void info(const std::string& m) { write(LogLevel::Info, m); }
	/*! \brief write(LogLevel::Debug, m). */
	inline void debug(const std::string& m) { write(LogLevel::Debug, m); }
	/*! \brief write(LogLevel::Verbose, m). */
	inline void verbose(const std::string& m) { write(LogLevel::Verbose, m); }
	/*! \brief write(LogLevel::Trace, m). */
	inline void trace(const std::string& m) { write(LogLevel::Trace, m); }

}
