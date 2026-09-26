// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <filesystem>
#include <wx/string.h>

/*!
 * \file DataDir.h
 * \brief Where the application keeps its files: "<exe folder>/<AppInfo::kDataFolderName>/".
 *
 * Settings, languages, databases, logs - everything goes through here, so the exe folder stays clean
 * and the whole state of an installation is one folder to back up or delete. The folder is created
 * on first use; if it cannot be created (read-only location) the exe folder is used instead.
 */
namespace DataDir {
	//! Folder of the running executable (reliable also when started from the debugger).
	std::filesystem::path exeDirectory();
	//! The data folder, created if missing.
	std::filesystem::path path();
	//! path() / fileName.
	std::filesystem::path file(const std::filesystem::path& fileName);
	//! The data folder as a wxString with a trailing separator (for file dialogs).
	wxString wx();
}
