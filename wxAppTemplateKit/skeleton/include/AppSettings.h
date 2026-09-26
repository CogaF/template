/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

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
	/*! \brief Value of key, or nullopt if the key is not in the file. */
	std::optional<std::string> get(const std::string& key);
	/*! \brief Value of key, or fallback. */
	std::string getString(const std::string& key, const std::string& fallback = {});
	/*! \brief Value of key as an integer, or fallback if missing or not a number. */
	int getInt(const std::string& key, int fallback);
	/*! \brief Value of key as a floating-point number, or fallback if missing or not a number. */
	double getDouble(const std::string& key, double fallback);
	/*! \brief Value of key as a boolean ("1", "true", "yes" are true), or fallback if missing. */
	bool getBool(const std::string& key, bool fallback);

	/*! \brief Stores key=value (merge-safe, crash-safe - see the file comment); false if the file cannot be written. */
	bool set(const std::string& key, const std::string& value);
	/*! \brief Stores an integer value. */
	bool setInt(const std::string& key, int value);
	/*! \brief Stores a boolean value as "1" / "0". */
	bool setBool(const std::string& key, bool value);
}
