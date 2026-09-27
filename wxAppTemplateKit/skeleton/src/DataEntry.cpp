/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file DataEntry.cpp
 * \brief Implementation of DataEntry.h.
 */

#include "DataEntry.h"
#include "AsciiTableDialog.h"
#include "HexUtils.h"
#include "I18n.h"
#include "UVT.h"

#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

using SerialData::DataFormat;

namespace {
	constexpr DataFormat kFormats[3] = { DataFormat::Hex, DataFormat::Ascii, DataFormat::Mixed };
	int formatIndex(DataFormat f) { return f == DataFormat::Hex ? 0 : f == DataFormat::Ascii ? 1 : 2; }
}

DataEntry::DataEntry(wxWindow* parent, DataFormat format, const wxString& text, bool processEnter)
	: wxPanel(parent), format_(format) {
	auto* top = new wxBoxSizer(wxVERTICAL);

	auto* row = new wxBoxSizer(wxHORIZONTAL);
	for (int i = 0; i < 3; ++i) {
		radio_[i] = new wxRadioButton(this, wxID_ANY, FormatName(kFormats[i]), wxDefaultPosition, wxDefaultSize, i == 0 ? wxRB_GROUP : 0);
		radio_[i]->Bind(wxEVT_RADIOBUTTON, [this, i](wxCommandEvent&) { setFormatConverting(kFormats[i]); });
		row->Add(radio_[i], 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
	}
	row->AddStretchSpacer();
	auto* insert = new wxButton(this, wxID_ANY, tr(UVT::INSERT_CHAR_BTN));
	insert->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowCharacterTable(); });
	row->Add(insert, 0, wxALIGN_CENTER_VERTICAL);
	top->Add(row, 0, wxEXPAND | wxBOTTOM, 4);

	text_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, processEnter ? wxTE_PROCESS_ENTER : 0);
	text_->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	text_->Bind(wxEVT_CHAR, &DataEntry::onChar, this);
	text_->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { onText(); });
	if (processEnter) text_->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { if (onEnter_) onEnter_(); });
	top->Add(text_, 0, wxEXPAND);

	hint_ = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END);
	top->Add(hint_, 0, wxEXPAND | wxTOP, 2);

	SetSizer(top);
	SetData(format, text);
}

wxString DataEntry::FormatName(DataFormat format) {
	switch (format) {
	case DataFormat::Ascii: return tr(UVT::FORMAT_ASCII_LABEL);
	case DataFormat::Mixed: return tr(UVT::FORMAT_MIXED_LABEL);
	default:                return tr(UVT::FORMAT_HEX_LABEL);
	}
}

wxString DataEntry::ErrorText(SerialData::ParseError error) {
	switch (error) {
	case SerialData::ParseError::NotHexDigit:  return tr(UVT::PARSE_NOT_HEX_MSG);
	case SerialData::ParseError::OddHexDigits: return tr(UVT::PARSE_ODD_HEX_MSG);
	case SerialData::ParseError::NotAscii:     return tr(UVT::PARSE_NOT_ASCII_MSG);
	case SerialData::ParseError::BadMixedByte: return tr(UVT::PARSE_BAD_MIXED_MSG);
	default:                                   return wxEmptyString;
	}
}

wxString DataEntry::GetText() const { return text_->GetValue(); }

void DataEntry::SetData(DataFormat format, const wxString& text) {
	format_ = format;
	radio_[formatIndex(format)]->SetValue(true);
	reformatting_ = true;
	text_->ChangeValue(text);
	reformatting_ = false;
	onText(); // tidies Hex, updates the hint, notifies
}

SerialData::ParseResult DataEntry::Parse() const {
	return SerialData::parse(text_->GetValue().utf8_string(), format_);
}

void DataEntry::selectCharacter(size_t position) {
	text_->SetFocus();
	text_->SetSelection(static_cast<long>(position), static_cast<long>(position) + 1);
}

bool DataEntry::GetBytes(std::vector<uint8_t>& out) {
	const SerialData::ParseResult r = Parse();
	if (!r.ok()) {
		wxMessageBox(wxString::Format(tr(UVT::PARSE_ERROR_AT_FMT), static_cast<int>(r.errorPosition) + 1, ErrorText(r.error)),
			tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		selectCharacter(r.errorPosition);
		return false;
	}
	out = r.bytes;
	return true;
}

void DataEntry::setFormatConverting(DataFormat to) {
	if (to == format_) return;
	const DataFormat from = format_;
	const SerialData::ParseResult r = Parse();
	if (!r.ok()) {
		radio_[formatIndex(from)]->SetValue(true);
		wxMessageBox(wxString::Format(tr(UVT::CONVERT_FAILED_FMT),
			wxString::Format(tr(UVT::PARSE_ERROR_AT_FMT), static_cast<int>(r.errorPosition) + 1, ErrorText(r.error))),
			tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		selectCharacter(r.errorPosition);
		return;
	}
	std::optional<std::string> converted = SerialData::toText(r.bytes, to);
	if (!converted) { // ASCII cannot show control bytes
		if (wxMessageBox(tr(UVT::CONVERT_TO_MIXED_MSG), FormatName(to), wxYES_NO | wxICON_QUESTION, this) != wxYES) {
			radio_[formatIndex(from)]->SetValue(true);
			return;
		}
		to = DataFormat::Mixed;
		converted = SerialData::toText(r.bytes, to);
	}
	SetData(to, wxString::FromUTF8(*converted));
}

void DataEntry::InsertByte(uint8_t b) {
	wxString piece;
	switch (format_) {
	case DataFormat::Hex:
		piece = Utils::Hex::toHex(b); // onText() adds the spaces
		break;
	case DataFormat::Ascii:
		if (!SerialData::isPrintable(b)) {
			// ASCII cannot hold a control byte: continue in Mixed (always possible from ASCII).
			const long caret = text_->GetInsertionPoint();
			SetData(DataFormat::Mixed, text_->GetValue());
			text_->SetInsertionPoint(caret);
			InsertByte(b);
			return;
		}
		piece = wxString(static_cast<char>(b));
		break;
	case DataFormat::Mixed:
		piece = SerialData::isPrintable(b) ? wxString(static_cast<char>(b))
			: wxString::FromUTF8(std::string(SerialData::kMixedBytePrefix) + Utils::Hex::toHex(b));
		break;
	}
	text_->WriteText(piece); // replaces the selection, fires wxEVT_TEXT
	// No SetFocus() here: on some platforms focusing a text box selects all of it, and the next
	// insert (e.g. from the character table) would replace everything.
}

void DataEntry::ShowCharacterTable() {
	AsciiTableDialog dlg(this, [this](uint8_t b) { InsertByte(b); });
	dlg.ShowModal();
	const long caret = text_->GetInsertionPoint();
	text_->SetFocus();
	text_->SetInsertionPoint(caret); // keep the caret after the inserted characters (no select-all)
}

void DataEntry::onChar(wxKeyEvent& event) {
	const wxChar c = event.GetUnicodeKey();
	// Navigation keys, Backspace, Enter, Ctrl+C/V/X/A ... always pass.
	if (c == WXK_NONE || c < 32 || event.ControlDown() || event.AltDown()) { event.Skip(); return; }
	if (format_ == DataFormat::Hex) {
		if (wxIsxdigit(c) || c == ' ') event.Skip(); // anything else is not typed at all
		return;
	}
	if (c <= 0x7E) event.Skip(); // ASCII and Mixed: printable ASCII only
}

void DataEntry::onText() {
	if (reformatting_) return;
	if (format_ == DataFormat::Hex) {
		size_t cursor = static_cast<size_t>(text_->GetInsertionPoint());
		const std::string tidy = SerialData::formatHexInput(text_->GetValue().utf8_string(), cursor);
		if (wxString::FromUTF8(tidy) != text_->GetValue()) {
			reformatting_ = true;
			text_->ChangeValue(wxString::FromUTF8(tidy));
			text_->SetInsertionPoint(static_cast<long>(cursor));
			reformatting_ = false;
		}
	}
	updateHint();
	if (onChange_) onChange_();
}

void DataEntry::updateHint() {
	const SerialData::ParseResult r = Parse();
	wxString hint;
	if (!r.ok()) {
		hint = wxString::Format(tr(UVT::PARSE_ERROR_AT_FMT), static_cast<int>(r.errorPosition) + 1, ErrorText(r.error));
		hint_->SetForegroundColour(wxColour(220, 40, 40));
	}
	else {
		switch (format_) {
		case DataFormat::Hex:   hint = tr(UVT::HINT_HEX); break;
		case DataFormat::Ascii: hint = tr(UVT::HINT_ASCII); break;
		case DataFormat::Mixed: hint = tr(UVT::HINT_MIXED); break;
		}
		if (!r.bytes.empty()) hint = wxString::Format(tr(UVT::BYTE_COUNT_FMT), static_cast<int>(r.bytes.size())) + "  " + hint;
		hint_->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
	}
	hint_->SetLabelText(hint);
	hint_->SetToolTip(hint);
	hint_->Refresh();
}
