// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <wx/string.h>

/*!
 * \file UVT.h
 * \brief User-Visible Texts: every string the user can see, in English, in one place.
 *
 * Always show them through tr() (I18n.h): tr(UVT::MENU_FILE). The English text is the translation
 * key in languages/language-<code>.xml, so changing a text here needs its translations updated too
 * (a missing one just shows the English text). Format strings (%s, %d) must keep the same
 * placeholders in the same order in every translation.
 *
 * Naming: avoid prefixes the Windows headers use for macros (PARITY_, STATUS_, ERROR_, MB_, WM_,
 * IDOK...) - a macro silently replaces a constant of the same name and the build fails far from the
 * cause (e.g. winbase.h defines PARITY_NONE). Suffix with _LABEL/_BTN/_TITLE/_MSG instead.
 */
namespace UVT {
	// --- Menus ---
	inline const wxString MENU_FILE = "&File";
	inline const wxString MENU_EXIT = "E&xit\tAlt-X";
	inline const wxString MENU_SETTINGS = "&Settings";
	inline const wxString MENU_SERIAL_PORT = "&Serial Port...";
	inline const wxString MENU_THEME = "&Theme...";
	inline const wxString MENU_LANGUAGE = "&Language...";
	inline const wxString MENU_LOG_LEVEL = "Log &Level...";
	inline const wxString MENU_OPEN_DATA_FOLDER = "Open &Data Folder";
	inline const wxString MENU_HELP = "&Help";
	inline const wxString MENU_VERSION_INFO = "&Version Info...";
	inline const wxString MENU_ABOUT = "&About...";

	// --- Main window ---
	inline const wxString PORT_LABEL = "Port:";
	inline const wxString PORT_NOT_SET = "not set";
	inline const wxString OPEN_PORT_BTN = "Open Port";
	inline const wxString CLOSE_PORT_BTN = "Close Port";
	inline const wxString SEND_HEX_LABEL = "Send (hex):";
	inline const wxString SEND_BTN = "Send";
	inline const wxString SEND_HEX_HINT = "e.g. 01 03 00 00 00 01";
	inline const wxString CLEAR_LOG_BTN = "Clear Log";
	inline const wxString STATUSBAR_READY = "Ready";
	inline const wxString STATUSBAR_PORT_OPEN_FMT = "Port open: %s";
	inline const wxString STATUSBAR_PORT_CLOSED = "Port closed";
	inline const wxString PORT_OPEN_FAILED_FMT = "Cannot open %s:\n%s";
	inline const wxString SEND_BAD_HEX = "Write the bytes to send as hexadecimal pairs, e.g. 01 03 00 00.";
	inline const wxString SEND_NOT_OPEN = "Open the port first.";
	inline const wxString REPLY_FMT = "Reply (%s): %s";
	inline const wxString ERROR_TITLE = "Error";

	// --- Serial port dialog ---
	inline const wxString SERIAL_DIALOG_TITLE = "Serial Port";
	inline const wxString SERIAL_PORT = "Port";
	inline const wxString SERIAL_BAUDRATE = "Baud rate";
	inline const wxString SERIAL_DATA_BITS = "Data bits";
	inline const wxString SERIAL_PARITY = "Parity";
	inline const wxString SERIAL_STOP_BITS = "Stop bits";
	inline const wxString SERIAL_FLOW_CONTROL = "Flow control";
	inline const wxString SERIAL_INTER_BYTE_TIMEOUT = "Inter-byte timeout (ms)";
	inline const wxString SERIAL_READ_TIMEOUT = "Read timeout (ms)";
	inline const wxString SERIAL_WRITE_TIMEOUT = "Write timeout (ms)";
	inline const wxString SERIAL_REFRESH_PORTS = "Refresh";
	inline const wxString PARITY_NONE_LABEL = "None";
	inline const wxString PARITY_ODD_LABEL = "Odd";
	inline const wxString PARITY_EVEN_LABEL = "Even";
	inline const wxString PARITY_MARK_LABEL = "Mark";
	inline const wxString PARITY_SPACE_LABEL = "Space";
	inline const wxString FLOW_NONE = "None";
	inline const wxString FLOW_SOFTWARE = "Software (XON/XOFF)";
	inline const wxString FLOW_HARDWARE = "Hardware (RTS/CTS)";

	// --- Theme / language ---
	inline const wxString THEME_TITLE = "Theme";
	inline const wxString THEME_PROMPT = "Select the theme:";
	inline const wxString THEME_LIGHT = "Light";
	inline const wxString THEME_DARK = "Dark";
	inline const wxString LANGUAGE_TITLE = "Language";
	inline const wxString LANGUAGE_PROMPT = "Select the language:";
	inline const wxString APPLY_CHOICE_MSG =
		"The change is applied completely only when the application starts.\n\n"
		"Restart now: save it and restart the application automatically.\n"
		"Use now: save it and use it right away - some parts change only at the next start.\n"
		"Save for next start: keep working; it applies the next time the application is launched.\n"
		"Cancel: discard the selection.";
	inline const wxString RESTART_NOW_BTN = "Restart now";
	inline const wxString USE_NOW_BTN = "Use now";
	inline const wxString SAVE_FOR_NEXT_START_BTN = "Save for next start";
	inline const wxString CANCEL_BTN = "Cancel";
	inline const wxString THEME_PARTIAL_MSG =
		"The new theme is used for the application's colours, including the windows opened from now on. "
		"Parts drawn by the system (title bars, scroll bars) change at the next start.";
	inline const wxString LANGUAGE_LOAD_FAILED_FMT = "The language file for \"%s\" could not be loaded - the current language is kept.";

	// --- Log level ---
	inline const wxString LOG_LEVEL_TITLE = "Log Level";
	inline const wxString LOG_LEVEL_PROMPT = "Write to the log messages up to:";

	// --- About / version ---
	inline const wxString ABOUT_TITLE_FMT = "About %s";
	inline const wxString ABOUT_BODY_FMT =
		"%s\nVersion %s\n\n%s\nLicensed under the %s.\n\n"
		"Built with wxWidgets %s, SQLite %s and serial (wjwwood/serial, CogaF fork).";
	inline const wxString VERSION_INFO_TITLE = "Version Information";
	inline const wxString VERSION_INFO_FMT =
		"Version: %s\nBuild: %d (%s)\nBuild type: %s\nSource (git): %s\nData folder: %s";
	inline const wxString BUILD_DEVELOPMENT = "Development (Debug)";
	inline const wxString BUILD_RELEASE_CANDIDATE = "Release candidate";
	inline const wxString BUILD_FINAL = "Final release";
	inline const wxString VERSION_HISTORY_HEADER = "Version history (newest first):";
}
