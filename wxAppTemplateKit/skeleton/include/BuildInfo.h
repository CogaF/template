/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <string>

/*!
 * \file BuildInfo.h
 * \brief Version and build identification. Only BuildInfo.cpp includes the generated header
 * (GeneratedBuildInfo.h, rewritten before every build), so a build recompiles one small file.
 */

/*! \brief Build number (counts every build on the building PC - see IncrementBuildNumber in the .vcxproj). */
int GetBuildNumber();
/*! \brief "MAJOR.MINOR.PATCH[-PRERELEASE]", e.g. "0.1.0-rc.1". */
std::string GetAppVersion();
/*! \brief `git describe --always --dirty` of the built source, "unknown" without git. */
std::string GetGitDescribe();
/*! \brief Build date and time, "YYYY-MM-DD HH:MM". */
std::string GetBuildDate();

/*!
 * \brief Debug builds are development builds; Release builds are release candidates while Version.h has a
 * pre-release label, otherwise final releases.
 */
enum class AppBuildChannel {
	Development,      /*!< Debug build */
	ReleaseCandidate, /*!< Release build with a pre-release label in Version.h */
	Final             /*!< Release build without a pre-release label */
};
/*! \brief The channel of this build (see AppBuildChannel). */
AppBuildChannel GetBuildChannel();

/*! \brief "v0.1.0-rc.1b12" (+ "D" for Debug) - the same text the build puts in the versioned exe name. */
std::string GetCompactVersion();
/*! \brief Window title text: "Template App (v0.1.0-rc.1b12, 2026-09-26 14:03)". */
std::string GetWindowTitle();
