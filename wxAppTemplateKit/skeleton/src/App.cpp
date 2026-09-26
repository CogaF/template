/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file App.cpp
 * \brief Implementation of App.h.
 */

#include "App.h"
#include "AppInfo.h"
#include "AppSettings.h"
#include "BuildInfo.h"
#include "DataDir.h"
#include "I18n.h"
#include "Log.h"
#include "MainWindow.h"
#include "Theme.h"

#include <wx/toplevel.h>

wxIMPLEMENT_APP(App);

bool App::OnInit() {
	if (!wxApp::OnInit()) return false;
	SetAppDisplayName(wxString::FromUTF8(AppInfo::kName));

	// Log first, so everything after it is recorded (<data folder>/log.txt).
	Log::setLevel(static_cast<LogLevel>(AppSettings::getInt("logLevel", static_cast<int>(LogLevel::Info))));
	Log::enableFile(DataDir::file("log.txt"));
	Log::info(std::string(AppInfo::kName) + " " + GetCompactVersion() + " starting, data folder: " + DataDir::path().string());

	I18n::load(AppSettings::getString("language", "en"));
	Theme::applyAtStartup(); // before the first window

	auto* frame = new MainWindow();
	frame->Show(true);
	return true;
}

int App::OnExit() {
	Log::info("Exiting.");
	Log::enableFile({});
	return wxApp::OnExit();
}

int App::FilterEvent(wxEvent& event) {
	if (event.GetEventType() == wxEVT_SHOW && static_cast<wxShowEvent&>(event).IsShown()) {
		if (auto* tlw = wxDynamicCast(event.GetEventObject(), wxTopLevelWindow)) Theme::onWindowShown(tlw);
	}
	return wxApp::FilterEvent(event);
}
