/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file Theme.cpp
 * \brief Implementation of Theme.h.
 */

#include "Theme.h"
#include "AppSettings.h"
#include "Log.h"

#include <wx/app.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/listbox.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/toplevel.h>

namespace {
	bool g_startupDark = false;
	int g_liveOverride = -1; // -1 none, 0 dark, 1 light (see switchNow())

	const wxColour kDarkBackground(30, 30, 30);
	const wxColour kDarkForeground(220, 220, 220);
}

namespace Theme {

bool savedDark() { return AppSettings::getString("theme", "light") == "dark"; }
void save(bool dark) { AppSettings::set("theme", dark ? "dark" : "light"); }
bool startupDark() { return g_startupDark; }

void applyAtStartup() {
	g_startupDark = savedDark();
#if wxCHECK_VERSION(3, 3, 0)
	const auto result = wxTheApp->SetAppearance(g_startupDark ? wxApp::Appearance::Dark : wxApp::Appearance::Light);
	if (result != wxApp::AppearanceResult::Ok) Log::warning("Theme: the system appearance could not be set.");
#endif
}

void applyColours(wxWindow* win, bool dark) {
	if (!win) return;
	if (dark) {
		win->SetBackgroundColour(kDarkBackground);
		win->SetForegroundColour(kDarkForeground);
	}
	else if (!g_startupDark) {
		// Back to the platform defaults.
		win->SetBackgroundColour(wxNullColour);
		win->SetForegroundColour(wxNullColour);
	}
	else {
		// Started dark: the platform "default" is still dark then - use explicit light colours.
		const bool isInput = wxDynamicCast(win, wxTextCtrl) || wxDynamicCast(win, wxListBox) ||
			wxDynamicCast(win, wxChoice) || wxDynamicCast(win, wxComboBox) || wxDynamicCast(win, wxListCtrl);
		win->SetBackgroundColour(isInput ? *wxWHITE : wxColour(240, 240, 240));
		win->SetForegroundColour(*wxBLACK);
	}
	for (wxWindow* child : win->GetChildren()) applyColours(child, dark);
	win->Refresh();
}

bool switchNow(bool dark) {
	bool nativeSwitched = false;
#if wxCHECK_VERSION(3, 3, 4)
	// Up to 3.3.3 SetAppearance() refuses once a window exists (CannotChange); 3.3.4 switches live.
	nativeSwitched = wxTheApp->SetAppearance(dark ? wxApp::Appearance::Dark : wxApp::Appearance::Light)
		== wxApp::AppearanceResult::Ok;
#endif
	g_liveOverride = dark ? 0 : 1;
	for (wxWindow* tlw : wxTopLevelWindows) applyColours(tlw, dark);
	return nativeSwitched;
}

void onWindowShown(wxWindow* win) {
	if (g_liveOverride >= 0) applyColours(win, g_liveOverride == 0);
}

} // namespace Theme
