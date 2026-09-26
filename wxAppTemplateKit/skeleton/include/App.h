/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <wx/app.h>

/*!
 * \file App.h
 * \brief Application entry point: loads settings, language and theme, then opens MainWindow.
 */
/*!
 * \brief The wxWidgets application object (see the file comment for the startup order).
 */
class App : public wxApp {
public:
	/*! \brief Log, language and theme first, then the main window. \return false to abort startup. */
	bool OnInit() override;
	/*! \brief Closes the log file. */
	int OnExit() override;
	/*! \brief Sees every event first - used to theme windows shown after a live theme switch. */
	int FilterEvent(wxEvent& event) override;
};

wxDECLARE_APP(App);
