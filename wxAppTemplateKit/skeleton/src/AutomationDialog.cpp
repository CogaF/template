/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file AutomationDialog.cpp
 * \brief Implementation of AutomationDialog.h.
 */

#include "AutomationDialog.h"
#include "DataEntry.h"
#include "I18n.h"
#include "MessageDialogs.h"
#include "UVT.h"

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

using namespace SerialData;

namespace {
	/*! \brief "Hex: 01 03 00 00" - a text as the user wrote it, with its notation. */
	wxString written(DataFormat format, const std::string& text) {
		return DataEntry::FormatName(format) + ": " + wxString::FromUTF8(text);
	}

	long selectedRow(wxListCtrl* list) { return list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED); }

	/*!
	 * \brief A tab: help line, list with tick boxes, and Add / Edit / Duplicate / Remove buttons
	 * that call the given functions with the selected row (-1 = none).
	 */
	wxListCtrl* makePage(wxNotebook* book, const wxString& title, const wxString& help, const std::vector<wxString>& columns,
		std::function<void(long)> add, std::function<void(long)> edit, std::function<void(long)> duplicate, std::function<void(long)> remove) {
		auto* page = new wxPanel(book);
		auto* top = new wxBoxSizer(wxVERTICAL);
		top->Add(new wxStaticText(page, wxID_ANY, help), 0, wxEXPAND | wxALL, 8);
		auto* list = new wxListCtrl(page, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
		list->EnableCheckBoxes();
		const int widths[] = { 150, 190, 190, 80 };
		for (size_t i = 0; i < columns.size(); ++i) list->AppendColumn(columns[i], wxLIST_FORMAT_LEFT, page->FromDIP(widths[i < 4 ? i : 3]));
		list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [edit](wxListEvent& e) { edit(e.GetIndex()); });
		auto* row = new wxBoxSizer(wxHORIZONTAL);
		row->Add(list, 1, wxEXPAND | wxRIGHT, 6);
		auto* buttons = new wxBoxSizer(wxVERTICAL);
		auto button = [&](const wxString& label, std::function<void(long)> fn) {
			auto* b = new wxButton(page, wxID_ANY, label);
			b->Bind(wxEVT_BUTTON, [list, fn](wxCommandEvent&) { fn(selectedRow(list)); });
			buttons->Add(b, 0, wxEXPAND | wxBOTTOM, 4);
		};
		button(tr(UVT::ADD_BTN), add);
		button(tr(UVT::EDIT_BTN), edit);
		button(tr(UVT::DUPLICATE_BTN), duplicate);
		button(tr(UVT::REMOVE_BTN), remove);
		row->Add(buttons, 0);
		top->Add(row, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
		page->SetSizer(top);
		book->AddPage(page, title);
		return list;
	}

	void selectRow(wxListCtrl* list, long row) {
		if (row < 0 || row >= list->GetItemCount()) return;
		list->SetItemState(row, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
		list->EnsureVisible(row);
	}
}

AutomationDialog::AutomationDialog(wxWindow* parent, std::vector<AutoReplyRule> replies, std::vector<PeriodicMessage> periodic, Page page)
	: wxDialog(parent, wxID_ANY, tr(UVT::AUTOMATION_TITLE), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  replies_(std::move(replies)), periodic_(std::move(periodic)) {
	auto* book = new wxNotebook(this, wxID_ANY);

	auto removeAsk = [this](const std::string& name) {
		return wxMessageBox(wxString::Format(tr(UVT::REMOVE_CONFIRM_FMT), wxString::FromUTF8(name)), tr(UVT::REMOVE_BTN), wxYES_NO | wxICON_QUESTION, this) == wxYES;
	};

	replyList_ = makePage(book, tr(UVT::AUTO_REPLIES_TAB), tr(UVT::AUTO_REPLIES_HELP),
		{ tr(UVT::COL_NAME), tr(UVT::COL_WHEN_RECEIVED), tr(UVT::COL_REPLY), tr(UVT::COL_DELAY_MS) },
		[this](long) { editReply(-1); },
		[this](long row) { if (row >= 0) editReply(row); },
		[this](long row) {
			if (row < 0) return;
			replies_.insert(replies_.begin() + row + 1, replies_[static_cast<size_t>(row)]);
			fillReplies();
			selectRow(replyList_, row + 1);
		},
		[this, removeAsk](long row) {
			if (row < 0 || !removeAsk(replies_[static_cast<size_t>(row)].name)) return;
			replies_.erase(replies_.begin() + row);
			fillReplies();
		});
	periodicList_ = makePage(book, tr(UVT::PERIODIC_TAB), tr(UVT::PERIODIC_HELP),
		{ tr(UVT::COL_NAME), tr(UVT::COL_MESSAGE), tr(UVT::COL_EVERY_MS) },
		[this](long) { editPeriodic(-1); },
		[this](long row) { if (row >= 0) editPeriodic(row); },
		[this](long row) {
			if (row < 0) return;
			periodic_.insert(periodic_.begin() + row + 1, periodic_[static_cast<size_t>(row)]);
			fillPeriodic();
			selectRow(periodicList_, row + 1);
		},
		[this, removeAsk](long row) {
			if (row < 0 || !removeAsk(periodic_[static_cast<size_t>(row)].name)) return;
			periodic_.erase(periodic_.begin() + row);
			fillPeriodic();
		});

	auto onCheck = [this](wxListCtrl* list, bool on) {
		return [this, list, on](wxListEvent& e) {
			if (filling_) return;
			const auto row = static_cast<size_t>(e.GetIndex());
			if (list == replyList_ && row < replies_.size()) replies_[row].enabled = on;
			if (list == periodicList_ && row < periodic_.size()) periodic_[row].enabled = on;
		};
	};
	replyList_->Bind(wxEVT_LIST_ITEM_CHECKED, onCheck(replyList_, true));
	replyList_->Bind(wxEVT_LIST_ITEM_UNCHECKED, onCheck(replyList_, false));
	periodicList_->Bind(wxEVT_LIST_ITEM_CHECKED, onCheck(periodicList_, true));
	periodicList_->Bind(wxEVT_LIST_ITEM_UNCHECKED, onCheck(periodicList_, false));

	fillReplies();
	fillPeriodic();
	book->SetSelection(page == Page::Periodic ? 1 : 0);

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(book, 1, wxEXPAND | wxALL, 8);
	top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
	SetSizer(top);
	SetSize(FromDIP(wxSize(760, 420)));
	CentreOnParent();
}

void AutomationDialog::fillReplies() {
	filling_ = true;
	replyList_->DeleteAllItems();
	for (size_t i = 0; i < replies_.size(); ++i) {
		const AutoReplyRule& r = replies_[i];
		const long row = replyList_->InsertItem(static_cast<long>(i), wxString::FromUTF8(r.name));
		replyList_->SetItem(row, 1, written(r.patternFormat, r.patternText));
		replyList_->SetItem(row, 2, written(r.reply.format, r.reply.text));
		replyList_->SetItem(row, 3, wxString::Format("%d", r.delayMs));
		replyList_->CheckItem(row, r.enabled);
	}
	filling_ = false;
}

void AutomationDialog::fillPeriodic() {
	filling_ = true;
	periodicList_->DeleteAllItems();
	for (size_t i = 0; i < periodic_.size(); ++i) {
		const PeriodicMessage& p = periodic_[i];
		const long row = periodicList_->InsertItem(static_cast<long>(i), wxString::FromUTF8(p.name));
		periodicList_->SetItem(row, 1, written(p.message.format, p.message.text));
		periodicList_->SetItem(row, 2, wxString::Format("%d", p.periodMs));
		periodicList_->CheckItem(row, p.enabled);
	}
	filling_ = false;
}

void AutomationDialog::editReply(long row) {
	AutoReplyRule initial;
	if (row >= 0) initial = replies_[static_cast<size_t>(row)];
	else initial.name = wxString::Format(tr(UVT::DEFAULT_AUTO_REPLY_NAME_FMT), static_cast<int>(replies_.size()) + 1).utf8_string();
	AutoReplyDialog dlg(this, initial);
	if (dlg.ShowModal() != wxID_OK) return;
	if (row >= 0) replies_[static_cast<size_t>(row)] = dlg.rule();
	else { replies_.push_back(dlg.rule()); row = static_cast<long>(replies_.size()) - 1; }
	fillReplies();
	selectRow(replyList_, row);
}

void AutomationDialog::editPeriodic(long row) {
	PeriodicMessage initial;
	if (row >= 0) initial = periodic_[static_cast<size_t>(row)];
	else initial.name = wxString::Format(tr(UVT::DEFAULT_PERIODIC_NAME_FMT), static_cast<int>(periodic_.size()) + 1).utf8_string();
	PeriodicDialog dlg(this, initial);
	if (dlg.ShowModal() != wxID_OK) return;
	if (row >= 0) periodic_[static_cast<size_t>(row)] = dlg.periodic();
	else { periodic_.push_back(dlg.periodic()); row = static_cast<long>(periodic_.size()) - 1; }
	fillPeriodic();
	selectRow(periodicList_, row);
}
