/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <wx/string.h>

/*!
 * \file UVT.h
 * \brief User-Visible Texts: every string the user can see, in English, in one place.
 *
 * Always show them through tr() (I18n.h): tr(UVT::MENU_FILE). The English text is the translation
 * key in languages/language-\<code\>.xml, so changing a text here needs its translations updated too
 * (a missing one just shows the English text). Format strings (%s, %d) must keep the same
 * placeholders in the same order in every translation.
 *
 * Naming: avoid prefixes the Windows headers use for macros (PARITY_, STATUS_, ERROR_, MB_, WM_,
 * IDOK...) - a macro silently replaces a constant of the same name and the build fails far from the
 * cause (e.g. winbase.h defines PARITY_NONE). Suffix with _LABEL/_BTN/_TITLE/_MSG instead.
 */
namespace UVT {
	// --- Menus ---
	/*! \brief Menu item: file. */
	inline const wxString MENU_FILE = "&File";
	/*! \brief Menu item: exit. */
	inline const wxString MENU_EXIT = "E&xit\tAlt-X";
	/*! \brief Menu item: settings. */
	inline const wxString MENU_SETTINGS = "&Settings";
	/*! \brief Menu item: serial port. */
	inline const wxString MENU_SERIAL_PORT = "&Serial Port...";
	/*! \brief Menu item: theme. */
	inline const wxString MENU_THEME = "&Theme...";
	/*! \brief Menu item: language. */
	inline const wxString MENU_LANGUAGE = "&Language...";
	/*! \brief Menu item: log level. */
	inline const wxString MENU_LOG_LEVEL = "Log &Level...";
	/*! \brief Menu item: open data folder. */
	inline const wxString MENU_OPEN_DATA_FOLDER = "Open &Data Folder";
	/*! \brief Menu item: help. */
	inline const wxString MENU_HELP = "&Help";
	/*! \brief Menu item: version info. */
	inline const wxString MENU_VERSION_INFO = "&Version Info...";
	/*! \brief Menu item: about. */
	inline const wxString MENU_ABOUT = "&About...";

	// --- Main window ---
	/*! \brief Label: port. */
	inline const wxString PORT_LABEL = "Port:";
	/*! \brief Text: port not set. */
	inline const wxString PORT_NOT_SET = "not set";
	/*! \brief Button: open port. */
	inline const wxString OPEN_PORT_BTN = "Open Port";
	/*! \brief Button: close port. */
	inline const wxString CLOSE_PORT_BTN = "Close Port";
	/*! \brief Label: send hex. */
	inline const wxString SEND_HEX_LABEL = "Send (hex):";
	/*! \brief Button: send. */
	inline const wxString SEND_BTN = "Send";
	/*! \brief Hint: send hex. */
	inline const wxString SEND_HEX_HINT = "e.g. 01 03 00 00 00 01";
	/*! \brief Button: clear log. */
	inline const wxString CLEAR_LOG_BTN = "Clear Log";
	/*! \brief Status bar text: ready. */
	inline const wxString STATUSBAR_READY = "Ready";
	/*! \brief Format string: statusbar port open. */
	inline const wxString STATUSBAR_PORT_OPEN_FMT = "Port open: %s";
	/*! \brief Status bar text: port closed. */
	inline const wxString STATUSBAR_PORT_CLOSED = "Port closed";
	/*! \brief Format string: port open failed. */
	inline const wxString PORT_OPEN_FAILED_FMT = "Cannot open %s:\n%s";
	/*! \brief Text: send bad hex. */
	inline const wxString SEND_BAD_HEX = "Write the bytes to send as hexadecimal pairs, e.g. 01 03 00 00.";
	/*! \brief Text: send not open. */
	inline const wxString SEND_NOT_OPEN = "Open the port first.";
	/*! \brief Format string: reply. */
	inline const wxString REPLY_FMT = "Reply (%s): %s";
	/*! \brief Window title: error. */
	inline const wxString ERROR_TITLE = "Error";

	// --- Serial port dialog ---
	/*! \brief Window title: serial dialog. */
	inline const wxString SERIAL_DIALOG_TITLE = "Serial Port";
	/*! \brief Text: serial port. */
	inline const wxString SERIAL_PORT = "Port";
	/*! \brief Text: serial baudrate. */
	inline const wxString SERIAL_BAUDRATE = "Baud rate";
	/*! \brief Text: serial data bits. */
	inline const wxString SERIAL_DATA_BITS = "Data bits";
	/*! \brief Text: serial parity. */
	inline const wxString SERIAL_PARITY = "Parity";
	/*! \brief Text: serial stop bits. */
	inline const wxString SERIAL_STOP_BITS = "Stop bits";
	/*! \brief Text: serial flow control. */
	inline const wxString SERIAL_FLOW_CONTROL = "Flow control";
	/*! \brief Text: serial inter byte timeout. */
	inline const wxString SERIAL_INTER_BYTE_TIMEOUT = "Inter-byte timeout (ms)";
	/*! \brief Text: serial read timeout. */
	inline const wxString SERIAL_READ_TIMEOUT = "Read timeout (ms)";
	/*! \brief Text: serial write timeout. */
	inline const wxString SERIAL_WRITE_TIMEOUT = "Write timeout (ms)";
	/*! \brief Text: serial refresh ports. */
	inline const wxString SERIAL_REFRESH_PORTS = "Refresh";
	/*! \brief Label: parity none. */
	inline const wxString PARITY_NONE_LABEL = "None";
	/*! \brief Label: parity odd. */
	inline const wxString PARITY_ODD_LABEL = "Odd";
	/*! \brief Label: parity even. */
	inline const wxString PARITY_EVEN_LABEL = "Even";
	/*! \brief Label: parity mark. */
	inline const wxString PARITY_MARK_LABEL = "Mark";
	/*! \brief Label: parity space. */
	inline const wxString PARITY_SPACE_LABEL = "Space";
	/*! \brief Text: flow none. */
	inline const wxString FLOW_NONE = "None";
	/*! \brief Text: flow software. */
	inline const wxString FLOW_SOFTWARE = "Software (XON/XOFF)";
	/*! \brief Text: flow hardware. */
	inline const wxString FLOW_HARDWARE = "Hardware (RTS/CTS)";

	// --- Theme / language ---
	/*! \brief Window title: theme. */
	inline const wxString THEME_TITLE = "Theme";
	/*! \brief Prompt: theme. */
	inline const wxString THEME_PROMPT = "Select the theme:";
	/*! \brief Text: theme light. */
	inline const wxString THEME_LIGHT = "Light";
	/*! \brief Text: theme dark. */
	inline const wxString THEME_DARK = "Dark";
	/*! \brief Window title: language. */
	inline const wxString LANGUAGE_TITLE = "Language";
	/*! \brief Prompt: language. */
	inline const wxString LANGUAGE_PROMPT = "Select the language:";
	/*! \brief Message: apply choice. */
	inline const wxString APPLY_CHOICE_MSG =
		"The change is applied completely only when the application starts.\n\n"
		"Restart now: save it and restart the application automatically.\n"
		"Use now: save it and use it right away - some parts change only at the next start.\n"
		"Save for next start: keep working; it applies the next time the application is launched.\n"
		"Cancel: discard the selection.";
	/*! \brief Button: restart now. */
	inline const wxString RESTART_NOW_BTN = "Restart now";
	/*! \brief Button: use now. */
	inline const wxString USE_NOW_BTN = "Use now";
	/*! \brief Button: save for next start. */
	inline const wxString SAVE_FOR_NEXT_START_BTN = "Save for next start";
	/*! \brief Button: cancel. */
	inline const wxString CANCEL_BTN = "Cancel";
	/*! \brief Message: theme partial. */
	inline const wxString THEME_PARTIAL_MSG =
		"The new theme is used for the application's colours, including the windows opened from now on. "
		"Parts drawn by the system (title bars, scroll bars) change at the next start.";
	/*! \brief Format string: language load failed. */
	inline const wxString LANGUAGE_LOAD_FAILED_FMT = "The language file for \"%s\" could not be loaded - the current language is kept.";

	// --- Log level ---
	/*! \brief Window title: log level. */
	inline const wxString LOG_LEVEL_TITLE = "Log Level";
	/*! \brief Prompt: log level. */
	inline const wxString LOG_LEVEL_PROMPT = "Write to the log messages up to:";

	// --- About / version ---
	/*! \brief Format string: about title. */
	inline const wxString ABOUT_TITLE_FMT = "About %s";
	/*! \brief Format string: about body. */
	inline const wxString ABOUT_BODY_FMT =
		"%s\nVersion %s\n\n%s\nLicensed under the %s.\n\n"
		"Built with wxWidgets %s, SQLite %s and serial (wjwwood/serial, CogaF fork).";
	/*! \brief Window title: version info. */
	inline const wxString VERSION_INFO_TITLE = "Version Information";
	/*! \brief Format string: version info. */
	inline const wxString VERSION_INFO_FMT =
		"Version: %s\nBuild: %d (%s)\nBuild type: %s\nSource (git): %s\nData folder: %s";
	/*! \brief Text: build development. */
	inline const wxString BUILD_DEVELOPMENT = "Development (Debug)";
	/*! \brief Text: build release candidate. */
	inline const wxString BUILD_RELEASE_CANDIDATE = "Release candidate";
	/*! \brief Text: build final. */
	inline const wxString BUILD_FINAL = "Final release";
	/*! \brief Text: version history header. */
	inline const wxString VERSION_HISTORY_HEADER = "Version history (newest first):";
}
