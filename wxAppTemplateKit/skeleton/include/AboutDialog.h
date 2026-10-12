/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <wx/dialog.h>

class wxNotebook;
class wxWindow;

/*!
 * \file AboutDialog.h
 * \brief Help > About: everything about this application and this installation, on tabs.
 */

/*!
 * \brief The About window, a notebook with these pages:
 *  - <b>About</b>: icon, name, version and build date, description, author, contact, project page,
 *    copyright, licence, what the application does;
 *  - <b>Developer</b>: author, e-mail, web page, "Report a problem" (an e-mail with the system
 *    details filled in, or the issues page);
 *  - <b>Dependencies</b>: every library used, its version, its licence and its home page (clickable);
 *  - <b>Changes</b>: the version history (Version.h);
 *  - <b>System</b>: versions, folders, database - with "Copy to clipboard" for a support request.
 *
 * Every name, address and link comes from AppInfo.h; an empty one hides its line. Add a page for
 * whatever the real application needs (instruments, licence state...).
 */
class AboutDialog : public wxDialog {
public:
	/*! \brief Builds the window. \param parent the main window. \param databaseFile the database in use ("" if none). */
	AboutDialog(wxWindow* parent, const wxString& databaseFile);

	/*! \brief The technical details of this installation, one "name: value" per line (always English: they go to the developer). */
	static wxString systemInfoText(const wxString& databaseFile);

private:
	wxWindow* buildAboutPage(wxNotebook* book);
	wxWindow* buildDeveloperPage(wxNotebook* book);
	wxWindow* buildDependenciesPage(wxNotebook* book);
	wxWindow* buildChangesPage(wxNotebook* book);
	wxWindow* buildSystemPage(wxNotebook* book);

	wxString databaseFile_; //!< shown on the System page
};
