/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <wx/panel.h>

#include "SerialData.h"

class wxRadioButton;
class wxStaticText;
class wxTextCtrl;

/*!
 * \file DataEntry.h
 * \brief A text box for serial data with the notation chosen by radio buttons: Hex, ASCII or Mixed
 * (SerialData.h), an "Insert character..." button (AsciiTableDialog) and a hint line that explains
 * the notation or shows what is wrong with the text.
 *
 * - Hex: only hex digits can be typed; the bytes are separated by a space automatically (pasted
 *   "0x4A,0x5B" becomes "4A 5B").
 * - ASCII: printable ASCII characters only.
 * - Mixed: text plus //0xHH for any byte.
 * Changing the notation converts the text already written (bytes that ASCII cannot show - control
 * characters - offer to switch to Mixed instead).
 */
/*!
 * \brief Serial data text box with notation radio buttons (see the file comment).
 */
class DataEntry : public wxPanel {
public:
	/*!
	 * \param parent        parent window.
	 * \param format        initial notation.
	 * \param text          initial text (in that notation).
	 * \param processEnter  true to report the Enter key through SetOnEnter().
	 */
	DataEntry(wxWindow* parent, SerialData::DataFormat format = SerialData::DataFormat::Hex,
		const wxString& text = wxEmptyString, bool processEnter = false);

	/*! \brief The notation in use. */
	SerialData::DataFormat GetFormat() const { return format_; }
	/*! \brief The text as written. */
	wxString GetText() const;
	/*! \brief Sets notation and text as they are (no conversion). */
	void SetData(SerialData::DataFormat format, const wxString& text);

	/*! \brief The bytes of the current text, or why it is not valid. */
	SerialData::ParseResult Parse() const;
	/*!
	 * \brief The bytes of the current text. If it is not valid, explains the problem in a message
	 * box, selects the wrong character and returns false.
	 */
	bool GetBytes(std::vector<uint8_t>& out);
	/*! \brief Inserts one byte at the caret, written in the current notation (see InsertByte in the .cpp). */
	void InsertByte(uint8_t b);
	/*! \brief Opens the character table (AsciiTableDialog) to insert characters at the caret. */
	void ShowCharacterTable();

	/*! \brief Called after every change of text or notation. */
	void SetOnChange(std::function<void()> fn) { onChange_ = std::move(fn); }
	/*! \brief Called when Enter is pressed (needs processEnter in the constructor). */
	void SetOnEnter(std::function<void()> fn) { onEnter_ = std::move(fn); }

	/*! \brief The translated explanation of a ParseError ("not a hex digit", ...). */
	static wxString ErrorText(SerialData::ParseError error);
	/*! \brief The translated name of a notation ("Hex", "ASCII", "Mixed"). */
	static wxString FormatName(SerialData::DataFormat format);

private:
	void setFormatConverting(SerialData::DataFormat to);
	void onChar(wxKeyEvent& event);
	void onText();
	void updateHint();
	void selectCharacter(size_t position);

	SerialData::DataFormat format_;
	wxRadioButton* radio_[3] = {};
	wxTextCtrl* text_ = nullptr;
	wxStaticText* hint_ = nullptr;
	bool reformatting_ = false;
	std::function<void()> onChange_;
	std::function<void()> onEnter_;
};
