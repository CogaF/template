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
	/*! \brief Button: send. */
	inline const wxString SEND_BTN = "Send";
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
	/*! \brief Text: send not open. */
	inline const wxString SEND_NOT_OPEN = "Open the port first.";
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

	// --- Serial menu ---
	/*! \brief Menu: serial. */
	inline const wxString MENU_SERIAL = "Se&rial";
	/*! \brief Menu item: auto replies. */
	inline const wxString MENU_AUTO_REPLIES = "&Auto Replies...";
	/*! \brief Menu item: periodic messages. */
	inline const wxString MENU_PERIODIC = "&Periodic Messages...";
	/*! \brief Menu item: clear the serial console. */
	inline const wxString MENU_CLEAR_CONSOLE = "&Clear Console";

	// --- Main window: send box, automation switches, console ---
	/*! \brief Group box: send. */
	inline const wxString SEND_GROUP_LABEL = "Send";
	/*! \brief Check box: auto replies on/off. */
	inline const wxString AUTO_REPLIES_SWITCH = "Auto replies";
	/*! \brief Check box: periodic messages on/off. */
	inline const wxString PERIODIC_SWITCH = "Periodic messages";
	/*! \brief Format string: how many items are enabled. */
	inline const wxString AUTOMATION_COUNT_FMT = "(%d of %d enabled)";
	/*! \brief Label: serial console. */
	inline const wxString CONSOLE_LABEL = "Serial console (everything received and sent)";
	/*! \brief Check box: show times in the console. */
	inline const wxString CONSOLE_TIMESTAMPS = "Time";
	/*! \brief Check box: show hex bytes in the console. */
	inline const wxString CONSOLE_SHOW_HEX = "Hex too";
	/*! \brief Check box: keep the console on the newest line. */
	inline const wxString CONSOLE_FOLLOW = "Follow newest";
	/*! \brief Button: clear. */
	inline const wxString CLEAR_BTN = "Clear";
	/*! \brief Label: application log. */
	inline const wxString LOG_VIEW_LABEL = "Application log";
	/*! \brief Console: received. */
	inline const wxString CONSOLE_RX = "RX";
	/*! \brief Console: sent. */
	inline const wxString CONSOLE_TX = "TX";
	/*! \brief Console: sent by an auto reply (its name). */
	inline const wxString CONSOLE_AUTO_REPLY_FMT = "TX auto \"%s\"";
	/*! \brief Console: sent by a periodic message (its name). */
	inline const wxString CONSOLE_PERIODIC_FMT = "TX periodic \"%s\"";
	/*! \brief Console: a write failed (the reason). */
	inline const wxString CONSOLE_WRITE_FAILED_FMT = "Write failed (%s).";
	/*! \brief Console: an auto reply could not be built. */
	inline const wxString CONSOLE_REPLY_FAILED_FMT = "Auto reply \"%s\" cannot be built - check its text.";
	/*! \brief Console: a periodic message could not be built. */
	inline const wxString CONSOLE_PERIODIC_FAILED_FMT = "Periodic message \"%s\" cannot be built - check its text. It has been stopped.";
	/*! \brief Console: too many messages waiting. */
	inline const wxString CONSOLE_QUEUE_FULL = "Too many messages waiting - this one was not sent.";

	// --- Data entry (Hex / ASCII / Mixed) ---
	/*! \brief Radio button: Hex notation. */
	inline const wxString FORMAT_HEX_LABEL = "Hex";
	/*! \brief Radio button: ASCII notation. */
	inline const wxString FORMAT_ASCII_LABEL = "ASCII";
	/*! \brief Radio button: Mixed notation. */
	inline const wxString FORMAT_MIXED_LABEL = "Mixed";
	/*! \brief Button: open the character table. */
	inline const wxString INSERT_CHAR_BTN = "Insert character...";
	/*! \brief Hint: Hex notation. */
	inline const wxString HINT_HEX = "Hex bytes, e.g. 01 03 00 0A - only 0-9 and A-F; the spaces between bytes are added for you.";
	/*! \brief Hint: ASCII notation. */
	inline const wxString HINT_ASCII = "Printable ASCII characters only (space to ~). For control bytes use Mixed.";
	/*! \brief Hint: Mixed notation. */
	inline const wxString HINT_MIXED = "Text, and //0x with two hex digits for any byte: OK//0x0D//0x0A (one //0x for each byte).";
	/*! \brief Format string: number of bytes. */
	inline const wxString BYTE_COUNT_FMT = "%d byte(s).";
	/*! \brief Format string: where the text is wrong and why. */
	inline const wxString PARSE_ERROR_AT_FMT = "Character %d: %s";
	/*! \brief Error: not a hex digit. */
	inline const wxString PARSE_NOT_HEX_MSG = "only hex digits (0-9, A-F) are allowed.";
	/*! \brief Error: a byte with one hex digit. */
	inline const wxString PARSE_ODD_HEX_MSG = "this byte has only one hex digit - a byte needs two.";
	/*! \brief Error: not printable ASCII. */
	inline const wxString PARSE_NOT_ASCII_MSG = "not a printable ASCII character - use Mixed and //0x for other bytes.";
	/*! \brief Error: //0x without two hex digits. */
	inline const wxString PARSE_BAD_MIXED_MSG = "//0x must be followed by two hex digits, e.g. //0x0D.";
	/*! \brief Format string: the text cannot change notation. */
	inline const wxString CONVERT_FAILED_FMT = "The text cannot be converted - %s";
	/*! \brief Question: switch to Mixed instead of ASCII. */
	inline const wxString CONVERT_TO_MIXED_MSG = "The data contains bytes that are not printable ASCII characters (e.g. CR, LF, 00), which ASCII cannot show.\n\nSwitch to Mixed instead? (No keeps the current notation.)";

	// --- Character table ---
	/*! \brief Window title: character table. */
	inline const wxString ASCII_TABLE_TITLE = "Insert Character";
	/*! \brief Help: character table. */
	inline const wxString ASCII_TABLE_HELP = "Double-click a character to add it and close this table. Right-click to add it and keep the table open.";
	/*! \brief Column: decimal value. */
	inline const wxString ASCII_COL_DEC = "Dec";
	/*! \brief Column: hex value. */
	inline const wxString ASCII_COL_HEX = "Hex";
	/*! \brief Column: character. */
	inline const wxString ASCII_COL_CHAR = "Char";
	/*! \brief Column: name. */
	inline const wxString ASCII_COL_NAME = "Name";
	/*! \brief Column: description. */
	inline const wxString ASCII_COL_DESCRIPTION = "Description";
	/*! \brief Name of the space character. */
	inline const wxString ASCII_SPACE_NAME = "Space";
	/*! \brief Description: a digit. */
	inline const wxString ASCII_DIGIT_FMT = "Digit %s";
	/*! \brief Description: a capital letter. */
	inline const wxString ASCII_CAPITAL_LETTER_FMT = "Capital letter %s";
	/*! \brief Description: a small letter. */
	inline const wxString ASCII_SMALL_LETTER_FMT = "Small letter %s";
	/*! \brief Description: bytes 0x80-0xFF. */
	inline const wxString ASCII_NOT_ASCII_DESC = "Not ASCII (shown as its Latin-1 character)";
	/*! \brief Button: add and keep the table open. */
	inline const wxString ASCII_ADD_KEEP_OPEN_BTN = "Add, keep open";
	/*! \brief Menu item: add a character and keep the table open. */
	inline const wxString ASCII_ADD_KEEP_OPEN_FMT = "Add %s and keep this table open";
	/*! \brief Label: the characters added so far. */
	inline const wxString ASCII_ADDED_FMT = "Added: %s";
	/*! \brief Button: close. */
	inline const wxString CLOSE_BTN = "Close";

	// --- Common buttons and labels of the message dialogs ---
	/*! \brief Button: add. */
	inline const wxString ADD_BTN = "Add...";
	/*! \brief Button: edit. */
	inline const wxString EDIT_BTN = "Edit...";
	/*! \brief Button: duplicate. */
	inline const wxString DUPLICATE_BTN = "Duplicate";
	/*! \brief Button: remove. */
	inline const wxString REMOVE_BTN = "Remove";
	/*! \brief Question: remove an item. */
	inline const wxString REMOVE_CONFIRM_FMT = "Remove \"%s\"?";
	/*! \brief Label: name. */
	inline const wxString NAME_LABEL = "Name:";
	/*! \brief Check box: enabled. */
	inline const wxString ENABLED_CHECK = "Enabled";
	/*! \brief Error: not a number. */
	inline const wxString NOT_A_NUMBER_FMT = "\"%s\" is not a valid number.";
	/*! \brief Label: byte order. */
	inline const wxString BYTE_ORDER_LABEL = "Byte order:";
	/*! \brief Radio button: big-endian. */
	inline const wxString ORDER_BIG_ENDIAN = "Big-endian (most significant byte first)";
	/*! \brief Radio button: little-endian. */
	inline const wxString ORDER_LITTLE_ENDIAN = "Little-endian (least significant byte first)";
	/*! \brief Short text: big-endian. */
	inline const wxString ORDER_BIG_SHORT = "big-endian";
	/*! \brief Short text: little-endian. */
	inline const wxString ORDER_LITTLE_SHORT = "little-endian";

	// --- Incrementing value dialog ---
	/*! \brief Window title: incrementing value. */
	inline const wxString COUNTER_DIALOG_TITLE = "Incrementing Value";
	/*! \brief Label: how the value is written. */
	inline const wxString COUNTER_ENCODING_LABEL = "Written as:";
	/*! \brief Choice: binary number. */
	inline const wxString COUNTER_ENC_BINARY = "Binary number";
	/*! \brief Choice: ASCII decimal digits. */
	inline const wxString COUNTER_ENC_DECIMAL = "ASCII decimal digits";
	/*! \brief Choice: ASCII hex digits. */
	inline const wxString COUNTER_ENC_HEX = "ASCII hex digits";
	/*! \brief Label: size. */
	inline const wxString COUNTER_SIZE_LABEL = "Size:";
	/*! \brief Unit: bytes (and bits). */
	inline const wxString COUNTER_UNIT_BYTES_FMT = "bytes (%d bits)";
	/*! \brief Unit: characters. */
	inline const wxString COUNTER_UNIT_CHARACTERS = "characters";
	/*! \brief Label: first byte index. */
	inline const wxString COUNTER_INDEX_LABEL = "Starts at byte index:";
	/*! \brief Explanation: the bytes a binary value occupies. */
	inline const wxString COUNTER_SPAN_BINARY_FMT = "%d-bit value, %d byte(s): from byte %d to byte %d (both included).";
	/*! \brief Explanation: the bytes an ASCII value occupies. */
	inline const wxString COUNTER_SPAN_TEXT_FMT = "%d character(s): from byte %d to byte %d (both included).";
	/*! \brief Explanation: the message is extended. */
	inline const wxString COUNTER_SPAN_EXTENDS_FMT = "The message is %d byte(s) long now: it is extended with zero bytes.";
	/*! \brief Explanation: default byte order. */
	inline const wxString COUNTER_ORDER_NOTE = "Big-endian is the default: Modbus and most binary protocols send numbers this way. (The serial line sends the bits of each byte least significant first, but the order of the bytes is set by the protocol.)";
	/*! \brief Label: start value. */
	inline const wxString COUNTER_START_LABEL = "Start value:";
	/*! \brief Label: end value. */
	inline const wxString COUNTER_END_LABEL = "End value:";
	/*! \brief Label: step. */
	inline const wxString COUNTER_STEP_LABEL = "Step:";
	/*! \brief Hint: number formats. */
	inline const wxString COUNTER_NUMBER_HINT = "decimal, or hex as 0x1F";
	/*! \brief Label: how fast the value advances. */
	inline const wxString COUNTER_ADVANCE_LABEL = "Advances:";
	/*! \brief Unit after the number of steps. */
	inline const wxString COUNTER_STEPS_WORD = "step(s)";
	/*! \brief Radio button: per message. */
	inline const wxString COUNTER_PER_MESSAGE = "per message sent";
	/*! \brief Radio button: per second. */
	inline const wxString COUNTER_PER_SECOND = "per second";
	/*! \brief Explanation: the first values. */
	inline const wxString COUNTER_VALUES_FMT = "Values: %s never beyond %s, then again from the start value.";
	/*! \brief Explanation: the largest value. */
	inline const wxString COUNTER_MAX_FMT = "Largest value for this size: %s (%s).";
	/*! \brief List line: one incrementing value. */
	inline const wxString COUNTER_SUMMARY_FMT = "Bytes %d-%d (%s): %s to %s, step %s, %s";
	/*! \brief Rate: steps per message. */
	inline const wxString RATE_STEPS_PER_MESSAGE_FMT = "%s step(s) per message sent";
	/*! \brief Rate: one step every N messages. */
	inline const wxString RATE_ONE_STEP_EVERY_MESSAGES_FMT = "one step every %s messages sent";
	/*! \brief Rate: steps per second. */
	inline const wxString RATE_STEPS_PER_SECOND_FMT = "%s step(s) per second";
	/*! \brief Rate: one step every N seconds. */
	inline const wxString RATE_ONE_STEP_EVERY_SECONDS_FMT = "one step every %s seconds";
	/*! \brief Error: size out of range. */
	inline const wxString COUNTER_ERR_WIDTH = "The size is out of range for this kind of value.";
	/*! \brief Error: start/end do not fit. */
	inline const wxString COUNTER_ERR_RANGE_FMT = "The start and end values must fit in the chosen size: at most %s (%s).";
	/*! \brief Error: step 0. */
	inline const wxString COUNTER_ERR_STEP = "The step cannot be 0.";
	/*! \brief Error: step goes the wrong way. */
	inline const wxString COUNTER_ERR_DIRECTION = "The step must go from the start value towards the end value (negative when the end is below the start).";
	/*! \brief Error: rate not positive. */
	inline const wxString COUNTER_ERR_RATE = "The number of steps must be a number greater than 0 (e.g. 1, 0.5, 0.1).";

	// --- Checksum dialog ---
	/*! \brief Window title: checksum. */
	inline const wxString CHECKSUM_DIALOG_TITLE = "Checksum";
	/*! \brief Check box: add a checksum. */
	inline const wxString CHECKSUM_ENABLE = "Add a checksum to the message";
	/*! \brief Label: checksum type. */
	inline const wxString CHECKSUM_TYPE_LABEL = "Type:";
	/*! \brief Choice item: checksum and its size. */
	inline const wxString CHECKSUM_TYPE_ITEM_FMT = "%s - %d byte(s)";
	/*! \brief Label: insert position. */
	inline const wxString CHECKSUM_POSITION_LABEL = "Insert at index:";
	/*! \brief Check box: at the end. */
	inline const wxString CHECKSUM_AT_END = "at the end (follows the length of the message)";
	/*! \brief Explanation: index range. */
	inline const wxString CHECKSUM_POSITION_NOTE_FMT = "0 = before the first byte, %d = after the last one.";
	/*! \brief Label: bytes covered. */
	inline const wxString CHECKSUM_COVER_LABEL = "Calculated on:";
	/*! \brief Radio button: all bytes. */
	inline const wxString CHECKSUM_ALL_BYTES = "all the bytes";
	/*! \brief Radio button: all bytes except some. */
	inline const wxString CHECKSUM_EXCEPT = "all the bytes except the indexes:";
	/*! \brief Hint: excluded indexes. */
	inline const wxString CHECKSUM_EXCEPT_HINT = "e.g. 0, 5-7 (0 = first byte)";
	/*! \brief Label: checksum bytes to send. */
	inline const wxString CHECKSUM_BYTES_LABEL = "Bytes to send:";
	/*! \brief Text after the number of bytes. */
	inline const wxString CHECKSUM_BYTES_OF_FMT = "of %d";
	/*! \brief Radio button: lower part. */
	inline const wxString CHECKSUM_LOWER_PART = "lower part (least significant bytes)";
	/*! \brief Radio button: upper part. */
	inline const wxString CHECKSUM_UPPER_PART = "upper part (most significant bytes)";
	/*! \brief Preview: checksum value and bytes. */
	inline const wxString CHECKSUM_PREVIEW_FMT = "Checksum of the current message: %s - sent as %s at index %d.";
	/*! \brief Error: excluded indexes. */
	inline const wxString CHECKSUM_ERR_EXCLUDED = "Write the excluded indexes as numbers and ranges separated by commas, e.g. 0, 5-7.";
	/*! \brief Text: no checksum. */
	inline const wxString CHECKSUM_NONE = "none";
	/*! \brief Summary: checksum type, bytes, position. */
	inline const wxString CHECKSUM_SUMMARY_FMT = "%s, %d byte(s) at %s";
	/*! \brief Short text: at the end. */
	inline const wxString CHECKSUM_AT_END_SHORT = "the end";
	/*! \brief Short text: at an index. */
	inline const wxString CHECKSUM_AT_INDEX_FMT = "index %d";
	/*! \brief Short text: excluded indexes. */
	inline const wxString CHECKSUM_EXCLUDING_FMT = "excluding %s";

	// --- Message editor ---
	/*! \brief Label: incrementing values. */
	inline const wxString MESSAGE_COUNTERS_LABEL = "Incrementing values (written over the message bytes):";
	/*! \brief Label: checksum. */
	inline const wxString MESSAGE_CHECKSUM_LABEL = "Checksum:";
	/*! \brief Button: edit the checksum. */
	inline const wxString CHECKSUM_BTN = "Checksum...";
	/*! \brief Label: preview. */
	inline const wxString MESSAGE_PREVIEW_LABEL = "Preview of the first messages (one second apart):";
	/*! \brief Preview line: message number and size. */
	inline const wxString PREVIEW_LINE_FMT = "#%d (%d bytes):";
	/*! \brief Error: empty message. */
	inline const wxString MESSAGE_EMPTY_MSG = "Write the message to send.";

	// --- Auto reply and periodic message dialogs ---
	/*! \brief Window title: auto reply. */
	inline const wxString AUTO_REPLY_DIALOG_TITLE = "Auto Reply";
	/*! \brief Group box: the pattern. */
	inline const wxString AUTO_REPLY_PATTERN_LABEL = "When these bytes are received";
	/*! \brief Group box: the reply. */
	inline const wxString AUTO_REPLY_REPLY_LABEL = "Reply with";
	/*! \brief Label: delay. */
	inline const wxString AUTO_REPLY_DELAY_LABEL = "Delay before replying (ms):";
	/*! \brief Error: empty pattern. */
	inline const wxString AUTO_REPLY_EMPTY_PATTERN_MSG = "Write the bytes to recognise.";
	/*! \brief Window title: periodic message. */
	inline const wxString PERIODIC_DIALOG_TITLE = "Periodic Message";
	/*! \brief Label: period. */
	inline const wxString PERIODIC_PERIOD_LABEL = "Send every (ms):";
	/*! \brief Group box: the message. */
	inline const wxString PERIODIC_MESSAGE_LABEL = "Message";
	/*! \brief Default name of a new auto reply. */
	inline const wxString DEFAULT_AUTO_REPLY_NAME_FMT = "Auto reply %d";
	/*! \brief Default name of a new periodic message. */
	inline const wxString DEFAULT_PERIODIC_NAME_FMT = "Periodic message %d";

	// --- Auto replies and periodic messages lists ---
	/*! \brief Window title: automation lists. */
	inline const wxString AUTOMATION_TITLE = "Auto Replies and Periodic Messages";
	/*! \brief Tab: auto replies. */
	inline const wxString AUTO_REPLIES_TAB = "Auto replies";
	/*! \brief Tab: periodic messages. */
	inline const wxString PERIODIC_TAB = "Periodic messages";
	/*! \brief Help: auto replies. */
	inline const wxString AUTO_REPLIES_HELP = "While the port is open, each ticked rule replies as soon as its bytes are received. Double-click to edit.";
	/*! \brief Help: periodic messages. */
	inline const wxString PERIODIC_HELP = "While the port is open, each ticked message is sent at its interval. Double-click to edit.";
	/*! \brief Column: name. */
	inline const wxString COL_NAME = "Name";
	/*! \brief Column: pattern. */
	inline const wxString COL_WHEN_RECEIVED = "When received";
	/*! \brief Column: reply. */
	inline const wxString COL_REPLY = "Reply";
	/*! \brief Column: delay. */
	inline const wxString COL_DELAY_MS = "Delay (ms)";
	/*! \brief Column: message. */
	inline const wxString COL_MESSAGE = "Message";
	/*! \brief Column: period. */
	inline const wxString COL_EVERY_MS = "Every (ms)";
}
