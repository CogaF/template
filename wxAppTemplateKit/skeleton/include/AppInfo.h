/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

/*!
 * \file AppInfo.h
 * \brief The application's identity - the ONE place its name lives in the code.
 *
 * NewProject.ps1 replaces "Template App" / "TemplateApp" everywhere (code, project files, file
 * names), so a generated project starts with its own name. The exe name comes from
 * AppExeBaseName in the .vcxproj - keep both the same.
 */
namespace AppInfo {
	/*! \brief Shown in the window title, About box, status bar; also the exe file name. */
	inline constexpr const char* kName = "Template App";
	/*! \brief Folder next to the exe holding every file the application reads or writes (see DataDir.h). */
	inline constexpr const char* kDataFolderName = "Template App data";
	/*! \brief Copyright line shown in the About box. */
	inline constexpr const char* kCopyright = "Copyright (C) 2026 Fation Coga";
	/*! \brief Licence name shown in the About box. */
	inline constexpr const char* kLicense = "GNU Lesser General Public License v3.0 or later (LGPL-3.0-or-later)";
}
