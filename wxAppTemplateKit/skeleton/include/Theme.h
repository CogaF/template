/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <wx/window.h>

/*!
 * \file Theme.h
 * \brief Light / dark appearance.
 *
 * The saved choice ("theme=dark|light" in AppSettings) is applied at startup, before the first
 * window, through wxApp::SetAppearance() (wxWidgets 3.3+) - the only moment wxWidgets applies it
 * completely. switchNow() changes it while running ("Use now"): the app's own colours of every open
 * window, and of every window shown afterwards (App::FilterEvent() calls onWindowShown()). With
 * wxWidgets 3.3.4+ the parts drawn by Windows itself (title bars, scroll bars) switch too; with older
 * versions they follow at the next start.
 */
namespace Theme {
	/*! \brief The theme saved in AppSettings (true = dark). */
	bool savedDark();
	/*! \brief Saves the theme for the next start (true = dark). */
	void save(bool dark);

	/*! \brief Call once in OnInit() before creating any window. */
	void applyAtStartup();
	/*! \brief The theme the application started with (true = dark). */
	bool startupDark();

	/*!
	 * \brief Live switch. Returns true if the whole appearance switched (native parts included),
	 * false if only the application's colours did (restart to apply it completely).
	 */
	bool switchNow(bool dark);
	/*! \brief Recolours win (and its children) if a live switch happened this session. */
	void onWindowShown(wxWindow* win);
	/*! \brief Recolours win and its children for the given theme. */
	void applyColours(wxWindow* win, bool dark);
}
