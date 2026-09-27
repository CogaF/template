/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file MessageDialogs.cpp
 * \brief Implementation of MessageDialogs.h.
 */

#include "MessageDialogs.h"
#include "DataEntry.h"
#include "HexUtils.h"
#include "I18n.h"
#include "UVT.h"

#include <charconv>
#include <cmath>
#include <limits>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

using namespace SerialData;

namespace {
	wxString num(uint64_t v) { return wxString::FromUTF8(std::to_string(v)); }
	wxString num(int64_t v) { return wxString::FromUTF8(std::to_string(v)); }

	wxString rateText(double v) {
		char buf[64];
		const auto r = std::to_chars(buf, buf + sizeof(buf), v);
		return wxString::FromUTF8(std::string(buf, r.ptr));
	}

	/*! \brief "42", "0x2A", "2Ah", "0b101010" -> 42. */
	std::optional<uint64_t> readUnsigned(const wxString& text) {
		return Utils::Hex::parseNumber(text.Strip(wxString::both).utf8_string());
	}

	/*! \brief As readUnsigned(), with an optional leading '-'. */
	std::optional<int64_t> readSigned(const wxString& text) {
		wxString t = text.Strip(wxString::both);
		const bool negative = t.StartsWith("-");
		if (negative || t.StartsWith("+")) t = t.Mid(1);
		const auto v = Utils::Hex::parseNumber(t.utf8_string());
		if (!v || *v > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) return std::nullopt;
		return negative ? -static_cast<int64_t>(*v) : static_cast<int64_t>(*v);
	}

	/*! \brief "0.1" or "0,1" -> 0.1 (independent of the locale). */
	std::optional<double> readDouble(const wxString& text) {
		wxString t = text.Strip(wxString::both);
		t.Replace(",", ".");
		const std::string s = t.utf8_string();
		double v = 0;
		const auto r = std::from_chars(s.data(), s.data() + s.size(), v);
		if (r.ec != std::errc() || r.ptr != s.data() + s.size()) return std::nullopt;
		return v;
	}

	wxString encodingName(CounterEncoding e) {
		switch (e) {
		case CounterEncoding::AsciiDecimal: return tr(UVT::COUNTER_ENC_DECIMAL);
		case CounterEncoding::AsciiHex:     return tr(UVT::COUNTER_ENC_HEX);
		default:                            return tr(UVT::COUNTER_ENC_BINARY);
		}
	}

	wxStaticText* note(wxWindow* parent, const wxString& text) {
		auto* t = new wxStaticText(parent, wxID_ANY, text);
		t->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
		return t;
	}

	wxString hexOrText(const std::vector<uint8_t>& bytes) {
		return wxString::FromUTF8(Utils::Hex::bytesToString(bytes) + "   |  " + display(bytes));
	}
}

// ================================================================================================
// CounterDialog
// ================================================================================================

CounterDialog::CounterDialog(wxWindow* parent, const CounterSpec& initial, size_t messageLength)
	: wxDialog(parent, wxID_ANY, tr(UVT::COUNTER_DIALOG_TITLE), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  counter_(initial), messageLength_(messageLength) {
	auto* grid = new wxFlexGridSizer(2, wxSize(10, 6));
	grid->AddGrowableCol(1, 1);
	auto label = [&](const wxString& text) { grid->Add(new wxStaticText(this, wxID_ANY, text), 0, wxALIGN_CENTER_VERTICAL); };
	auto onChange = [this](wxEvent&) { refresh(); };

	label(tr(UVT::COUNTER_ENCODING_LABEL));
	encoding_ = new wxChoice(this, wxID_ANY);
	for (CounterEncoding e : { CounterEncoding::Binary, CounterEncoding::AsciiDecimal, CounterEncoding::AsciiHex }) encoding_->Append(encodingName(e));
	encoding_->SetSelection(static_cast<int>(initial.encoding));
	encoding_->Bind(wxEVT_CHOICE, onChange);
	grid->Add(encoding_, 1, wxEXPAND);

	label(tr(UVT::COUNTER_SIZE_LABEL));
	auto* sizeRow = new wxBoxSizer(wxHORIZONTAL);
	width_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, maxCounterWidth(initial.encoding), initial.width);
	width_->Bind(wxEVT_SPINCTRL, onChange);
	width_->Bind(wxEVT_TEXT, onChange);
	unit_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
	sizeRow->Add(width_, 0, wxRIGHT, 6);
	sizeRow->Add(unit_, 1, wxALIGN_CENTER_VERTICAL);
	grid->Add(sizeRow, 1, wxEXPAND);

	label(tr(UVT::COUNTER_INDEX_LABEL));
	index_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 65535, static_cast<int>(initial.index));
	index_->Bind(wxEVT_SPINCTRL, onChange);
	index_->Bind(wxEVT_TEXT, onChange);
	grid->Add(index_, 0);

	grid->AddSpacer(0);
	span_ = note(this, wxEmptyString);
	grid->Add(span_, 1, wxEXPAND);

	label(tr(UVT::BYTE_ORDER_LABEL));
	auto* orderBox = new wxBoxSizer(wxVERTICAL);
	bigEndian_ = new wxRadioButton(this, wxID_ANY, tr(UVT::ORDER_BIG_ENDIAN), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	littleEndian_ = new wxRadioButton(this, wxID_ANY, tr(UVT::ORDER_LITTLE_ENDIAN));
	(initial.order == ByteOrder::LittleEndian ? littleEndian_ : bigEndian_)->SetValue(true);
	orderBox->Add(bigEndian_, 0, wxBOTTOM, 2);
	orderBox->Add(littleEndian_, 0, wxBOTTOM, 2);
	auto* orderNote = note(this, tr(UVT::COUNTER_ORDER_NOTE));
	orderNote->Wrap(FromDIP(420));
	orderBox->Add(orderNote, 0);
	grid->Add(orderBox, 1, wxEXPAND);

	auto numberRow = [&](const wxString& text, const wxString& value) {
		label(text);
		auto* ctrl = new wxTextCtrl(this, wxID_ANY, value);
		ctrl->SetHint(tr(UVT::COUNTER_NUMBER_HINT));
		ctrl->Bind(wxEVT_TEXT, onChange);
		grid->Add(ctrl, 1, wxEXPAND);
		return ctrl;
	};
	start_ = numberRow(tr(UVT::COUNTER_START_LABEL), num(initial.start));
	end_ = numberRow(tr(UVT::COUNTER_END_LABEL), num(initial.end));
	step_ = numberRow(tr(UVT::COUNTER_STEP_LABEL), num(initial.step));

	label(tr(UVT::COUNTER_ADVANCE_LABEL));
	auto* rateRow = new wxBoxSizer(wxHORIZONTAL);
	rate_ = new wxTextCtrl(this, wxID_ANY, rateText(initial.rate), wxDefaultPosition, wxSize(FromDIP(70), -1));
	rate_->Bind(wxEVT_TEXT, onChange);
	perMessage_ = new wxRadioButton(this, wxID_ANY, tr(UVT::COUNTER_PER_MESSAGE), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	perSecond_ = new wxRadioButton(this, wxID_ANY, tr(UVT::COUNTER_PER_SECOND));
	(initial.rateMode == CounterRate::PerSecond ? perSecond_ : perMessage_)->SetValue(true);
	rateRow->Add(rate_, 0, wxRIGHT, 6);
	rateRow->Add(new wxStaticText(this, wxID_ANY, tr(UVT::COUNTER_STEPS_WORD)), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	rateRow->Add(perMessage_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	rateRow->Add(perSecond_, 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(rateRow, 1, wxEXPAND);
	for (wxRadioButton* r : { bigEndian_, littleEndian_, perMessage_, perSecond_ }) r->Bind(wxEVT_RADIOBUTTON, onChange);

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(grid, 0, wxEXPAND | wxALL, 12);
	info_ = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(520), FromDIP(70)), wxST_NO_AUTORESIZE);
	top->Add(info_, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
	top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
	SetSizerAndFit(top);
	refresh();
	CentreOnParent();
}

bool CounterDialog::read(CounterSpec& c, wxString* error) const {
	c.encoding = static_cast<CounterEncoding>((std::max)(encoding_->GetSelection(), 0));
	c.width = width_->GetValue();
	c.index = static_cast<size_t>(index_->GetValue());
	c.order = littleEndian_->GetValue() ? ByteOrder::LittleEndian : ByteOrder::BigEndian;
	c.rateMode = perSecond_->GetValue() ? CounterRate::PerSecond : CounterRate::PerMessage;
	auto bad = [&](const wxTextCtrl* ctrl) {
		if (error) *error = wxString::Format(tr(UVT::NOT_A_NUMBER_FMT), ctrl->GetValue());
		return false;
	};
	const auto start = readUnsigned(start_->GetValue());
	if (!start) return bad(start_);
	const auto end = readUnsigned(end_->GetValue());
	if (!end) return bad(end_);
	const auto step = readSigned(step_->GetValue());
	if (!step) return bad(step_);
	const auto rate = readDouble(rate_->GetValue());
	if (!rate) return bad(rate_);
	c.start = *start;
	c.end = *end;
	c.step = *step;
	c.rate = *rate;
	wxString why;
	switch (validate(c)) {
	case SpecError::None: return true;
	case SpecError::CounterWidth: why = tr(UVT::COUNTER_ERR_WIDTH); break;
	case SpecError::CounterRange:
		why = wxString::Format(tr(UVT::COUNTER_ERR_RANGE_FMT), num(maxCounterValue(c)), wxString::FromUTF8(Utils::Hex::toHex(maxCounterValue(c), 1, true)));
		break;
	case SpecError::CounterStep: why = tr(UVT::COUNTER_ERR_STEP); break;
	case SpecError::CounterDirection: why = tr(UVT::COUNTER_ERR_DIRECTION); break;
	default: why = tr(UVT::COUNTER_ERR_RATE); break;
	}
	if (error) *error = why;
	return false;
}

void CounterDialog::refresh() {
	const auto encoding = static_cast<CounterEncoding>((std::max)(encoding_->GetSelection(), 0));
	const int maxWidth = maxCounterWidth(encoding);
	if (width_->GetMax() != maxWidth) width_->SetRange(1, maxWidth);
	const bool binary = encoding == CounterEncoding::Binary;
	unit_->SetLabel(binary ? wxString::Format(tr(UVT::COUNTER_UNIT_BYTES_FMT), width_->GetValue() * 8) : tr(UVT::COUNTER_UNIT_CHARACTERS));
	DataEntry::SetEnabledRepainting(bigEndian_, binary && width_->GetValue() > 1);
	DataEntry::SetEnabledRepainting(littleEndian_, binary && width_->GetValue() > 1);

	const int w = width_->GetValue();
	const int first = index_->GetValue();
	wxString span = binary ? wxString::Format(tr(UVT::COUNTER_SPAN_BINARY_FMT), w * 8, w, first, first + w - 1)
		: wxString::Format(tr(UVT::COUNTER_SPAN_TEXT_FMT), w, first, first + w - 1);
	if (static_cast<size_t>(first + w) > messageLength_)
		span += " " + wxString::Format(tr(UVT::COUNTER_SPAN_EXTENDS_FMT), static_cast<int>(messageLength_));
	span_->SetLabel(span);
	span_->Wrap(FromDIP(420));

	CounterSpec c;
	wxString error;
	wxString info;
	if (!read(c, &error)) {
		info = error;
		info_->SetForegroundColour(wxColour(220, 40, 40));
	}
	else {
		info_->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		wxString values;
		for (uint64_t k = 0; k < 4; ++k) values << num(counterValue(c, k)) << ", ";
		info = wxString::Format(tr(UVT::COUNTER_VALUES_FMT), values + "...", num(c.end)) + "\n"
			+ MessageEditor::DescribeRate(c.rateMode, c.rate) + "\n"
			+ wxString::Format(tr(UVT::COUNTER_MAX_FMT), num(maxCounterValue(c)), wxString::FromUTF8(Utils::Hex::toHex(maxCounterValue(c), 1, true)));
	}
	info_->SetLabel(info);
	Layout();
}

bool CounterDialog::TransferDataFromWindow() {
	wxString error;
	if (!read(counter_, &error)) {
		wxMessageBox(error, tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		return false;
	}
	return true;
}

// ================================================================================================
// ChecksumDialog
// ================================================================================================

ChecksumDialog::ChecksumDialog(wxWindow* parent, const ChecksumSpec& initial, std::vector<uint8_t> message)
	: wxDialog(parent, wxID_ANY, tr(UVT::CHECKSUM_DIALOG_TITLE), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  checksum_(initial), message_(std::move(message)) {
	auto onChange = [this](wxEvent&) { refresh(); };
	auto* top = new wxBoxSizer(wxVERTICAL);
	enabled_ = new wxCheckBox(this, wxID_ANY, tr(UVT::CHECKSUM_ENABLE));
	enabled_->SetValue(initial.enabled);
	enabled_->Bind(wxEVT_CHECKBOX, onChange);
	top->Add(enabled_, 0, wxALL, 12);

	auto* grid = new wxFlexGridSizer(2, wxSize(10, 6));
	grid->AddGrowableCol(1, 1);
	auto label = [&](const wxString& text) { grid->Add(new wxStaticText(this, wxID_ANY, text), 0, wxALIGN_CENTER_VERTICAL); };

	label(tr(UVT::CHECKSUM_TYPE_LABEL));
	type_ = new wxChoice(this, wxID_ANY);
	for (const ChecksumInfo& info : checksumTypes()) {
		type_->Append(wxString::Format(tr(UVT::CHECKSUM_TYPE_ITEM_FMT), wxString::FromUTF8(info.name), info.bytes));
		if (info.type == initial.type) type_->SetSelection(static_cast<int>(type_->GetCount()) - 1);
	}
	type_->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onTypeChanged(); });
	grid->Add(type_, 1, wxEXPAND);

	label(tr(UVT::CHECKSUM_POSITION_LABEL));
	auto* posRow = new wxBoxSizer(wxHORIZONTAL);
	const int len = static_cast<int>(message_.size());
	insertAt_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, len,
		initial.insertAt < 0 || initial.insertAt > len ? len : initial.insertAt);
	insertAt_->Bind(wxEVT_SPINCTRL, onChange);
	insertAt_->Bind(wxEVT_TEXT, onChange);
	atEnd_ = new wxCheckBox(this, wxID_ANY, tr(UVT::CHECKSUM_AT_END));
	atEnd_->SetValue(initial.insertAt < 0);
	atEnd_->Bind(wxEVT_CHECKBOX, onChange);
	posRow->Add(insertAt_, 0, wxRIGHT, 8);
	posRow->Add(atEnd_, 1, wxALIGN_CENTER_VERTICAL);
	grid->Add(posRow, 1, wxEXPAND);
	grid->AddSpacer(0);
	grid->Add(note(this, wxString::Format(tr(UVT::CHECKSUM_POSITION_NOTE_FMT), len)), 0);

	label(tr(UVT::CHECKSUM_COVER_LABEL));
	auto* cover = new wxBoxSizer(wxVERTICAL);
	allBytes_ = new wxRadioButton(this, wxID_ANY, tr(UVT::CHECKSUM_ALL_BYTES), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	auto* exceptRow = new wxBoxSizer(wxHORIZONTAL);
	except_ = new wxRadioButton(this, wxID_ANY, tr(UVT::CHECKSUM_EXCEPT));
	excluded_ = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(indexListToString(initial.excluded)));
	excluded_->SetHint(tr(UVT::CHECKSUM_EXCEPT_HINT));
	excluded_->Bind(wxEVT_TEXT, onChange);
	(initial.excluded.empty() ? allBytes_ : except_)->SetValue(true);
	exceptRow->Add(except_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	exceptRow->Add(excluded_, 1);
	cover->Add(allBytes_, 0, wxBOTTOM, 4);
	cover->Add(exceptRow, 0, wxEXPAND);
	grid->Add(cover, 1, wxEXPAND);

	label(tr(UVT::CHECKSUM_BYTES_LABEL));
	auto* partBox = new wxBoxSizer(wxVERTICAL);
	auto* bytesRow = new wxBoxSizer(wxHORIZONTAL);
	bytes_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 8, 1);
	bytes_->Bind(wxEVT_SPINCTRL, onChange);
	bytes_->Bind(wxEVT_TEXT, onChange);
	bytesOf_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
	bytesRow->Add(bytes_, 0, wxRIGHT, 6);
	bytesRow->Add(bytesOf_, 1, wxALIGN_CENTER_VERTICAL);
	lower_ = new wxRadioButton(this, wxID_ANY, tr(UVT::CHECKSUM_LOWER_PART), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	upper_ = new wxRadioButton(this, wxID_ANY, tr(UVT::CHECKSUM_UPPER_PART));
	(initial.upperPart ? upper_ : lower_)->SetValue(true);
	partBox->Add(bytesRow, 0, wxBOTTOM, 4);
	partBox->Add(lower_, 0, wxBOTTOM, 2);
	partBox->Add(upper_, 0);
	grid->Add(partBox, 1, wxEXPAND);

	label(tr(UVT::BYTE_ORDER_LABEL));
	auto* orderBox = new wxBoxSizer(wxVERTICAL);
	bigEndian_ = new wxRadioButton(this, wxID_ANY, tr(UVT::ORDER_BIG_ENDIAN), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	littleEndian_ = new wxRadioButton(this, wxID_ANY, tr(UVT::ORDER_LITTLE_ENDIAN));
	(initial.order == ByteOrder::LittleEndian ? littleEndian_ : bigEndian_)->SetValue(true);
	orderBox->Add(bigEndian_, 0, wxBOTTOM, 2);
	orderBox->Add(littleEndian_, 0);
	grid->Add(orderBox, 1, wxEXPAND);
	for (wxRadioButton* r : { allBytes_, except_, lower_, upper_, bigEndian_, littleEndian_ }) r->Bind(wxEVT_RADIOBUTTON, onChange);

	top->Add(grid, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
	preview_ = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(540), FromDIP(60)), wxST_NO_AUTORESIZE);
	top->Add(preview_, 1, wxEXPAND | wxALL, 12);
	top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	SetSizerAndFit(top);

	// Bytes: after the type is known (its range depends on it).
	const int full = checksumInfo(initial.type).bytes;
	bytes_->SetRange(1, full);
	bytes_->SetValue(initial.bytes <= 0 || initial.bytes > full ? full : initial.bytes);
	refresh();
	CentreOnParent();
}

void ChecksumDialog::onTypeChanged() {
	const ChecksumInfo& info = checksumTypes()[static_cast<size_t>((std::max)(type_->GetSelection(), 0))];
	bytes_->SetRange(1, info.bytes);
	bytes_->SetValue(info.bytes);
	(info.usualOrder == ByteOrder::LittleEndian ? littleEndian_ : bigEndian_)->SetValue(true); // the usual one for this type
	refresh();
}

bool ChecksumDialog::read(ChecksumSpec& k) const {
	const ChecksumInfo& info = checksumTypes()[static_cast<size_t>((std::max)(type_->GetSelection(), 0))];
	k.enabled = enabled_->GetValue();
	k.type = info.type;
	k.insertAt = atEnd_->GetValue() ? -1 : insertAt_->GetValue();
	k.bytes = bytes_->GetValue() >= info.bytes ? 0 : bytes_->GetValue();
	k.upperPart = upper_->GetValue();
	k.order = littleEndian_->GetValue() ? ByteOrder::LittleEndian : ByteOrder::BigEndian;
	k.excluded.clear();
	if (except_->GetValue()) {
		const auto list = parseIndexList(excluded_->GetValue().utf8_string());
		if (!list) return false;
		k.excluded = *list;
	}
	return true;
}

void ChecksumDialog::refresh() {
	const bool on = enabled_->GetValue();
	for (wxWindow* w : std::initializer_list<wxWindow*>{ type_, atEnd_, allBytes_, except_, bytes_, bigEndian_, littleEndian_ }) DataEntry::SetEnabledRepainting(w, on);
	DataEntry::SetEnabledRepainting(insertAt_, on && !atEnd_->GetValue());
	DataEntry::SetEnabledRepainting(excluded_, on && except_->GetValue());
	const ChecksumInfo& info = checksumTypes()[static_cast<size_t>((std::max)(type_->GetSelection(), 0))];
	bytesOf_->SetLabel(wxString::Format(tr(UVT::CHECKSUM_BYTES_OF_FMT), info.bytes));
	const bool part = on && bytes_->GetValue() < info.bytes;
	DataEntry::SetEnabledRepainting(lower_, part);
	DataEntry::SetEnabledRepainting(upper_, part);
	DataEntry::SetEnabledRepainting(bigEndian_, on && bytes_->GetValue() > 1);
	DataEntry::SetEnabledRepainting(littleEndian_, on && bytes_->GetValue() > 1);

	ChecksumSpec k;
	wxString text;
	if (!read(k)) {
		text = tr(UVT::CHECKSUM_ERR_EXCLUDED);
		preview_->SetForegroundColour(wxColour(220, 40, 40));
	}
	else {
		preview_->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
		if (on) {
			MessageSpec spec;
			spec.format = DataFormat::Hex;
			spec.text = Utils::Hex::bytesToString(message_);
			spec.checksum = k;
			spec.checksum.enabled = true;
			const BuildResult r = build(spec, {});
			const std::vector<uint8_t> sent(r.bytes.begin() + static_cast<std::ptrdiff_t>(r.checksumAt),
				r.bytes.begin() + static_cast<std::ptrdiff_t>(r.checksumAt + r.checksumBytes));
			text = wxString::Format(tr(UVT::CHECKSUM_PREVIEW_FMT), wxString::FromUTF8(Utils::Hex::toHex(r.checksumValue, info.bytes * 2, true)),
				wxString::FromUTF8(Utils::Hex::bytesToString(sent)), static_cast<int>(r.checksumAt))
				+ "\n" + wxString::FromUTF8(Utils::Hex::bytesToString(r.bytes));
		}
	}
	preview_->SetLabel(text);
	Layout();
}

bool ChecksumDialog::TransferDataFromWindow() {
	if (!read(checksum_)) {
		wxMessageBox(tr(UVT::CHECKSUM_ERR_EXCLUDED), tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		return false;
	}
	return true;
}

// ================================================================================================
// MessageEditor
// ================================================================================================

MessageEditor::MessageEditor(wxWindow* parent, const MessageSpec& spec)
	: wxPanel(parent), counters_(spec.counters), checksum_(spec.checksum) {
	auto* top = new wxBoxSizer(wxVERTICAL);
	entry_ = new DataEntry(this, spec.format, wxString::FromUTF8(spec.text));
	entry_->SetOnChange([this] { refresh(); });
	top->Add(entry_, 0, wxEXPAND | wxBOTTOM, 8);

	top->Add(new wxStaticText(this, wxID_ANY, tr(UVT::MESSAGE_COUNTERS_LABEL)), 0, wxBOTTOM, 2);
	auto* counterRow = new wxBoxSizer(wxHORIZONTAL);
	counterList_ = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, FromDIP(70)));
	counterList_->Bind(wxEVT_LISTBOX_DCLICK, [this](wxCommandEvent&) { editCounter(counterList_->GetSelection()); });
	counterRow->Add(counterList_, 1, wxEXPAND | wxRIGHT, 6);
	auto* counterButtons = new wxBoxSizer(wxVERTICAL);
	auto* add = new wxButton(this, wxID_ANY, tr(UVT::ADD_BTN));
	auto* edit = new wxButton(this, wxID_ANY, tr(UVT::EDIT_BTN));
	auto* remove = new wxButton(this, wxID_ANY, tr(UVT::REMOVE_BTN));
	add->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { editCounter(-1); });
	edit->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { editCounter(counterList_->GetSelection()); });
	remove->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		const int sel = counterList_->GetSelection();
		if (sel < 0 || sel >= static_cast<int>(counters_.size())) return;
		counters_.erase(counters_.begin() + sel);
		refresh();
	});
	for (wxButton* b : { add, edit, remove }) counterButtons->Add(b, 0, wxEXPAND | wxBOTTOM, 2);
	counterRow->Add(counterButtons, 0);
	top->Add(counterRow, 0, wxEXPAND | wxBOTTOM, 8);

	auto* checksumRow = new wxBoxSizer(wxHORIZONTAL);
	checksumRow->Add(new wxStaticText(this, wxID_ANY, tr(UVT::MESSAGE_CHECKSUM_LABEL)), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	checksumText_ = new wxStaticText(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE | wxST_ELLIPSIZE_END);
	checksumRow->Add(checksumText_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	auto* checksumBtn = new wxButton(this, wxID_ANY, tr(UVT::CHECKSUM_BTN));
	checksumBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		ChecksumDialog dlg(this, checksum_, messageBeforeChecksum());
		if (dlg.ShowModal() != wxID_OK) return;
		checksum_ = dlg.checksum();
		refresh();
	});
	checksumRow->Add(checksumBtn, 0);
	top->Add(checksumRow, 0, wxEXPAND | wxBOTTOM, 8);

	top->Add(new wxStaticText(this, wxID_ANY, tr(UVT::MESSAGE_PREVIEW_LABEL)), 0, wxBOTTOM, 2);
	preview_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(-1, FromDIP(80)), wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
	preview_->SetFont(wxFont(9, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
	top->Add(preview_, 1, wxEXPAND);

	SetSizer(top);
	refresh();
}

MessageSpec MessageEditor::GetSpec() const {
	MessageSpec spec;
	spec.format = entry_->GetFormat();
	spec.text = entry_->GetText().utf8_string();
	spec.counters = counters_;
	spec.checksum = checksum_;
	return spec;
}

std::vector<uint8_t> MessageEditor::messageBeforeChecksum() const {
	const BuildResult r = buildWithoutChecksum(GetSpec(), {});
	return r.ok() ? r.bytes : std::vector<uint8_t>();
}

bool MessageEditor::CheckValid() {
	std::vector<uint8_t> bytes;
	if (!entry_->GetBytes(bytes)) return false;
	if (build(GetSpec(), {}).bytes.empty()) {
		wxMessageBox(tr(UVT::MESSAGE_EMPTY_MSG), tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		return false;
	}
	return true;
}

void MessageEditor::editCounter(int index) {
	const bool adding = index < 0 || index >= static_cast<int>(counters_.size());
	const size_t length = messageBeforeChecksum().size();
	CounterSpec initial;
	if (!adding) initial = counters_[static_cast<size_t>(index)]; // a new one starts at index 0

	CounterDialog dlg(this, initial, length);
	if (dlg.ShowModal() != wxID_OK) return;
	if (adding) counters_.push_back(dlg.counter());
	else counters_[static_cast<size_t>(index)] = dlg.counter();
	refresh();
}

wxString MessageEditor::DescribeRate(CounterRate mode, double rate) {
	const bool perSecond = mode == CounterRate::PerSecond;
	if (rate >= 1.0 || rate <= 0.0)
		return wxString::Format(tr(perSecond ? UVT::RATE_STEPS_PER_SECOND_FMT : UVT::RATE_STEPS_PER_MESSAGE_FMT), rateText(rate));
	return wxString::Format(tr(perSecond ? UVT::RATE_ONE_STEP_EVERY_SECONDS_FMT : UVT::RATE_ONE_STEP_EVERY_MESSAGES_FMT), rateText(1.0 / rate));
}

wxString MessageEditor::DescribeCounter(const CounterSpec& c) {
	wxString order;
	if (c.encoding == CounterEncoding::Binary && c.width > 1)
		order = ", " + tr(c.order == ByteOrder::LittleEndian ? UVT::ORDER_LITTLE_SHORT : UVT::ORDER_BIG_SHORT);
	return wxString::Format(tr(UVT::COUNTER_SUMMARY_FMT), static_cast<int>(c.index), static_cast<int>(c.index) + c.width - 1,
		encodingName(c.encoding) + order, num(c.start), num(c.end), num(c.step), DescribeRate(c.rateMode, c.rate));
}

wxString MessageEditor::DescribeChecksum(const ChecksumSpec& k) {
	if (!k.enabled) return tr(UVT::CHECKSUM_NONE);
	const ChecksumInfo& info = checksumInfo(k.type);
	const int n = k.bytes <= 0 || k.bytes > info.bytes ? info.bytes : k.bytes;
	wxString where = k.insertAt < 0 ? tr(UVT::CHECKSUM_AT_END_SHORT) : wxString::Format(tr(UVT::CHECKSUM_AT_INDEX_FMT), k.insertAt);
	wxString text = wxString::Format(tr(UVT::CHECKSUM_SUMMARY_FMT), wxString::FromUTF8(info.name), n, where);
	if (n < info.bytes) text << ", " << tr(k.upperPart ? UVT::CHECKSUM_UPPER_PART : UVT::CHECKSUM_LOWER_PART);
	if (n > 1) text << ", " << tr(k.order == ByteOrder::LittleEndian ? UVT::ORDER_LITTLE_SHORT : UVT::ORDER_BIG_SHORT);
	if (!k.excluded.empty()) text << ", " << wxString::Format(tr(UVT::CHECKSUM_EXCLUDING_FMT), wxString::FromUTF8(indexListToString(k.excluded)));
	return text;
}

void MessageEditor::refresh() {
	const int sel = counterList_->GetSelection();
	counterList_->Clear();
	for (const CounterSpec& c : counters_) counterList_->Append(DescribeCounter(c));
	if (sel >= 0 && sel < static_cast<int>(counterList_->GetCount())) counterList_->SetSelection(sel);
	checksumText_->SetLabelText(DescribeChecksum(checksum_));
	checksumText_->SetToolTip(DescribeChecksum(checksum_));

	// The first messages, one second apart (so per-second values move too).
	MessageGenerator gen(GetSpec());
	gen.restart(0);
	wxString text;
	for (int i = 0; i < 4; ++i) {
		const BuildResult r = gen.next(static_cast<uint64_t>(i) * 1000);
		if (!r.ok()) {
			text = wxString::Format(tr(UVT::PARSE_ERROR_AT_FMT), static_cast<int>(r.errorPosition) + 1, DataEntry::ErrorText(r.error));
			break;
		}
		text << wxString::Format(tr(UVT::PREVIEW_LINE_FMT), i + 1, static_cast<int>(r.bytes.size())) << "  " << hexOrText(r.bytes) << "\n";
	}
	preview_->ChangeValue(text);
}

// ================================================================================================
// AutoReplyDialog / PeriodicDialog
// ================================================================================================

namespace {
	void nameAndEnabled(wxWindow* dlg, wxSizer* top, const std::string& name, bool enabled, wxTextCtrl*& nameCtrl, wxCheckBox*& enabledCtrl) {
		auto* row = new wxBoxSizer(wxHORIZONTAL);
		row->Add(new wxStaticText(dlg, wxID_ANY, tr(UVT::NAME_LABEL)), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
		nameCtrl = new wxTextCtrl(dlg, wxID_ANY, wxString::FromUTF8(name));
		row->Add(nameCtrl, 1, wxRIGHT, 12);
		enabledCtrl = new wxCheckBox(dlg, wxID_ANY, tr(UVT::ENABLED_CHECK));
		enabledCtrl->SetValue(enabled);
		row->Add(enabledCtrl, 0, wxALIGN_CENTER_VERTICAL);
		top->Add(row, 0, wxEXPAND | wxALL, 12);
	}
}

AutoReplyDialog::AutoReplyDialog(wxWindow* parent, const AutoReplyRule& rule)
	: wxDialog(parent, wxID_ANY, tr(UVT::AUTO_REPLY_DIALOG_TITLE), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  rule_(rule) {
	auto* top = new wxBoxSizer(wxVERTICAL);
	nameAndEnabled(this, top, rule.name, rule.enabled, name_, enabled_);

	auto* patternBox = new wxStaticBoxSizer(wxVERTICAL, this, tr(UVT::AUTO_REPLY_PATTERN_LABEL));
	pattern_ = new DataEntry(patternBox->GetStaticBox(), rule.patternFormat, wxString::FromUTF8(rule.patternText));
	patternBox->Add(pattern_, 0, wxEXPAND | wxALL, 6);
	top->Add(patternBox, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

	auto* replyBox = new wxStaticBoxSizer(wxVERTICAL, this, tr(UVT::AUTO_REPLY_REPLY_LABEL));
	reply_ = new MessageEditor(replyBox->GetStaticBox(), rule.reply);
	replyBox->Add(reply_, 1, wxEXPAND | wxALL, 6);
	auto* delayRow = new wxBoxSizer(wxHORIZONTAL);
	delayRow->Add(new wxStaticText(replyBox->GetStaticBox(), wxID_ANY, tr(UVT::AUTO_REPLY_DELAY_LABEL)), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	delay_ = new wxSpinCtrl(replyBox->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 600000, rule.delayMs);
	delayRow->Add(delay_, 0);
	replyBox->Add(delayRow, 0, wxALL, 6);
	top->Add(replyBox, 1, wxEXPAND | wxALL, 12);

	top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	SetSizerAndFit(top);
	SetMinSize(FromDIP(wxSize(640, -1)));
	SetSize(wxSize(FromDIP(640), GetSize().y));
	CentreOnParent();
}

bool AutoReplyDialog::TransferDataFromWindow() {
	std::vector<uint8_t> pattern;
	if (!pattern_->GetBytes(pattern)) return false;
	if (pattern.empty()) {
		wxMessageBox(tr(UVT::AUTO_REPLY_EMPTY_PATTERN_MSG), tr(UVT::ERROR_TITLE), wxOK | wxICON_INFORMATION, this);
		return false;
	}
	if (!reply_->CheckValid()) return false;
	rule_.name = name_->GetValue().Strip(wxString::both).utf8_string();
	rule_.enabled = enabled_->GetValue();
	rule_.patternFormat = pattern_->GetFormat();
	rule_.patternText = pattern_->GetText().utf8_string();
	rule_.reply = reply_->GetSpec();
	rule_.delayMs = delay_->GetValue();
	return true;
}

PeriodicDialog::PeriodicDialog(wxWindow* parent, const PeriodicMessage& periodic)
	: wxDialog(parent, wxID_ANY, tr(UVT::PERIODIC_DIALOG_TITLE), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  periodic_(periodic) {
	auto* top = new wxBoxSizer(wxVERTICAL);
	nameAndEnabled(this, top, periodic.name, periodic.enabled, name_, enabled_);

	auto* periodRow = new wxBoxSizer(wxHORIZONTAL);
	periodRow->Add(new wxStaticText(this, wxID_ANY, tr(UVT::PERIODIC_PERIOD_LABEL)), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	period_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 10, 86400000, periodic.periodMs);
	periodRow->Add(period_, 0);
	top->Add(periodRow, 0, wxLEFT | wxRIGHT, 12);

	auto* box = new wxStaticBoxSizer(wxVERTICAL, this, tr(UVT::PERIODIC_MESSAGE_LABEL));
	message_ = new MessageEditor(box->GetStaticBox(), periodic.message);
	box->Add(message_, 1, wxEXPAND | wxALL, 6);
	top->Add(box, 1, wxEXPAND | wxALL, 12);

	top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	SetSizerAndFit(top);
	SetMinSize(FromDIP(wxSize(640, -1)));
	SetSize(wxSize(FromDIP(640), GetSize().y));
	CentreOnParent();
}

bool PeriodicDialog::TransferDataFromWindow() {
	if (!message_->CheckValid()) return false;
	periodic_.name = name_->GetValue().Strip(wxString::both).utf8_string();
	periodic_.enabled = enabled_->GetValue();
	periodic_.periodMs = period_->GetValue();
	periodic_.message = message_->GetSpec();
	return true;
}
