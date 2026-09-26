/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/*!
 * \file TextUtils.h
 * \brief String helpers and small, safe file operations.
 *
 * Strings here are std::string holding UTF-8. For text shown in the GUI keep wxString (see I18n.h)
 * and convert at the boundary with wxString::utf8_string() / wxString::FromUTF8().
 */
namespace Utils::Str {
	/*! \brief s without leading and trailing whitespace (space, tab, CR, LF). */
	std::string trim(std::string_view s);
	/*! \brief s split at every separator; keepEmpty = false drops empty parts ("a,,b" -> "a","b"). */
	std::vector<std::string> split(std::string_view s, char separator, bool keepEmpty = true);
	/*! \brief parts joined with separator. */
	std::string join(const std::vector<std::string>& parts, std::string_view separator);
	/*! \brief ASCII lower case (non-ASCII UTF-8 bytes are kept as they are). */
	std::string toLower(std::string_view s);
	/*! \brief ASCII upper case (non-ASCII UTF-8 bytes are kept as they are). */
	std::string toUpper(std::string_view s);
	/*! \brief true if a and b are equal ignoring ASCII case. */
	bool equalsIgnoreCase(std::string_view a, std::string_view b);
	/*! \brief s with every occurrence of from replaced by to. */
	std::string replaceAll(std::string s, std::string_view from, std::string_view to);
	/*! \brief s cut to maxLength characters (bytes), with "..." at the end when it was cut. */
	std::string ellipsize(std::string_view s, size_t maxLength);
	/*! \brief Parses a whole string as a signed integer; nullopt on any extra character. */
	std::optional<int64_t> toInt(std::string_view s);
	/*! \brief Parses a whole string as a floating-point number ('.' as decimal separator, any locale). */
	std::optional<double> toDouble(std::string_view s);
}

namespace Utils::Files {
	/*! \brief Creates dir and any missing parents; true if it exists afterwards. */
	bool ensureDirectory(const std::filesystem::path& dir);
	/*!
	 * \brief name made safe as a file name on Windows: characters <>:"/\|?* and control characters
	 * become '_', trailing dots/spaces are removed, reserved names (CON, COM1, ...) get a '_' prefix.
	 * "\\\\.\\COM10" -> "COM10".
	 */
	std::string sanitizeFileName(std::string_view name);
	/*!
	 * \brief dir/base + extension if that file does not exist yet, otherwise dir/base_2 + extension,
	 * base_3 ... - never overwrites. extension includes the dot (".txt").
	 */
	std::filesystem::path uniquePath(const std::filesystem::path& dir, std::string_view base, std::string_view extension);
	/*! \brief dir/prefix_2026-09-26_14-03-01 + extension (see Utils::Time::fileNameStamp()), made unique. */
	std::filesystem::path timestampedPath(const std::filesystem::path& dir, std::string_view prefix, std::string_view extension);

	/*! \brief Whole file as a string (bytes as they are); nullopt if it cannot be read. */
	std::optional<std::string> readText(const std::filesystem::path& file);
	/*!
	 * \brief Replaces file with text through a temporary file + rename, so a crash or a full disk never
	 * leaves a half-written file. Creates missing folders.
	 */
	bool writeTextAtomic(const std::filesystem::path& file, std::string_view text);
	/*! \brief Appends one line (a '\n' is added) to file, creating it if needed. */
	bool appendLine(const std::filesystem::path& file, std::string_view line);
	/*! \brief Size of file in bytes, or nullopt if it does not exist. */
	std::optional<uint64_t> size(const std::filesystem::path& file);
	/*! \brief "0 B", "1.5 KB", "3.2 MB", "1.1 GB" - for showing sizes to the user. */
	std::string humanSize(uint64_t bytes);
}
