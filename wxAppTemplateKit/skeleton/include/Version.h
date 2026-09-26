/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

/*!
 * \file Version.h
 * \brief Application version (semantic versioning) and its history - the ONE place both are kept.
 *
 *  - MAJOR: old files or habits stop working. Resets MINOR and PATCH.
 *  - MINOR: new features, everything old keeps working. Resets PATCH.
 *  - PATCH: bug fixes only.
 *  - PRERELEASE: "rc.1", "rc.2", ... while testing; "" for the final release.
 *
 * The BUILD number is not part of the version: the IncrementBuildNumber target in the .vcxproj
 * counts every build (BuildCounter.txt) and the git commit identifies the exact source. The build
 * also reads the four defines below (by regular expression) to name the versioned exe copy - keep
 * the "#define NAME value" form.
 *
 * Release routine: set the numbers, add an entry at the TOP of kAppVersionHistory, commit, tag
 * (git tag -a v1.2.0 -m "..."), build Release and hand out the versioned exe.
 */

/*! \brief MAJOR: incompatible changes. */
#define APP_VERSION_MAJOR 0
/*! \brief MINOR: new features, compatible. */
#define APP_VERSION_MINOR 1
/*! \brief PATCH: bug fixes only. */
#define APP_VERSION_PATCH 0
/*! \brief "rc.N" while testing, "" for the final release. */
#define APP_VERSION_PRERELEASE "rc.1"

/*! \brief One released (or release-candidate) version; notes: one item per line, "Added:/Changed:/Fixed:/Note:". */
struct AppVersionHistoryEntry {
	const char* version; /*!< "0.1.0-rc.1" */
	const char* date;    /*!< release date, YYYY-MM-DD */
	const char* notes;   /*!< one item per line */
};

/*! \brief Newest first. */
inline constexpr AppVersionHistoryEntry kAppVersionHistory[] = {
	{ "0.1.0-rc.1", "2026-09-26",
		"Note: project created from the wxAppTemplate kit." },
};
