/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file Log.cpp
 * \brief Implementation of Log.h.
 */

#include "Log.h"
#include "TimeUtils.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <format>
#include <fstream>
#include <mutex>
#include <sstream>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // std::min/std::max, not the windows.h macros
#endif
#include <windows.h>
#endif

namespace {
	std::atomic<int> g_level{ static_cast<int>(LogLevel::Info) };
	std::mutex g_mutex;               // guards everything below
	std::ofstream g_file;
	std::deque<std::string> g_pending;
	size_t g_dropped = 0;

	char levelLetter(LogLevel level) {
		switch (level) {
		case LogLevel::Error:   return 'E';
		case LogLevel::Warning: return 'W';
		case LogLevel::Info:    return 'I';
		case LogLevel::Debug:   return 'D';
		case LogLevel::Verbose: return 'V';
		case LogLevel::Trace:   return 'T';
		default:                return '-';
		}
	}
}

namespace Log {

void setLevel(LogLevel level) { g_level.store(static_cast<int>(level), std::memory_order_relaxed); }
LogLevel level() { return static_cast<LogLevel>(g_level.load(std::memory_order_relaxed)); }

const char* levelName(LogLevel level) {
	switch (level) {
	case LogLevel::None:    return "None";
	case LogLevel::Error:   return "Error";
	case LogLevel::Warning: return "Warning";
	case LogLevel::Info:    return "Info";
	case LogLevel::Debug:   return "Debug";
	case LogLevel::Verbose: return "Verbose";
	case LogLevel::Trace:   return "Trace";
	}
	return "?";
}

bool isEnabled(LogLevel lvl) {
	return lvl != LogLevel::None && static_cast<int>(lvl) <= g_level.load(std::memory_order_relaxed);
}

void enableFile(const std::filesystem::path& filePath) {
	std::lock_guard<std::mutex> lock(g_mutex);
	if (g_file.is_open()) g_file.close();
	if (!filePath.empty()) g_file.open(filePath, std::ios::app);
}

void write(LogLevel lvl, const std::string& message) {
	if (!isEnabled(lvl)) return;
	std::ostringstream tid;
	tid << std::this_thread::get_id();
	const std::string line = std::format("{} [{}] [{}] {}", Utils::Time::nowString(), levelLetter(lvl), tid.str(), message);

#ifdef _WIN32
	OutputDebugStringA((line + "\n").c_str());
#endif
	std::lock_guard<std::mutex> lock(g_mutex);
	if (g_file.is_open()) {
		g_file << line << '\n';
		g_file.flush(); // complete up to the last line even after a crash
	}
	if (g_pending.size() >= kMaxPendingLines) {
		g_pending.pop_front();
		++g_dropped;
	}
	g_pending.push_back(line);
}

std::vector<std::string> takePendingLines() {
	std::lock_guard<std::mutex> lock(g_mutex);
	std::vector<std::string> out;
	out.reserve(g_pending.size() + 1);
	if (g_dropped > 0) {
		out.push_back(std::format("... {} older line(s) not shown (the GUI fell behind) - see the log file", g_dropped));
		g_dropped = 0;
	}
	out.insert(out.end(), std::make_move_iterator(g_pending.begin()), std::make_move_iterator(g_pending.end()));
	g_pending.clear();
	return out;
}

} // namespace Log
