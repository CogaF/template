/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file MainWindow.cpp
 * \brief Implementation of MainWindow.h.
 */

#include "MainWindow.h"
#include "AppInfo.h"
#include "AppSettings.h"
#include "BuildInfo.h"
#include "DataDir.h"
#include "GuiUtils.h"
#include "HexUtils.h"
#include "I18n.h"
#include "Log.h"
#include "SerialConfigDialog.h"
#include "StatusLed.h"
#include "Theme.h"
#include "UVT.h"
#include "Version.h"

#include <sstream>

#include <wx/button.h>
#include <wx/choicdlg.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/statusbr.h>
#include <wx/stdpaths.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/version.h>

#include "sqlite3.h"

namespace {
	enum MenuId {
		ID_SERIAL_SETTINGS = wxID_HIGHEST + 1,
		ID_THEME,
		ID_LANGUAGE,
		ID_LOG_LEVEL,
		ID_OPEN_DATA_FOLDER,
		ID_VERSION_INFO,
	};

}

MainWindow::MainWindow()
	: wxFrame(nullptr, wxID_ANY, wxString::FromUTF8(GetWindowTitle()), wxDefaultPosition, wxSize(900, 600)),
	  logTimer_(this) {
	if (const auto saved = SerialConfig::fromString(AppSettings::getString("serialPort"))) config_ = *saved;

	buildMenus();
	buildContent();
	CreateStatusBar(2);
	SetStatusText(tr(UVT::STATUSBAR_READY), 0);
	updatePortControls();

	if (db_.open(DataDir::file("events.db"))) {
		db_.exec("CREATE TABLE IF NOT EXISTS events("
			"id INTEGER PRIMARY KEY, time_utc TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, kind TEXT NOT NULL, text TEXT)");
		recordEvent("app", "started " + GetCompactVersion());
	}

	worker_ = std::make_unique<SerialWorker>(link_, this);
	Bind(wxEVT_TIMER, &MainWindow::onLogTimer, this, logTimer_.GetId());
	Bind(wxEVT_CLOSE_WINDOW, &MainWindow::onClose, this);
	logTimer_.Start(200);

	if (Theme::startupDark()) Theme::applyColours(this, true);
	Centre();
}

MainWindow::~MainWindow() {
	if (worker_) worker_->stop();
	link_.close();
}

void MainWindow::buildMenus() {
	auto* file = new wxMenu();
	file->Append(wxID_EXIT, tr(UVT::MENU_EXIT));

	auto* settings = new wxMenu();
	settings->Append(ID_SERIAL_SETTINGS, tr(UVT::MENU_SERIAL_PORT));
	settings->AppendSeparator();
	settings->Append(ID_THEME, tr(UVT::MENU_THEME));
	settings->Append(ID_LANGUAGE, tr(UVT::MENU_LANGUAGE));
	settings->Append(ID_LOG_LEVEL, tr(UVT::MENU_LOG_LEVEL));
	settings->AppendSeparator();
	settings->Append(ID_OPEN_DATA_FOLDER, tr(UVT::MENU_OPEN_DATA_FOLDER));

	auto* help = new wxMenu();
	help->Append(ID_VERSION_INFO, tr(UVT::MENU_VERSION_INFO));
	help->Append(wxID_ABOUT, tr(UVT::MENU_ABOUT));

	auto* bar = new wxMenuBar();
	bar->Append(file, tr(UVT::MENU_FILE));
	bar->Append(settings, tr(UVT::MENU_SETTINGS));
	bar->Append(help, tr(UVT::MENU_HELP));
	SetMenuBar(bar);

	Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);
	Bind(wxEVT_MENU, &MainWindow::onSerialSettings, this, ID_SERIAL_SETTINGS);
	Bind(wxEVT_MENU, &MainWindow::onTheme, this, ID_THEME);
	Bind(wxEVT_MENU, &MainWindow::onLanguage, this, ID_LANGUAGE);
	Bind(wxEVT_MENU, &MainWindow::onLogLevel, this, ID_LOG_LEVEL);
	Bind(wxEVT_MENU, &MainWindow::onOpenDataFolder, this, ID_OPEN_DATA_FOLDER);
	Bind(wxEVT_MENU, &MainWindow::onVersionInfo, this, ID_VERSION_INFO);
	Bind(wxEVT_MENU, &MainWindow::onAbout, this, wxID_ABOUT);
}

void MainWindow::buildContent() {
	auto* panel = new wxPanel(this);
	auto* top = new wxBoxSizer(wxVERTICAL);

	auto* portRow = new wxBoxSizer(wxHORIZONTAL);
	portLed_ = new StatusLed(panel);
	portRow->Add(portLed_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	portRow->Add(new wxStaticText(panel, wxID_ANY, tr(UVT::PORT_LABEL)), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	portText_ = new wxStaticText(panel, wxID_ANY, wxEmptyString);
	portRow->Add(portText_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	portButton_ = new wxButton(panel, wxID_ANY, tr(UVT::OPEN_PORT_BTN));
	portButton_->Bind(wxEVT_BUTTON, &MainWindow::onTogglePort, this);
	portRow->Add(portButton_, 0);
	top->Add(portRow, 0, wxEXPAND | wxALL, 8);

	auto* sendRow = new wxBoxSizer(wxHORIZONTAL);
	sendRow->Add(new wxStaticText(panel, wxID_ANY, tr(UVT::SEND_HEX_LABEL)), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	sendText_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	sendText_->SetHint(tr(UVT::SEND_HEX_HINT));
	sendText_->Bind(wxEVT_TEXT_ENTER, &MainWindow::onSend, this);
	sendRow->Add(sendText_, 1, wxRIGHT, 6);
	sendButton_ = new wxButton(panel, wxID_ANY, tr(UVT::SEND_BTN));
	sendButton_->Bind(wxEVT_BUTTON, &MainWindow::onSend, this);
	sendRow->Add(sendButton_, 0);
	top->Add(sendRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	logText_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2);
	logText_->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	top->Add(logText_, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

	auto* clear = new wxButton(panel, wxID_ANY, tr(UVT::CLEAR_LOG_BTN));
	clear->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { logText_->Clear(); });
	top->Add(clear, 0, wxALIGN_RIGHT | wxALL, 8);

	panel->SetSizer(top);
}

// ------------------------------------------------------------------------------------------------
// Serial port
// ------------------------------------------------------------------------------------------------

void MainWindow::updatePortControls() {
	const bool open = link_.isOpen();
	portLed_->SetState(open ? StatusLed::State::Green : StatusLed::State::Off);
	portLed_->SetHint(open ? wxString::Format(tr(UVT::STATUSBAR_PORT_OPEN_FMT), wxString::FromUTF8(config_.describe())) : tr(UVT::STATUSBAR_PORT_CLOSED));
	portText_->SetLabel(config_.isValid() ? wxString::FromUTF8(config_.describe()) : tr(UVT::PORT_NOT_SET));
	portButton_->SetLabel(open ? tr(UVT::CLOSE_PORT_BTN) : tr(UVT::OPEN_PORT_BTN));
	portButton_->Enable(config_.isValid());
	sendButton_->Enable(open);
	SetStatusText(open ? wxString::Format(tr(UVT::STATUSBAR_PORT_OPEN_FMT), wxString::FromUTF8(config_.describe()))
		: tr(UVT::STATUSBAR_PORT_CLOSED), 1);
	Layout();
}

void MainWindow::openPort() {
	std::string error;
	if (!link_.open(config_, &error)) {
		wxMessageBox(wxString::Format(tr(UVT::PORT_OPEN_FAILED_FMT), wxString::FromUTF8(config_.port), wxString::FromUTF8(error)),
			tr(UVT::ERROR_TITLE), wxOK | wxICON_ERROR, this);
		return;
	}
	worker_->start();
	recordEvent("port", "opened " + config_.describe());
}

void MainWindow::closePort() {
	worker_->stop(); // aborts a transaction in progress, joins the thread
	link_.close();
	recordEvent("port", "closed " + config_.port);
}

void MainWindow::onTogglePort(wxCommandEvent&) {
	if (link_.isOpen()) closePort(); else openPort();
	updatePortControls();
}

void MainWindow::onSerialSettings(wxCommandEvent&) {
	SerialConfigDialog dlg(this, config_);
	if (dlg.ShowModal() != wxID_OK) return;
	const bool wasOpen = link_.isOpen();
	if (wasOpen) closePort();
	config_ = dlg.config();
	AppSettings::set("serialPort", config_.toString());
	if (wasOpen && config_.isValid()) openPort();
	updatePortControls();
}

void MainWindow::onSend(wxCommandEvent&) {
	if (!link_.isOpen()) {
		wxMessageBox(tr(UVT::SEND_NOT_OPEN), tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		return;
	}
	const auto parsed = Utils::Hex::parseBytes(sendText_->GetValue().utf8_string());
	if (!parsed || parsed->empty()) {
		wxMessageBox(tr(UVT::SEND_BAD_HEX), tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		return;
	}
	const std::vector<uint8_t>& bytes = *parsed;
	Log::info("TX: " + Utils::Hex::bytesToString(bytes));
	SerialWorker::Job job;
	job.request = bytes;
	job.timeoutMs = 1000;
	// No FrameComplete: the reply ends when the line goes quiet. A real protocol passes its framing
	// here, e.g. [](const auto& rx) { return rx.size() >= 3 ? size_t(4 + rx[2]) : size_t(0); }.
	job.done = [this](SerialLink::Result result, const std::vector<uint8_t>& reply) {
		// Runs on the GUI thread (see SerialWorker) - windows may be used directly here.
		const std::string text = std::string(SerialLink::resultName(result)) + (reply.empty() ? "" : ": " + Utils::Hex::bytesToString(reply));
		Log::info("RX " + text);
		recordEvent("rx", text);
		SetStatusText(wxString::Format(tr(UVT::REPLY_FMT), SerialLink::resultName(result), wxString::FromUTF8(Utils::Hex::bytesToString(reply))), 0);
	};
	worker_->submit(std::move(job));
	recordEvent("tx", Utils::Hex::bytesToString(bytes));
}

// ------------------------------------------------------------------------------------------------
// Database
// ------------------------------------------------------------------------------------------------

void MainWindow::recordEvent(const std::string& kind, const std::string& text) {
	if (!db_.isOpen()) return;
	Database::Statement ins(db_, "INSERT INTO events(kind, text) VALUES(?, ?)");
	ins.bind(1, kind).bind(2, text);
	ins.run();
}

// ------------------------------------------------------------------------------------------------
// Log view
// ------------------------------------------------------------------------------------------------

void MainWindow::onLogTimer(wxTimerEvent&) {
	const std::vector<std::string> lines = Log::takePendingLines();
	if (lines.empty()) return;
	wxString text;
	for (const std::string& line : lines) text << wxString::FromUTF8(line) << '\n';
	// Bounded (the full history is in the log file); keeps the view still while the user reads.
	Utils::Gui::appendToConsole(logText_, text, kMaxLogLines, true);
}

// ------------------------------------------------------------------------------------------------
// Theme / language / log level
// ------------------------------------------------------------------------------------------------

MainWindow::ApplyChoice MainWindow::askHowToApply(const wxString& title) {
	wxMessageDialog ask(this, tr(UVT::APPLY_CHOICE_MSG), title, wxYES_NO | wxCANCEL | wxHELP | wxICON_QUESTION);
	ask.SetYesNoCancelLabels(tr(UVT::RESTART_NOW_BTN), tr(UVT::USE_NOW_BTN), tr(UVT::CANCEL_BTN));
	ask.SetHelpLabel(tr(UVT::SAVE_FOR_NEXT_START_BTN));
	switch (ask.ShowModal()) {
	case wxID_YES:  return ApplyChoice::RestartNow;
	case wxID_NO:   return ApplyChoice::UseNow;
	case wxID_HELP: return ApplyChoice::NextStart;
	default:        return ApplyChoice::Cancel;
	}
}

bool MainWindow::restartApplication() {
	const wxString exe = wxStandardPaths::Get().GetExecutablePath();
	if (exe.empty() || wxExecute("\"" + exe + "\"", wxEXEC_ASYNC) == 0) return false;
	Close(true);
	return true;
}

void MainWindow::onTheme(wxCommandEvent&) {
	const wxString choices[] = { tr(UVT::THEME_LIGHT), tr(UVT::THEME_DARK) };
	const int picked = wxGetSingleChoiceIndex(tr(UVT::THEME_PROMPT), tr(UVT::THEME_TITLE), 2, choices, this, -1, -1, true, 150, 200,
		Theme::savedDark() ? 1 : 0);
	if (picked < 0) return;
	const bool dark = (picked == 1);
	const ApplyChoice how = askHowToApply(tr(UVT::THEME_TITLE));
	if (how == ApplyChoice::Cancel) return;
	Theme::save(dark);
	if (how == ApplyChoice::RestartNow) restartApplication();
	else if (how == ApplyChoice::UseNow && !Theme::switchNow(dark))
		wxMessageBox(tr(UVT::THEME_PARTIAL_MSG), tr(UVT::THEME_TITLE), wxOK | wxICON_INFORMATION, this);
}

void MainWindow::onLanguage(wxCommandEvent&) {
	const std::vector<I18n::Language> languages = I18n::available();
	wxArrayString names;
	int current = 0;
	for (size_t i = 0; i < languages.size(); ++i) {
		names.Add(languages[i].displayName);
		if (languages[i].code == I18n::current()) current = static_cast<int>(i);
	}
	wxSingleChoiceDialog dlg(this, tr(UVT::LANGUAGE_PROMPT), tr(UVT::LANGUAGE_TITLE), names);
	dlg.SetSelection(current);
	if (dlg.ShowModal() != wxID_OK) return;
	const std::string code = languages[static_cast<size_t>(dlg.GetSelection())].code;

	// Check the file now (then go back to the current language, so the question stays readable).
	const std::string previous = I18n::current();
	if (!I18n::load(code)) {
		I18n::load(previous);
		wxMessageBox(wxString::Format(tr(UVT::LANGUAGE_LOAD_FAILED_FMT), wxString::FromUTF8(code)), tr(UVT::ERROR_TITLE), wxOK | wxICON_ERROR, this);
		return;
	}
	I18n::load(previous);

	const ApplyChoice how = askHowToApply(tr(UVT::LANGUAGE_TITLE));
	if (how == ApplyChoice::Cancel) return;
	AppSettings::set("language", code);
	if (how == ApplyChoice::RestartNow) { restartApplication(); return; }
	if (how == ApplyChoice::UseNow) {
		// Menus and windows opened from now on use it; this window's other labels change at the next start.
		I18n::load(code);
		buildMenus();
	}
}

void MainWindow::onLogLevel(wxCommandEvent&) {
	wxArrayString names;
	for (int l = static_cast<int>(LogLevel::None); l <= static_cast<int>(LogLevel::Trace); ++l)
		names.Add(Log::levelName(static_cast<LogLevel>(l)));
	wxSingleChoiceDialog dlg(this, tr(UVT::LOG_LEVEL_PROMPT), tr(UVT::LOG_LEVEL_TITLE), names);
	dlg.SetSelection(static_cast<int>(Log::level()));
	if (dlg.ShowModal() != wxID_OK) return;
	Log::setLevel(static_cast<LogLevel>(dlg.GetSelection()));
	AppSettings::setInt("logLevel", dlg.GetSelection());
}

void MainWindow::onOpenDataFolder(wxCommandEvent&) {
	wxLaunchDefaultApplication(DataDir::wx());
}

// ------------------------------------------------------------------------------------------------
// Help
// ------------------------------------------------------------------------------------------------

void MainWindow::onVersionInfo(wxCommandEvent&) {
	wxString channel;
	switch (GetBuildChannel()) {
	case AppBuildChannel::Development:      channel = tr(UVT::BUILD_DEVELOPMENT); break;
	case AppBuildChannel::ReleaseCandidate: channel = tr(UVT::BUILD_RELEASE_CANDIDATE); break;
	default:                                channel = tr(UVT::BUILD_FINAL); break;
	}
	wxString text = wxString::Format(tr(UVT::VERSION_INFO_FMT), wxString::FromUTF8(GetAppVersion()), GetBuildNumber(),
		wxString::FromUTF8(GetBuildDate()), channel, wxString::FromUTF8(GetGitDescribe()), DataDir::wx());
	text << "\n\n" << tr(UVT::VERSION_HISTORY_HEADER) << "\n";
	for (const AppVersionHistoryEntry& e : kAppVersionHistory) {
		text << "\n" << e.version << "  (" << e.date << ")\n";
		std::istringstream notes(e.notes);
		std::string line;
		while (std::getline(notes, line)) text << "  - " << wxString::FromUTF8(line) << "\n";
	}
	wxDialog dlg(this, wxID_ANY, tr(UVT::VERSION_INFO_TITLE), wxDefaultPosition, wxSize(640, 480), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(new wxTextCtrl(&dlg, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY), 1, wxEXPAND | wxALL, 10);
	sizer->Add(dlg.CreateStdDialogButtonSizer(wxOK), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	dlg.SetSizer(sizer);
	dlg.ShowModal();
}

void MainWindow::onAbout(wxCommandEvent&) {
	const wxString name = wxString::FromUTF8(AppInfo::kName);
	wxMessageBox(wxString::Format(tr(UVT::ABOUT_BODY_FMT), name, wxString::FromUTF8(GetCompactVersion()),
		wxString::FromUTF8(AppInfo::kCopyright), wxString::FromUTF8(AppInfo::kLicense),
		wxString(wxVERSION_NUM_DOT_STRING), wxString::FromUTF8(sqlite3_libversion())),
		wxString::Format(tr(UVT::ABOUT_TITLE_FMT), name), wxOK | wxICON_INFORMATION, this);
}

void MainWindow::onClose(wxCloseEvent& event) {
	logTimer_.Stop();
	if (worker_) worker_->stop();
	link_.close();
	recordEvent("app", "closed");
	db_.close();
	event.Skip(); // default handling destroys the window
}
