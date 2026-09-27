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
#include "AutomationDialog.h"
#include "BuildInfo.h"
#include "DataDir.h"
#include "DataEntry.h"
#include "GuiUtils.h"
#include "HexUtils.h"
#include "I18n.h"
#include "Log.h"
#include "SerialConfigDialog.h"
#include "StatusLed.h"
#include "Theme.h"
#include "TimeUtils.h"
#include "UVT.h"
#include "Version.h"

#include <optional>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choicdlg.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statbox.h>
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
		ID_AUTO_REPLIES,
		ID_PERIODIC,
		ID_CLEAR_CONSOLE,
		ID_THEME,
		ID_LANGUAGE,
		ID_LOG_LEVEL,
		ID_OPEN_DATA_FOLDER,
		ID_VERSION_INFO,
	};

	// Console colours, readable on light and dark backgrounds.
	const wxColour kTimeColour(128, 128, 128);
	const wxColour kRxColour(0, 120, 215);
	const wxColour kTxColour(210, 100, 0);
	const wxColour kAutoReplyColour(160, 70, 210);
	const wxColour kPeriodicColour(0, 150, 80);
	const wxColour kErrorColour(220, 40, 40);

	/*! \brief A wxTimer that calls a function. */
	class FunctionTimer : public wxTimer {
	public:
		explicit FunctionTimer(std::function<void()> fn) : fn_(std::move(fn)) {}
		void Notify() override { fn_(); }
	private:
		std::function<void()> fn_;
	};
}

MainWindow::MainWindow()
	: wxFrame(nullptr, wxID_ANY, wxString::FromUTF8(GetWindowTitle()), wxDefaultPosition, wxSize(900, 600)),
	  logTimer_(this) {
	if (const auto saved = SerialConfig::fromString(AppSettings::getString("serialPort"))) config_ = *saved;
	loadAutomation();

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

	monitor_ = std::make_unique<SerialMonitor>(link_, this, [this](std::vector<SerialMonitor::Event>& events) { onSerialEvents(events); });
	Bind(wxEVT_TIMER, &MainWindow::onLogTimer, this, logTimer_.GetId());
	Bind(wxEVT_CLOSE_WINDOW, &MainWindow::onClose, this);
	logTimer_.Start(200);

	if (Theme::startupDark()) Theme::applyColours(this, true);
	Centre();
}

MainWindow::~MainWindow() {
	stopPeriodic();
	if (monitor_) monitor_->stop();
	link_.close();
}

void MainWindow::buildMenus() {
	auto* file = new wxMenu();
	file->Append(wxID_EXIT, tr(UVT::MENU_EXIT));

	auto* serialMenu = new wxMenu();
	serialMenu->Append(ID_SERIAL_SETTINGS, tr(UVT::MENU_SERIAL_PORT));
	serialMenu->AppendSeparator();
	serialMenu->Append(ID_AUTO_REPLIES, tr(UVT::MENU_AUTO_REPLIES));
	serialMenu->Append(ID_PERIODIC, tr(UVT::MENU_PERIODIC));
	serialMenu->AppendSeparator();
	serialMenu->Append(ID_CLEAR_CONSOLE, tr(UVT::MENU_CLEAR_CONSOLE));

	auto* settings = new wxMenu();
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
	bar->Append(serialMenu, tr(UVT::MENU_SERIAL));
	bar->Append(settings, tr(UVT::MENU_SETTINGS));
	bar->Append(help, tr(UVT::MENU_HELP));
	SetMenuBar(bar);

	Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(); }, wxID_EXIT);
	Bind(wxEVT_MENU, &MainWindow::onSerialSettings, this, ID_SERIAL_SETTINGS);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { onEditAutomation(false); }, ID_AUTO_REPLIES);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { onEditAutomation(true); }, ID_PERIODIC);
	Bind(wxEVT_MENU, [this](wxCommandEvent&) { console_->Clear(); }, ID_CLEAR_CONSOLE);
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

	// Port: LED, settings, open / close.
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

	// Send: notation, text, character table, Send.
	auto* sendBox = new wxStaticBoxSizer(wxHORIZONTAL, panel, tr(UVT::SEND_GROUP_LABEL));
	const auto savedFormat = SerialData::formatFromKey(AppSettings::getString("send.format", "hex"));
	sendEntry_ = new DataEntry(sendBox->GetStaticBox(), savedFormat ? *savedFormat : SerialData::DataFormat::Hex,
		wxString::FromUTF8(AppSettings::getString("send.text")), true);
	sendEntry_->SetOnEnter([this] { wxCommandEvent e; onSend(e); });
	sendBox->Add(sendEntry_, 1, wxEXPAND | wxALL, 4);
	sendButton_ = new wxButton(sendBox->GetStaticBox(), wxID_ANY, tr(UVT::SEND_BTN));
	sendButton_->Bind(wxEVT_BUTTON, &MainWindow::onSend, this);
	sendBox->Add(sendButton_, 0, wxALIGN_BOTTOM | wxALL, 4);
	top->Add(sendBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	// Automation: a switch, a summary and Edit... for each list.
	auto* autoRow = new wxBoxSizer(wxHORIZONTAL);
	auto automation = [&](const wxString& label, const char* key, wxCheckBox*& check, wxStaticText*& info, bool periodicPage) {
		check = new wxCheckBox(panel, wxID_ANY, label);
		check->SetValue(AppSettings::getBool(key, true));
		check->Bind(wxEVT_CHECKBOX, [this, key](wxCommandEvent& e) {
			AppSettings::setBool(key, e.IsChecked());
			applyAutomation();
		});
		info = new wxStaticText(panel, wxID_ANY, wxEmptyString);
		auto* edit = new wxButton(panel, wxID_ANY, tr(UVT::EDIT_BTN), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
		edit->Bind(wxEVT_BUTTON, [this, periodicPage](wxCommandEvent&) { onEditAutomation(periodicPage); });
		autoRow->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
		autoRow->Add(info, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
		autoRow->Add(edit, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 24);
	};
	automation(tr(UVT::AUTO_REPLIES_SWITCH), "automation.autoReplies", autoRepliesOn_, autoRepliesInfo_, false);
	automation(tr(UVT::PERIODIC_SWITCH), "automation.periodic", periodicOn_, periodicInfo_, true);
	top->Add(autoRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	// Serial console above, application log below.
	auto* splitter = new wxSplitterWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3DSASH);
	splitter->SetMinimumPaneSize(FromDIP(60));
	splitter->SetSashGravity(0.7);
	const wxFont mono(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

	auto* consolePanel = new wxPanel(splitter);
	auto* consoleSizer = new wxBoxSizer(wxVERTICAL);
	auto* consoleBar = new wxBoxSizer(wxHORIZONTAL);
	consoleBar->Add(new wxStaticText(consolePanel, wxID_ANY, tr(UVT::CONSOLE_LABEL)), 0, wxALIGN_CENTER_VERTICAL);
	consoleBar->AddStretchSpacer();
	auto option = [&](const wxString& label, const char* key, bool fallback) {
		auto* c = new wxCheckBox(consolePanel, wxID_ANY, label);
		c->SetValue(AppSettings::getBool(key, fallback));
		c->Bind(wxEVT_CHECKBOX, [key](wxCommandEvent& e) { AppSettings::setBool(key, e.IsChecked()); });
		consoleBar->Add(c, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
		return c;
	};
	consoleTime_ = option(tr(UVT::CONSOLE_TIMESTAMPS), "console.time", true);
	consoleHex_ = option(tr(UVT::CONSOLE_SHOW_HEX), "console.hex", true);
	consoleFollow_ = option(tr(UVT::CONSOLE_FOLLOW), "console.follow", true);
	auto* clearConsole = new wxButton(consolePanel, wxID_ANY, tr(UVT::CLEAR_BTN), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	clearConsole->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { console_->Clear(); });
	consoleBar->Add(clearConsole, 0, wxALIGN_CENTER_VERTICAL);
	consoleSizer->Add(consoleBar, 0, wxEXPAND | wxBOTTOM, 2);
	console_ = new wxTextCtrl(consolePanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2);
	console_->SetFont(mono);
	consoleSizer->Add(console_, 1, wxEXPAND);
	consolePanel->SetSizer(consoleSizer);

	auto* logPanel = new wxPanel(splitter);
	auto* logSizer = new wxBoxSizer(wxVERTICAL);
	auto* logBar = new wxBoxSizer(wxHORIZONTAL);
	logBar->Add(new wxStaticText(logPanel, wxID_ANY, tr(UVT::LOG_VIEW_LABEL)), 0, wxALIGN_CENTER_VERTICAL);
	logBar->AddStretchSpacer();
	auto* clear = new wxButton(logPanel, wxID_ANY, tr(UVT::CLEAR_LOG_BTN), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	clear->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { logText_->Clear(); });
	logBar->Add(clear, 0, wxALIGN_CENTER_VERTICAL);
	logSizer->Add(logBar, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
	logText_ = new wxTextCtrl(logPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxTE_RICH2);
	logText_->SetFont(mono);
	logSizer->Add(logText_, 1, wxEXPAND);
	logPanel->SetSizer(logSizer);

	splitter->SplitHorizontally(consolePanel, logPanel, -FromDIP(140));
	top->Add(splitter, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	panel->SetSizer(top);
	updateAutomationSummary();
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
	sendEntry_->Enable(open);
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
	// A received message ends when the line is quiet for the inter-byte timeout (at least 5 ms).
	monitor_->start((std::max)(static_cast<int>(config_.interByteTimeoutMs), 5));
	applyAutomation();
	recordEvent("port", "opened " + config_.describe());
}

void MainWindow::closePort() {
	stopPeriodic();
	monitor_->stop(); // joins the thread; queued messages are dropped
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
	if (!monitor_->isRunning()) {
		wxMessageBox(tr(UVT::SEND_NOT_OPEN), tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		return;
	}
	std::vector<uint8_t> bytes;
	if (!sendEntry_->GetBytes(bytes) || bytes.empty()) return;
	if (!monitor_->send(bytes)) printTraffic(Utils::Time::nowEpochMs(), tr(UVT::CONSOLE_TX), kErrorColour, bytes, tr(UVT::CONSOLE_QUEUE_FULL));
}

// ------------------------------------------------------------------------------------------------
// Serial console
// ------------------------------------------------------------------------------------------------

void MainWindow::printTraffic(uint64_t epochMs, const wxString& label, const wxColour& colour, const std::vector<uint8_t>& bytes, const wxString& note) {
	std::vector<Utils::Gui::ColouredText> pieces;
	if (consoleTime_->GetValue()) pieces.push_back({ wxString::FromUTF8(Utils::Time::toString(epochMs).substr(11)) + "  ", kTimeColour });
	pieces.push_back({ wxString::Format("%s (%d)  ", label, static_cast<int>(bytes.size())), colour });
	pieces.push_back({ wxString::FromUTF8(SerialData::display(bytes)), wxColour() });
	if (consoleHex_->GetValue() && !bytes.empty()) pieces.push_back({ "   | " + wxString::FromUTF8(Utils::Hex::bytesToString(bytes)), kTimeColour });
	if (!note.empty()) pieces.push_back({ "   " + note, kErrorColour });
	pieces.push_back({ "\n", wxColour() });
	Utils::Gui::appendToConsole(console_, pieces, kMaxConsoleLines, consoleFollow_->GetValue());
}

void MainWindow::onSerialEvents(std::vector<SerialMonitor::Event>& events) {
	using Kind = SerialMonitor::Kind;
	std::optional<Database::Transaction> transaction; // one write for the whole batch
	if (db_.isOpen()) transaction.emplace(db_);
	for (const SerialMonitor::Event& e : events) {
		switch (e.kind) {
		case Kind::Received:
			printTraffic(e.epochMs, tr(UVT::CONSOLE_RX), kRxColour, e.bytes);
			recordEvent("rx", Utils::Hex::bytesToString(e.bytes));
			break;
		case Kind::Sent:
			printTraffic(e.epochMs, tr(UVT::CONSOLE_TX), kTxColour, e.bytes);
			recordEvent("tx", Utils::Hex::bytesToString(e.bytes));
			break;
		case Kind::AutoReply:
			printTraffic(e.epochMs, wxString::Format(tr(UVT::CONSOLE_AUTO_REPLY_FMT), wxString::FromUTF8(e.source)), kAutoReplyColour, e.bytes);
			recordEvent("tx-auto", Utils::Hex::bytesToString(e.bytes));
			break;
		case Kind::Periodic:
			printTraffic(e.epochMs, wxString::Format(tr(UVT::CONSOLE_PERIODIC_FMT), wxString::FromUTF8(e.source)), kPeriodicColour, e.bytes);
			recordEvent("tx-periodic", Utils::Hex::bytesToString(e.bytes));
			break;
		case Kind::Error:
			if (e.bytes.empty()) // an auto reply that could not be built
				printTraffic(e.epochMs, tr(UVT::CONSOLE_TX), kErrorColour, e.bytes, wxString::Format(tr(UVT::CONSOLE_REPLY_FAILED_FMT), wxString::FromUTF8(e.source)));
			else
				printTraffic(e.epochMs, tr(UVT::CONSOLE_TX), kErrorColour, e.bytes, wxString::Format(tr(UVT::CONSOLE_WRITE_FAILED_FMT), SerialLink::resultName(e.result)));
			break;
		}
	}
	if (transaction) transaction->commit();
}

// ------------------------------------------------------------------------------------------------
// Auto replies and periodic messages
// ------------------------------------------------------------------------------------------------

void MainWindow::loadAutomation() {
	autoReplies_.clear();
	periodic_.clear();
	const int replies = AppSettings::getInt("autoReply.count", 0);
	for (int i = 0; i < replies; ++i)
		if (auto r = SerialData::autoReplyFromString(AppSettings::getString("autoReply." + std::to_string(i)))) autoReplies_.push_back(*r);
	const int periodic = AppSettings::getInt("periodic.count", 0);
	for (int i = 0; i < periodic; ++i)
		if (auto p = SerialData::periodicFromString(AppSettings::getString("periodic." + std::to_string(i)))) periodic_.push_back(*p);
}

void MainWindow::saveAutomation() {
	AppSettings::setInt("autoReply.count", static_cast<int>(autoReplies_.size()));
	for (size_t i = 0; i < autoReplies_.size(); ++i) AppSettings::set("autoReply." + std::to_string(i), SerialData::toString(autoReplies_[i]));
	AppSettings::setInt("periodic.count", static_cast<int>(periodic_.size()));
	for (size_t i = 0; i < periodic_.size(); ++i) AppSettings::set("periodic." + std::to_string(i), SerialData::toString(periodic_[i]));
}

void MainWindow::updateAutomationSummary() {
	auto summary = [](wxStaticText* info, auto const& list) {
		int enabled = 0;
		for (const auto& item : list) enabled += item.enabled ? 1 : 0;
		info->SetLabel(wxString::Format(tr(UVT::AUTOMATION_COUNT_FMT), enabled, static_cast<int>(list.size())));
	};
	summary(autoRepliesInfo_, autoReplies_);
	summary(periodicInfo_, periodic_);
	Layout();
}

void MainWindow::onEditAutomation(bool periodicPage) {
	AutomationDialog dlg(this, autoReplies_, periodic_, periodicPage ? AutomationDialog::Page::Periodic : AutomationDialog::Page::AutoReplies);
	if (dlg.ShowModal() != wxID_OK) return;
	autoReplies_ = dlg.autoReplies();
	periodic_ = dlg.periodic();
	saveAutomation();
	updateAutomationSummary();
	applyAutomation();
}

void MainWindow::stopPeriodic() {
	for (auto& run : periodicRuns_) run->timer->Stop();
	periodicRuns_.clear();
}

void MainWindow::applyAutomation() {
	if (!monitor_->isRunning()) return; // applied when the port opens
	monitor_->setAutoReplies(autoRepliesOn_->GetValue() ? autoReplies_ : std::vector<SerialData::AutoReplyRule>());
	stopPeriodic();
	if (!periodicOn_->GetValue()) return;
	const uint64_t now = Utils::Time::nowMonotonicMs();
	for (const SerialData::PeriodicMessage& p : periodic_) {
		if (!p.enabled) continue;
		auto run = std::make_unique<PeriodicRun>();
		run->name = p.name;
		run->generator = SerialData::MessageGenerator(p.message);
		run->generator.restart(now);
		PeriodicRun* raw = run.get();
		run->timer = std::make_unique<FunctionTimer>([this, raw] { sendPeriodic(*raw); });
		run->timer->Start((std::max)(p.periodMs, 10));
		periodicRuns_.push_back(std::move(run));
	}
}

void MainWindow::sendPeriodic(PeriodicRun& run) {
	const SerialData::BuildResult r = run.generator.next(Utils::Time::nowMonotonicMs());
	if (!r.ok() || r.bytes.empty()) {
		run.timer->Stop(); // would fail every time: report once
		printTraffic(Utils::Time::nowEpochMs(), tr(UVT::CONSOLE_TX), kErrorColour, {}, wxString::Format(tr(UVT::CONSOLE_PERIODIC_FAILED_FMT), wxString::FromUTF8(run.name)));
		return;
	}
	if (!monitor_->send(r.bytes, SerialMonitor::Kind::Periodic, run.name))
		printTraffic(Utils::Time::nowEpochMs(), tr(UVT::CONSOLE_TX), kErrorColour, r.bytes, tr(UVT::CONSOLE_QUEUE_FULL));
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
	stopPeriodic();
	if (monitor_) monitor_->stop();
	link_.close();
	AppSettings::set("send.format", SerialData::formatKey(sendEntry_->GetFormat()));
	AppSettings::set("send.text", sendEntry_->GetText().utf8_string());
	recordEvent("app", "closed");
	db_.close();
	event.Skip(); // default handling destroys the window
}
