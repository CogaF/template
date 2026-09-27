/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <cstdint>
#include <functional>

#include <wx/dialog.h>

class wxListCtrl;
class wxListEvent;
class wxStaticText;

/*!
 * \file AsciiTableDialog.h
 * \brief The table of the 256 byte values - decimal, hex, character, name and description - to
 * insert characters (control characters included) into a DataEntry.
 *
 * - Double-click (or Enter): adds the character and closes the table.
 * - Right-click: offers to add it and keeps the table open, to add several characters in a row.
 * The characters added so far are shown at the bottom.
 */
/*!
 * \brief Character table dialog (see the file comment).
 */
class AsciiTableDialog : public wxDialog {
public:
	/*!
	 * \param parent parent window.
	 * \param add    called with every byte the user adds (on the GUI thread, while the dialog is open).
	 */
	AsciiTableDialog(wxWindow* parent, std::function<void(uint8_t)> add);

private:
	void addByte(uint8_t b);
	void onRightClick(wxListEvent& event);

	std::function<void(uint8_t)> add_;
	wxListCtrl* list_ = nullptr;
	wxStaticText* added_ = nullptr;
	std::string addedText_;
};
