// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#include "AppSettings.h"
#include "DataDir.h"
#include "Log.h"

#include <charconv>
#include <fstream>
#include <mutex>
#include <vector>

namespace {
	std::mutex g_mutex;

	std::filesystem::path settingsFile() { return DataDir::file("AppSettings.txt"); }

	std::vector<std::string> readLines() {
		std::vector<std::string> lines;
		std::ifstream in(settingsFile());
		std::string line;
		while (std::getline(in, line)) {
			if (!line.empty() && line.back() == '\r') line.pop_back();
			lines.push_back(line);
		}
		return lines;
	}

	std::string trim(const std::string& s) {
		const size_t b = s.find_first_not_of(" \t");
		if (b == std::string::npos) return {};
		const size_t e = s.find_last_not_of(" \t");
		return s.substr(b, e - b + 1);
	}
}

namespace AppSettings {

std::optional<std::string> get(const std::string& key) {
	std::lock_guard<std::mutex> lock(g_mutex);
	for (const std::string& line : readLines()) {
		const size_t eq = line.find('=');
		if (eq == std::string::npos || line.starts_with('#')) continue;
		if (trim(line.substr(0, eq)) == key) return trim(line.substr(eq + 1));
	}
	return std::nullopt;
}

std::string getString(const std::string& key, const std::string& fallback) { return get(key).value_or(fallback); }

int getInt(const std::string& key, int fallback) {
	const auto v = get(key);
	if (!v) return fallback;
	int out = fallback;
	const auto [ptr, ec] = std::from_chars(v->data(), v->data() + v->size(), out);
	return (ec == std::errc()) ? out : fallback;
}

double getDouble(const std::string& key, double fallback) {
	const auto v = get(key);
	if (!v) return fallback;
	try { return std::stod(*v); } catch (...) { return fallback; }
}

bool getBool(const std::string& key, bool fallback) {
	const auto v = get(key);
	if (!v) return fallback;
	return *v == "1" || *v == "true" || *v == "yes";
}

bool set(const std::string& key, const std::string& value) {
	std::lock_guard<std::mutex> lock(g_mutex);
	std::vector<std::string> lines = readLines();
	bool replaced = false;
	for (std::string& line : lines) {
		const size_t eq = line.find('=');
		if (eq == std::string::npos || line.starts_with('#')) continue;
		if (trim(line.substr(0, eq)) == key) { line = key + "=" + value; replaced = true; }
	}
	if (!replaced) lines.push_back(key + "=" + value);

	const std::filesystem::path target = settingsFile();
	std::filesystem::path temp = target;
	temp += ".tmp";
	{
		std::ofstream out(temp, std::ios::trunc);
		if (!out.is_open()) {
			Log::error("AppSettings::set(): cannot write " + temp.string());
			return false;
		}
		for (const std::string& line : lines) out << line << '\n';
		if (!out.good()) return false;
	}
	std::error_code ec;
	std::filesystem::rename(temp, target, ec); // replaces the old file atomically
	if (ec) {
		Log::error("AppSettings::set(): cannot replace " + target.string() + ": " + ec.message());
		return false;
	}
	return true;
}

bool setInt(const std::string& key, int value) { return set(key, std::to_string(value)); }
bool setBool(const std::string& key, bool value) { return set(key, value ? "1" : "0"); }

} // namespace AppSettings
