/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file AsciiTableDialog.cpp
 * \brief Implementation of AsciiTableDialog.h.
 */

#include "AsciiTableDialog.h"
#include "HexUtils.h"
#include "I18n.h"
#include "SerialData.h"
#include "UVT.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace {
	enum Column { COL_DEC, COL_HEX, COL_CHAR, COL_NAME, COL_DESCRIPTION };

	wxString description(uint8_t b) {
		if (b >= '0' && b <= '9') return wxString::Format(tr(UVT::ASCII_DIGIT_FMT), wxString(static_cast<char>(b)));
		if (b >= 'A' && b <= 'Z') return wxString::Format(tr(UVT::ASCII_CAPITAL_LETTER_FMT), wxString(static_cast<char>(b)));
		if (b >= 'a' && b <= 'z') return wxString::Format(tr(UVT::ASCII_SMALL_LETTER_FMT), wxString(static_cast<char>(b)));
		if (b >= 0x80) return tr(UVT::ASCII_NOT_ASCII_DESC);
		return tr(wxString::FromUTF8(SerialData::asciiInfo(b).description));
	}
}

AsciiTableDialog::AsciiTableDialog(wxWindow* parent, std::function<void(uint8_t)> add)
	: wxDialog(parent, wxID_ANY, tr(UVT::ASCII_TABLE_TITLE), wxDefaultPosition, wxSize(620, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  add_(std::move(add)) {
	auto* top = new wxBoxSizer(wxVERTICAL);
	auto* help = new wxStaticText(this, wxID_ANY, tr(UVT::ASCII_TABLE_HELP));
	help->Wrap(FromDIP(590));
	top->Add(help, 0, wxEXPAND | wxALL, 8);

	list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
	list_->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	list_->AppendColumn(tr(UVT::ASCII_COL_DEC), wxLIST_FORMAT_RIGHT, 50);
	list_->AppendColumn(tr(UVT::ASCII_COL_HEX), wxLIST_FORMAT_LEFT, 50);
	list_->AppendColumn(tr(UVT::ASCII_COL_CHAR), wxLIST_FORMAT_CENTER, 50);
	list_->AppendColumn(tr(UVT::ASCII_COL_NAME), wxLIST_FORMAT_LEFT, 70);
	list_->AppendColumn(tr(UVT::ASCII_COL_DESCRIPTION), wxLIST_FORMAT_LEFT, 330);
	for (int v = 0; v < 256; ++v) {
		const auto b = static_cast<uint8_t>(v);
		const long row = list_->InsertItem(v, wxString::Format("%d", v));
		list_->SetItem(row, COL_HEX, wxString::FromUTF8(Utils::Hex::toHex(b)));
		// Printable ASCII as itself; 0x80-0xFF as their Latin-1 character (what a Windows terminal usually shows).
		if (SerialData::isPrintable(b)) list_->SetItem(row, COL_CHAR, wxString(static_cast<char>(b)));
		else if (b >= 0xA0) list_->SetItem(row, COL_CHAR, wxString(wxUniChar(static_cast<unsigned>(b))));
		const char* name = SerialData::asciiInfo(b).name;
		list_->SetItem(row, COL_NAME, b == ' ' ? tr(UVT::ASCII_SPACE_NAME) : wxString::FromUTF8(name));
		list_->SetItem(row, COL_DESCRIPTION, description(b));
		list_->SetItemData(row, v);
	}
	list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& e) {
		addByte(static_cast<uint8_t>(e.GetData()));
		EndModal(wxID_OK);
	});
	list_->Bind(wxEVT_LIST_ITEM_RIGHT_CLICK, &AsciiTableDialog::onRightClick, this);
	top->Add(list_, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);

	added_ = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_START);
	top->Add(added_, 0, wxEXPAND | wxALL, 8);

	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	auto* addKeep = new wxButton(this, wxID_ANY, tr(UVT::ASCII_ADD_KEEP_OPEN_BTN));
	addKeep->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		const long sel = list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
		if (sel >= 0) addByte(static_cast<uint8_t>(list_->GetItemData(sel)));
	});
	buttons->Add(addKeep, 0, wxRIGHT, 8);
	buttons->AddStretchSpacer();
	buttons->Add(new wxButton(this, wxID_CANCEL, tr(UVT::CLOSE_BTN)), 0);
	SetEscapeId(wxID_CANCEL);
	top->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

	SetSizer(top);
	list_->SetItemState(0x41, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	list_->EnsureVisible(0x41);
	CentreOnParent();
}

void AsciiTableDialog::addByte(uint8_t b) {
	if (add_) add_(b);
	addedText_ += SerialData::displayByte(b);
	added_->SetLabelText(wxString::Format(tr(UVT::ASCII_ADDED_FMT), wxString::FromUTF8(addedText_)));
}

void AsciiTableDialog::onRightClick(wxListEvent& event) {
	const long row = event.GetIndex();
	if (row < 0) return;
	list_->SetItemState(row, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
	const auto b = static_cast<uint8_t>(list_->GetItemData(row));
	wxMenu menu;
	const int idAdd = wxWindow::NewControlId();
	wxString shown = wxString::FromUTF8(SerialData::displayByte(b));
	shown.Replace("&", "&&"); // '&' marks a menu accelerator
	menu.Append(idAdd, wxString::Format(tr(UVT::ASCII_ADD_KEEP_OPEN_FMT), shown));
	if (GetPopupMenuSelectionFromUser(menu) == idAdd) addByte(b);
}
