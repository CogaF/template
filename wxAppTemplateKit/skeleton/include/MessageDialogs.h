/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <cstdint>
#include <vector>

#include <wx/dialog.h>
#include <wx/panel.h>

#include "SerialMessage.h"

class DataEntry;
class wxCheckBox;
class wxChoice;
class wxListBox;
class wxRadioButton;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;

/*!
 * \file MessageDialogs.h
 * \brief Editing message templates (SerialMessage.h): an incrementing value, a checksum, a whole
 * message (text + values + checksum, with a live preview), an auto reply and a periodic message.
 */

/*!
 * \brief Edits one incrementing value: where it goes, its size (up to 64 bits), how it is written,
 * byte order, start / end / step and how fast it advances.
 */
class CounterDialog : public wxDialog {
public:
	/*!
	 * \param parent        parent window.
	 * \param initial       the value to edit.
	 * \param messageLength length of the message it goes into (for the explanations).
	 */
	CounterDialog(wxWindow* parent, const SerialData::CounterSpec& initial, size_t messageLength);
	/*! \brief The edited value (valid after ShowModal() returned wxID_OK). */
	const SerialData::CounterSpec& counter() const { return counter_; }

private:
	bool read(SerialData::CounterSpec& out, wxString* error) const;
	void refresh();
	bool TransferDataFromWindow() override;

	SerialData::CounterSpec counter_;
	size_t messageLength_;
	wxChoice* encoding_ = nullptr;
	wxSpinCtrl* width_ = nullptr;
	wxStaticText* unit_ = nullptr;
	wxSpinCtrl* index_ = nullptr;
	wxStaticText* span_ = nullptr;
	wxRadioButton* bigEndian_ = nullptr;
	wxRadioButton* littleEndian_ = nullptr;
	wxTextCtrl* start_ = nullptr;
	wxTextCtrl* end_ = nullptr;
	wxTextCtrl* step_ = nullptr;
	wxRadioButton* perMessage_ = nullptr;
	wxRadioButton* perSecond_ = nullptr;
	wxTextCtrl* rate_ = nullptr;
	wxStaticText* info_ = nullptr;
};

/*!
 * \brief Edits the checksum of a message: type, where it is inserted, which bytes it covers, how
 * many of its bytes are sent (lower or upper part) and in which order.
 */
class ChecksumDialog : public wxDialog {
public:
	/*!
	 * \param parent  parent window.
	 * \param initial the checksum to edit.
	 * \param message the message before the checksum (for the index range and the preview).
	 */
	ChecksumDialog(wxWindow* parent, const SerialData::ChecksumSpec& initial, std::vector<uint8_t> message);
	/*! \brief The edited checksum (valid after ShowModal() returned wxID_OK). */
	const SerialData::ChecksumSpec& checksum() const { return checksum_; }

private:
	bool read(SerialData::ChecksumSpec& out) const;
	void onTypeChanged();
	void refresh();
	bool TransferDataFromWindow() override;

	SerialData::ChecksumSpec checksum_;
	std::vector<uint8_t> message_;
	wxCheckBox* enabled_ = nullptr;
	wxChoice* type_ = nullptr;
	wxCheckBox* atEnd_ = nullptr;
	wxSpinCtrl* insertAt_ = nullptr;
	wxRadioButton* allBytes_ = nullptr;
	wxRadioButton* except_ = nullptr;
	wxTextCtrl* excluded_ = nullptr;
	wxSpinCtrl* bytes_ = nullptr;
	wxStaticText* bytesOf_ = nullptr;
	wxRadioButton* lower_ = nullptr;
	wxRadioButton* upper_ = nullptr;
	wxRadioButton* bigEndian_ = nullptr;
	wxRadioButton* littleEndian_ = nullptr;
	wxStaticText* preview_ = nullptr;
};

/*!
 * \brief A whole message template in a panel: the text (DataEntry), its incrementing values, its
 * checksum and a preview of the first messages it will produce.
 */
class MessageEditor : public wxPanel {
public:
	/*! \brief Shows spec for editing. */
	MessageEditor(wxWindow* parent, const SerialData::MessageSpec& spec);
	/*! \brief The template as edited. */
	SerialData::MessageSpec GetSpec() const;
	/*! \brief true if a message can be built from it; otherwise explains why (message box) and returns false. */
	bool CheckValid();

	/*! \brief One-line description of a counter ("Bytes 4-5 (Binary): 0 to 100, step 1, ..."). */
	static wxString DescribeCounter(const SerialData::CounterSpec& counter);
	/*! \brief One-line description of a checksum ("CRC-16/MODBUS: 2 bytes at the end, little-endian"). */
	static wxString DescribeChecksum(const SerialData::ChecksumSpec& checksum);
	/*! \brief How fast a counter advances ("one step every 10 seconds"). */
	static wxString DescribeRate(SerialData::CounterRate mode, double rate);

private:
	void refresh();
	void editCounter(int index); // -1 = add
	std::vector<uint8_t> messageBeforeChecksum() const;

	DataEntry* entry_ = nullptr;
	wxListBox* counterList_ = nullptr;
	wxStaticText* checksumText_ = nullptr;
	wxTextCtrl* preview_ = nullptr;
	std::vector<SerialData::CounterSpec> counters_;
	SerialData::ChecksumSpec checksum_;
};

/*! \brief Edits one auto reply: name, pattern, reply template, delay. */
class AutoReplyDialog : public wxDialog {
public:
	/*! \brief Shows rule for editing. */
	AutoReplyDialog(wxWindow* parent, const SerialData::AutoReplyRule& rule);
	/*! \brief The edited rule (valid after ShowModal() returned wxID_OK). */
	const SerialData::AutoReplyRule& rule() const { return rule_; }

private:
	bool TransferDataFromWindow() override;

	SerialData::AutoReplyRule rule_;
	wxTextCtrl* name_ = nullptr;
	wxCheckBox* enabled_ = nullptr;
	DataEntry* pattern_ = nullptr;
	MessageEditor* reply_ = nullptr;
	wxSpinCtrl* delay_ = nullptr;
};

/*! \brief Edits one periodic message: name, period, message template. */
class PeriodicDialog : public wxDialog {
public:
	/*! \brief Shows periodic for editing. */
	PeriodicDialog(wxWindow* parent, const SerialData::PeriodicMessage& periodic);
	/*! \brief The edited message (valid after ShowModal() returned wxID_OK). */
	const SerialData::PeriodicMessage& periodic() const { return periodic_; }

private:
	bool TransferDataFromWindow() override;

	SerialData::PeriodicMessage periodic_;
	wxTextCtrl* name_ = nullptr;
	wxCheckBox* enabled_ = nullptr;
	wxSpinCtrl* period_ = nullptr;
	MessageEditor* message_ = nullptr;
};
