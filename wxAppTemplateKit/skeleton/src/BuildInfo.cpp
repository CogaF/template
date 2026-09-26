// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#include "BuildInfo.h"
#include "AppInfo.h"
#include "Version.h"

#include <format>

// Written by the IncrementBuildNumber target before every build (gitignored). The fallback keeps the
// code compiling outside that build (other build systems, code analysis).
#if __has_include("GeneratedBuildInfo.h")
#include "GeneratedBuildInfo.h"
#else
constexpr int BUILD_NUMBER = 0;
constexpr const char* GIT_DESCRIBE = "unknown";
constexpr const char* BUILD_DATE = __DATE__ " " __TIME__;
#endif

int GetBuildNumber() { return BUILD_NUMBER; }

std::string GetAppVersion() {
	std::string v = std::format("{}.{}.{}", APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
	const std::string pre = APP_VERSION_PRERELEASE;
	if (!pre.empty()) v += "-" + pre;
	return v;
}

std::string GetGitDescribe() {
	const std::string d = GIT_DESCRIBE;
	return d.empty() ? std::string("unknown") : d;
}

std::string GetBuildDate() { return BUILD_DATE; }

AppBuildChannel GetBuildChannel() {
#ifdef _DEBUG
	return AppBuildChannel::Development;
#else
	return std::string(APP_VERSION_PRERELEASE).empty() ? AppBuildChannel::Final : AppBuildChannel::ReleaseCandidate;
#endif
}

std::string GetCompactVersion() {
	// Keep in sync with the CopyVersionedExe target in the .vcxproj (the exe file name).
	std::string s = std::format("v{}b{}", GetAppVersion(), GetBuildNumber());
	if (GetBuildChannel() == AppBuildChannel::Development) s += "D";
	return s;
}

std::string GetWindowTitle() {
	return std::format("{} ({}, {})", AppInfo::kName, GetCompactVersion(), GetBuildDate());
}
