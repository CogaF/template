/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file TimeUtils.cpp
 * \brief Implementation of TimeUtils.h.
 */

#include "TimeUtils.h"

#include <ctime>
#include <format>
#include <thread>

namespace {
	/*! \brief Thread-safe conversion of a time_t to local calendar time. */
	std::tm localTime(std::time_t t) {
		std::tm tm{};
#ifdef _WIN32
		localtime_s(&tm, &t);
#else
		localtime_r(&t, &tm);
#endif
		return tm;
	}

	/*! \brief Splits an epoch-ms instant into local calendar time and the millisecond part. */
	std::tm splitEpochMs(uint64_t epochMs, int& millis) {
		millis = static_cast<int>(epochMs % 1000);
		return localTime(static_cast<std::time_t>(epochMs / 1000));
	}
}

namespace Utils::Time {

uint64_t nowMonotonicMs() {
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

uint64_t nowMonotonicUs() {
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count());
}

uint64_t nowEpochMs() {
	using namespace std::chrono;
	return static_cast<uint64_t>(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
}

std::string toString(uint64_t epochMs, bool withMilliseconds) {
	int ms = 0;
	const std::tm tm = splitEpochMs(epochMs, ms);
	std::string s = std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (withMilliseconds) s += std::format(".{:03}", ms);
	return s;
}

std::string nowString(bool withMilliseconds) { return toString(nowEpochMs(), withMilliseconds); }

std::string timeOfDayString(bool withMilliseconds) {
	int ms = 0;
	const std::tm tm = splitEpochMs(nowEpochMs(), ms);
	std::string s = std::format("{:02}:{:02}:{:02}", tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (withMilliseconds) s += std::format(".{:03}", ms);
	return s;
}

std::string dateString() {
	int ms = 0;
	const std::tm tm = splitEpochMs(nowEpochMs(), ms);
	return std::format("{:04}-{:02}-{:02}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}

std::string yearString() {
	int ms = 0;
	return std::to_string(splitEpochMs(nowEpochMs(), ms).tm_year + 1900);
}

std::string fileNameStamp() {
	int ms = 0;
	const std::tm tm = splitEpochMs(nowEpochMs(), ms);
	return std::format("{:04}-{:02}-{:02}_{:02}-{:02}-{:02}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

std::string durationString(int64_t milliseconds, bool withMilliseconds) {
	const bool negative = milliseconds < 0;
	uint64_t ms = negative ? static_cast<uint64_t>(-milliseconds) : static_cast<uint64_t>(milliseconds);
	const uint64_t days = ms / 86400000ULL;   ms %= 86400000ULL;
	const uint64_t hours = ms / 3600000ULL;   ms %= 3600000ULL;
	const uint64_t minutes = ms / 60000ULL;   ms %= 60000ULL;
	const uint64_t seconds = ms / 1000ULL;    ms %= 1000ULL;
	std::string s = negative ? "-" : "";
	if (days > 0) s += std::format("{}d ", days);
	s += std::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
	if (withMilliseconds) s += std::format(".{:03}", ms);
	return s;
}

uint64_t packTimestamp(uint64_t epochMs) {
	int ms = 0;
	const std::tm tm = splitEpochMs(epochMs, ms);
	uint64_t v = static_cast<uint64_t>(tm.tm_year + 1900);
	v = v * 100 + static_cast<uint64_t>(tm.tm_mon + 1);
	v = v * 100 + static_cast<uint64_t>(tm.tm_mday);
	v = v * 100 + static_cast<uint64_t>(tm.tm_hour);
	v = v * 100 + static_cast<uint64_t>(tm.tm_min);
	v = v * 100 + static_cast<uint64_t>(tm.tm_sec);
	return v * 1000 + static_cast<uint64_t>(ms);
}

uint64_t packNow() { return packTimestamp(nowEpochMs()); }

std::optional<std::string> unpackTimestamp(uint64_t packed) {
	std::string digits = std::to_string(packed);
	if (digits.size() <= 15) digits.insert(0, 15 - digits.size(), '0'); // short form: YYMMDD...
	if (digits.size() == 15) digits.insert(0, "20");
	if (digits.size() != 17) return std::nullopt;
	const int month = std::stoi(digits.substr(4, 2)), day = std::stoi(digits.substr(6, 2));
	const int hour = std::stoi(digits.substr(8, 2)), minute = std::stoi(digits.substr(10, 2)), second = std::stoi(digits.substr(12, 2));
	if (month < 1 || month > 12 || day < 1 || day > 31 || hour > 23 || minute > 59 || second > 60) return std::nullopt;
	return std::format("{}-{}-{} {}:{}:{}.{}", digits.substr(0, 4), digits.substr(4, 2), digits.substr(6, 2),
		digits.substr(8, 2), digits.substr(10, 2), digits.substr(12, 2), digits.substr(14, 3));
}

void sleepMs(uint32_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
void sleepUs(uint32_t us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }

} // namespace Utils::Time
