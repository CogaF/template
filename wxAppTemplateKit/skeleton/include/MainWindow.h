// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <memory>
#include <string>

#include <wx/frame.h>
#include <wx/timer.h>

#include "Database.h"
#include "SerialLink.h"
#include "SerialWorker.h"

class wxButton;
class wxStaticText;
class wxTextCtrl;

/*!
 * \file MainWindow.h
 * \brief Skeleton main window: menus, a serial port you can open and send hex bytes to, the live
 * log, and a small event database - a working starting point showing how the pieces fit together.
 * Replace the centre panel with the real application.
 */
class MainWindow : public wxFrame {
public:
	MainWindow();
	~MainWindow() override;

private:
	void buildMenus();
	void buildContent();

	// --- menu / buttons ---
	void onSerialSettings(wxCommandEvent&);
	void onTogglePort(wxCommandEvent&);
	void onSend(wxCommandEvent&);
	void onTheme(wxCommandEvent&);
	void onLanguage(wxCommandEvent&);
	void onLogLevel(wxCommandEvent&);
	void onOpenDataFolder(wxCommandEvent&);
	void onVersionInfo(wxCommandEvent&);
	void onAbout(wxCommandEvent&);
	void onClose(wxCloseEvent& event);
	void onLogTimer(wxTimerEvent&);

	enum class ApplyChoice { RestartNow, UseNow, NextStart, Cancel };
	//! The "Restart now / Use now / Save for next start / Cancel" question (theme, language).
	ApplyChoice askHowToApply(const wxString& title);
	//! Starts a new instance of the exe and closes this one (returns false if it couldn't start).
	bool restartApplication();

	void openPort();
	void closePort();
	void updatePortControls();
	void recordEvent(const std::string& kind, const std::string& text);

	SerialConfig config_;
	SerialLink link_;
	std::unique_ptr<SerialWorker> worker_;
	Database db_;

	wxStaticText* portText_ = nullptr;
	wxButton* portButton_ = nullptr;
	wxTextCtrl* sendText_ = nullptr;
	wxButton* sendButton_ = nullptr;
	wxTextCtrl* logText_ = nullptr;
	wxTimer logTimer_;
	static constexpr int kMaxLogLines = 5000;
};
