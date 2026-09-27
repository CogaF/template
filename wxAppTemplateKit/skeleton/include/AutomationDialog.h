/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <vector>

#include <wx/dialog.h>

#include "SerialMessage.h"

class wxListCtrl;
class wxNotebook;

/*!
 * \file AutomationDialog.h
 * \brief The lists of auto replies and periodic messages, on two tabs: add, edit (double-click),
 * duplicate, remove, and enable / disable with the tick box of each row.
 */
/*!
 * \brief Auto replies and periodic messages lists (see the file comment).
 */
class AutomationDialog : public wxDialog {
public:
	/*! \brief The tab shown first. */
	enum class Page { AutoReplies, Periodic };

	/*!
	 * \param parent   parent window.
	 * \param replies  the auto replies to edit.
	 * \param periodic the periodic messages to edit.
	 * \param page     the tab shown first.
	 */
	AutomationDialog(wxWindow* parent, std::vector<SerialData::AutoReplyRule> replies,
		std::vector<SerialData::PeriodicMessage> periodic, Page page);

	/*! \brief The edited auto replies (after ShowModal() returned wxID_OK). */
	const std::vector<SerialData::AutoReplyRule>& autoReplies() const { return replies_; }
	/*! \brief The edited periodic messages (after ShowModal() returned wxID_OK). */
	const std::vector<SerialData::PeriodicMessage>& periodic() const { return periodic_; }

private:
	void fillReplies();
	void fillPeriodic();
	void editReply(long row);    // -1 = add
	void editPeriodic(long row); // -1 = add

	std::vector<SerialData::AutoReplyRule> replies_;
	std::vector<SerialData::PeriodicMessage> periodic_;
	wxListCtrl* replyList_ = nullptr;
	wxListCtrl* periodicList_ = nullptr;
	bool filling_ = false; // CheckItem() while filling must not count as the user's click
};
