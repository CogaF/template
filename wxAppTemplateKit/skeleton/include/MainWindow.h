/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <memory>
#include <string>

#include <wx/frame.h>
#include <wx/timer.h>

#include <vector>

#include "Database.h"
#include "SerialLink.h"
#include "SerialMessage.h"
#include "SerialMonitor.h"

class DataEntry;
class StatusLed;
class wxButton;
class wxCheckBox;
class wxStaticText;
class wxTextCtrl;

/*!
 * \file MainWindow.h
 * \brief Skeleton main window - a small serial terminal showing how the pieces fit together:
 *  - a serial port that, once open, is always listened to (SerialMonitor): everything received and
 *    sent is printed in the serial console (control characters as [0D=CR], other bytes as [C1]);
 *  - a send box with Hex / ASCII / Mixed notation and a character table (DataEntry);
 *  - auto replies and periodic messages with incrementing values and checksums (AutomationDialog);
 *  - the live application log, a small event database, theme / language / log level, version info.
 * Replace or extend the centre panel with the real application.
 */
/*!
 * \brief The main window (see the file comment).
 */
class MainWindow : public wxFrame {
public:
	/*! \brief Builds menus and content, opens the event database. */
	MainWindow();
	/*! \brief Stops the serial monitor and closes the port. */
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
	void onEditAutomation(bool periodicPage);

	enum class ApplyChoice { RestartNow, UseNow, NextStart, Cancel };
	/*! \brief The "Restart now / Use now / Save for next start / Cancel" question (theme, language). */
	ApplyChoice askHowToApply(const wxString& title);
	/*! \brief Starts a new instance of the exe and closes this one (returns false if it couldn't start). */
	bool restartApplication();

	void openPort();
	void closePort();
	void updatePortControls();
	void recordEvent(const std::string& kind, const std::string& text);

	/*! \brief Prints what crossed the line (called by the SerialMonitor on the GUI thread). */
	void onSerialEvents(std::vector<SerialMonitor::Event>& events);
	/*! \brief One console line: time, a coloured label, the data (and its hex). */
	void printTraffic(uint64_t epochMs, const wxString& label, const wxColour& colour, const std::vector<uint8_t>& bytes, const wxString& note = {});

	/*! \brief Reads the auto replies and periodic messages from the settings. */
	void loadAutomation();
	/*! \brief Writes them to the settings. */
	void saveAutomation();
	/*! \brief Hands the auto replies to the monitor and (re)starts the periodic messages, as switched on. */
	void applyAutomation();
	/*! \brief Stops every periodic message. */
	void stopPeriodic();
	/*! \brief "(2 of 3 enabled)" next to the switches. */
	void updateAutomationSummary();

	/*! \brief A periodic message being sent: its generator (counters) and its timer. */
	struct PeriodicRun {
		std::string name;                    /*!< shown in the console */
		SerialData::MessageGenerator generator; /*!< builds the messages */
		std::unique_ptr<wxTimer> timer;      /*!< fires every period */
	};
	void sendPeriodic(PeriodicRun& run);

	SerialConfig config_;
	SerialLink link_;
	std::unique_ptr<SerialMonitor> monitor_;
	Database db_;
	std::vector<SerialData::AutoReplyRule> autoReplies_;
	std::vector<SerialData::PeriodicMessage> periodic_;
	std::vector<std::unique_ptr<PeriodicRun>> periodicRuns_;

	StatusLed* portLed_ = nullptr;
	wxStaticText* portText_ = nullptr;
	wxButton* portButton_ = nullptr;
	DataEntry* sendEntry_ = nullptr;
	wxButton* sendButton_ = nullptr;
	wxCheckBox* autoRepliesOn_ = nullptr;
	wxCheckBox* periodicOn_ = nullptr;
	wxStaticText* autoRepliesInfo_ = nullptr;
	wxStaticText* periodicInfo_ = nullptr;
	wxTextCtrl* console_ = nullptr;
	wxCheckBox* consoleTime_ = nullptr;
	wxCheckBox* consoleHex_ = nullptr;
	wxCheckBox* consoleFollow_ = nullptr;
	wxTextCtrl* logText_ = nullptr;
	wxTimer logTimer_;
	static constexpr int kMaxLogLines = 5000;
	static constexpr int kMaxConsoleLines = 5000;
};
