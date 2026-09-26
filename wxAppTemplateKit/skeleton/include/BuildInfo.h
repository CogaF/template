// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <string>

/*!
 * \file BuildInfo.h
 * \brief Version and build identification. Only BuildInfo.cpp includes the generated header
 * (GeneratedBuildInfo.h, rewritten before every build), so a build recompiles one small file.
 */

//! Build number (counts every build on the building PC - see IncrementBuildNumber in the .vcxproj).
int GetBuildNumber();
//! "MAJOR.MINOR.PATCH[-PRERELEASE]", e.g. "0.1.0-rc.1".
std::string GetAppVersion();
//! `git describe --always --dirty` of the built source, "unknown" without git.
std::string GetGitDescribe();
//! Build date and time, "YYYY-MM-DD HH:MM".
std::string GetBuildDate();

//! Debug builds are development builds; Release builds are release candidates while Version.h has a
//! pre-release label, otherwise final releases.
enum class AppBuildChannel { Development, ReleaseCandidate, Final };
AppBuildChannel GetBuildChannel();

//! "v0.1.0-rc.1b12" (+ "D" for Debug) - the same text the build puts in the versioned exe name.
std::string GetCompactVersion();
//! Window title text: "Template App (v0.1.0-rc.1b12, 2026-09-26 14:03)".
std::string GetWindowTitle();
