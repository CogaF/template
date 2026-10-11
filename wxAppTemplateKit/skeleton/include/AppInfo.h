/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

/*!
 * \file AppInfo.h
 * \brief Who and what the application is: names, author, contacts, links - the ONE place they live
 * in the code (window title, About window, status bar, data folder).
 *
 * NewProject.ps1 replaces "Template App" / "TemplateApp" everywhere (code, project files, file
 * names) and fills in the author, contact and GitHub links below from its questions, so a generated
 * project starts with its own identity. The exe name comes from AppExeBaseName in the .vcxproj -
 * keep both the same. An empty text hides its line in the About window.
 */
namespace AppInfo {
	/*! \brief Shown in the window title, About box, status bar; also the exe file name. */
	inline constexpr const char* kName = "Template App";
	/*! \brief Folder next to the exe holding every file the application reads or writes (see DataDir.h). */
	inline constexpr const char* kDataFolderName = "Template App data";
	/*! \brief One or two sentences: what the application does (About window, translated through tr()). */
	inline constexpr const char* kDescription =
		"A starting point for Windows desktop applications in C++20 with wxWidgets, SQLite and serial ports: "
		"logging, settings, translations, themes, versioning, a serial terminal and a database, "
		"ready to become your own project.";
	/*! \brief A light remark under the description ("" for none; translated through tr()). */
	inline constexpr const char* kTagline =
		"Why TEMP over LATE? Because every project starts out TEMPorary and is delivered a little LATE. "
		"Run NewProject.bat now - before it is too late \xF0\x9F\x99\x82";
	/*! \brief Author (About and Developer pages). */
	inline constexpr const char* kAuthor = "Fation Coga";
	/*! \brief Contact e-mail ("" for none: no e-mail link, problems are reported on kIssues). */
	inline constexpr const char* kContact = "";
	/*! \brief Project page, e.g. the GitHub repository ("" for none). */
	inline constexpr const char* kWebsite = "";
	/*! \brief Where problems are reported, e.g. the repository's issues page ("" for none). */
	inline constexpr const char* kIssues = "";
	/*! \brief The author's page, e.g. the GitHub profile ("" for none). */
	inline constexpr const char* kDeveloperPage = "";
	/*! \brief Copyright line shown in the About box. */
	inline constexpr const char* kCopyright = "Copyright (C) 2026 Fation Coga";
	/*! \brief Licence name shown in the About box. */
	inline constexpr const char* kLicense = "GNU Lesser General Public License v3.0 or later (LGPL-3.0-or-later)";
	/*! \brief Where the licence text is, relative to kWebsite ("" when there is no website). */
	inline constexpr const char* kLicensePath = "/blob/main/LICENSE.md";
}
