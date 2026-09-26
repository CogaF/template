// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <wx/app.h>

/*!
 * \file App.h
 * \brief Application entry point: loads settings, language and theme, then opens MainWindow.
 */
class App : public wxApp {
public:
	bool OnInit() override;
	int OnExit() override;
	//! Sees every event first - used to theme windows shown after a live theme switch.
	int FilterEvent(wxEvent& event) override;
};

wxDECLARE_APP(App);
