/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file TextUtils.cpp
 * \brief Implementation of TextUtils.h.
 */

#include "TextUtils.h"
#include "TimeUtils.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>
#include <fstream>
#include <sstream>
#include <system_error>

namespace Utils::Str {

std::string trim(std::string_view s) {
	const auto isSpace = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
	size_t b = 0, e = s.size();
	while (b < e && isSpace(s[b])) ++b;
	while (e > b && isSpace(s[e - 1])) --e;
	return std::string(s.substr(b, e - b));
}

std::vector<std::string> split(std::string_view s, char separator, bool keepEmpty) {
	std::vector<std::string> parts;
	size_t start = 0;
	for (;;) {
		const size_t pos = s.find(separator, start);
		const std::string_view part = s.substr(start, pos == std::string_view::npos ? std::string_view::npos : pos - start);
		if (keepEmpty || !part.empty()) parts.emplace_back(part);
		if (pos == std::string_view::npos) break;
		start = pos + 1;
	}
	return parts;
}

std::string join(const std::vector<std::string>& parts, std::string_view separator) {
	std::string out;
	for (size_t i = 0; i < parts.size(); ++i) {
		if (i) out += separator;
		out += parts[i];
	}
	return out;
}

std::string toLower(std::string_view s) {
	std::string out(s);
	for (char& c : out) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
	return out;
}

std::string toUpper(std::string_view s) {
	std::string out(s);
	for (char& c : out) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
	return out;
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) { return a.size() == b.size() && toLower(a) == toLower(b); }

std::string replaceAll(std::string s, std::string_view from, std::string_view to) {
	if (from.empty()) return s;
	for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size()) s.replace(pos, from.size(), to);
	return s;
}

std::string ellipsize(std::string_view s, size_t maxLength) {
	if (s.size() <= maxLength) return std::string(s);
	if (maxLength <= 3) return std::string(s.substr(0, maxLength));
	return std::string(s.substr(0, maxLength - 3)) + "...";
}

std::optional<int64_t> toInt(std::string_view s) {
	const std::string t = trim(s);
	int64_t v = 0;
	const auto [ptr, ec] = std::from_chars(t.data(), t.data() + t.size(), v);
	if (ec != std::errc() || ptr != t.data() + t.size() || t.empty()) return std::nullopt;
	return v;
}

std::optional<double> toDouble(std::string_view s) {
	const std::string t = trim(s);
	if (t.empty()) return std::nullopt;
	std::istringstream in(t);
	in.imbue(std::locale::classic()); // '.' as decimal separator whatever the user's locale
	double v = 0;
	in >> v;
	if (in.fail() || !in.eof()) return std::nullopt;
	return v;
}

} // namespace Utils::Str

namespace Utils::Files {

bool ensureDirectory(const std::filesystem::path& dir) {
	std::error_code ec;
	if (std::filesystem::is_directory(dir, ec)) return true;
	std::filesystem::create_directories(dir, ec);
	return std::filesystem::is_directory(dir, ec);
}

std::string sanitizeFileName(std::string_view name) {
	// Windows device / long-path prefixes ("\\.\COM10", "\\?\C:\...") are not part of the name.
	if (name.starts_with("\\\\.\\") || name.starts_with("\\\\?\\")) name.remove_prefix(4);
	std::string out;
	for (char c : name) {
		const bool bad = static_cast<unsigned char>(c) < 0x20 || std::string_view("<>:\"/\\|?*").find(c) != std::string_view::npos;
		out += bad ? '_' : c;
	}
	// No leading underscores left over from replaced separators ("/dev/ttyUSB0" -> "dev_ttyUSB0").
	const size_t first = out.find_first_not_of('_');
	out = first == std::string::npos ? std::string() : out.substr(first);
	while (!out.empty() && (out.back() == '.' || out.back() == ' ')) out.pop_back();
	if (out.empty()) return "_";
	static const char* kReserved[] = { "CON", "PRN", "AUX", "NUL", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
		"COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9" };
	const std::string stem = Utils::Str::toUpper(out.substr(0, out.find('.')));
	for (const char* r : kReserved) if (stem == r) return "_" + out;
	return out;
}

std::filesystem::path uniquePath(const std::filesystem::path& dir, std::string_view base, std::string_view extension) {
	std::error_code ec;
	std::filesystem::path candidate = dir / (std::string(base) + std::string(extension));
	for (int n = 2; std::filesystem::exists(candidate, ec); ++n) {
		candidate = dir / (std::format("{}_{}", base, n) + std::string(extension));
	}
	return candidate;
}

std::filesystem::path timestampedPath(const std::filesystem::path& dir, std::string_view prefix, std::string_view extension) {
	return uniquePath(dir, std::string(prefix) + "_" + Utils::Time::fileNameStamp(), extension);
}

std::optional<std::string> readText(const std::filesystem::path& file) {
	std::ifstream in(file, std::ios::binary);
	if (!in.is_open()) return std::nullopt;
	std::ostringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

bool writeTextAtomic(const std::filesystem::path& file, std::string_view text) {
	if (file.has_parent_path() && !ensureDirectory(file.parent_path())) return false;
	std::filesystem::path temp = file;
	temp += ".tmp";
	{
		std::ofstream out(temp, std::ios::binary | std::ios::trunc);
		if (!out.is_open()) return false;
		out.write(text.data(), static_cast<std::streamsize>(text.size()));
		if (!out.good()) return false;
	}
	std::error_code ec;
	std::filesystem::rename(temp, file, ec);
	return !ec;
}

bool appendLine(const std::filesystem::path& file, std::string_view line) {
	std::ofstream out(file, std::ios::binary | std::ios::app);
	if (!out.is_open()) return false;
	out.write(line.data(), static_cast<std::streamsize>(line.size()));
	out.put('\n');
	return out.good();
}

std::optional<uint64_t> size(const std::filesystem::path& file) {
	std::error_code ec;
	const auto s = std::filesystem::file_size(file, ec);
	if (ec) return std::nullopt;
	return static_cast<uint64_t>(s);
}

std::string humanSize(uint64_t bytes) {
	static const char* kUnits[] = { "B", "KB", "MB", "GB", "TB" };
	double v = static_cast<double>(bytes);
	int unit = 0;
	while (v >= 1024.0 && unit < 4) { v /= 1024.0; ++unit; }
	return unit == 0 ? std::format("{} B", bytes) : std::format("{:.1f} {}", v, kUnits[unit]);
}

} // namespace Utils::Files
