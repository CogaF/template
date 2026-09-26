// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#include "DataDir.h"
#include "AppInfo.h"

#include <system_error>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace DataDir {

std::filesystem::path exeDirectory() {
#ifdef _WIN32
	// GetModuleFileNameW: always the exe's own folder, also when the debugger's working directory
	// is the project folder (relative paths would land there instead).
	wchar_t buffer[MAX_PATH];
	const DWORD n = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
	if (n > 0 && n < MAX_PATH) return std::filesystem::path(buffer).parent_path();
#endif
	const wxString exe = wxStandardPaths::Get().GetExecutablePath();
	if (!exe.empty()) return std::filesystem::path(exe.ToStdWstring()).parent_path();
	return std::filesystem::current_path();
}

std::filesystem::path path() {
	const std::filesystem::path exeDir = exeDirectory();
	const std::filesystem::path dir = exeDir / AppInfo::kDataFolderName;
	std::error_code ec;
	if (std::filesystem::is_directory(dir, ec)) return dir;
	std::filesystem::create_directories(dir, ec);
	return ec ? exeDir : dir;
}

std::filesystem::path file(const std::filesystem::path& fileName) { return path() / fileName; }

wxString wx() {
	wxString dir(path().wstring());
	if (!dir.EndsWith(wxFILE_SEP_PATH)) dir += wxFILE_SEP_PATH;
	return dir;
}

} // namespace DataDir
