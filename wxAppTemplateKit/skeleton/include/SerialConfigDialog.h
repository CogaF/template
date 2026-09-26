// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <wx/dialog.h>

#include "SerialConfig.h"

class wxComboBox;
class wxChoice;
class wxSpinCtrl;

/*!
 * \file SerialConfigDialog.h
 * \brief Dialog to pick a port and all its settings. The port box lists the detected ports but also
 * accepts a typed name (a port that is not plugged in right now keeps its setting).
 */
class SerialConfigDialog : public wxDialog {
public:
	SerialConfigDialog(wxWindow* parent, const SerialConfig& initial);
	//! The edited settings (valid after ShowModal() returned wxID_OK).
	SerialConfig config() const;

private:
	void fillPorts(const std::string& select);

	wxComboBox* port_ = nullptr;
	wxComboBox* baud_ = nullptr;
	wxChoice* dataBits_ = nullptr;
	wxChoice* parity_ = nullptr;
	wxChoice* stopBits_ = nullptr;
	wxChoice* flow_ = nullptr;
	wxSpinCtrl* interByte_ = nullptr;
	wxSpinCtrl* readTimeout_ = nullptr;
	wxSpinCtrl* writeTimeout_ = nullptr;
	SerialConfig initial_;
};
