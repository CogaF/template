// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <optional>
#include <string>

/*!
 * \file AppSettings.h
 * \brief Small persistent settings: "key=value" lines in "<data folder>/AppSettings.txt".
 *
 * Every set() re-reads the file, replaces (or appends) only its own key and writes the whole file
 * through a temporary + rename - so two settings written one after the other never overwrite each
 * other, and a crash mid-write never leaves a half-written file. Unknown lines (comments, keys of
 * newer versions) are kept as they are. Thread-safe.
 */
namespace AppSettings {
	std::optional<std::string> get(const std::string& key);
	std::string getString(const std::string& key, const std::string& fallback = {});
	int getInt(const std::string& key, int fallback);
	double getDouble(const std::string& key, double fallback);
	bool getBool(const std::string& key, bool fallback);

	bool set(const std::string& key, const std::string& value);
	bool setInt(const std::string& key, int value);
	bool setBool(const std::string& key, bool value);
}
