/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file StatusLed.cpp
 * \brief Implementation of StatusLed.h.
 */

#include "StatusLed.h"

#include <algorithm>
#include <wx/dcbuffer.h>

wxDEFINE_EVENT(EVT_STATUS_LED_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUS_LED_DOUBLE_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(EVT_STATUS_LED_RIGHT_CLICKED, wxCommandEvent);

StatusLed::StatusLed(wxWindow* parent, wxWindowID id, State state, const wxSize& size)
	: wxPanel(parent, id, wxDefaultPosition, parent ? parent->FromDIP(size) : size, wxBORDER_NONE), state_(state) {
	SetBackgroundStyle(wxBG_STYLE_PAINT); // required by wxAutoBufferedPaintDC
	SetMinSize(GetSize());
	Bind(wxEVT_PAINT, &StatusLed::onPaint, this);
	Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) { sendEvent(EVT_STATUS_LED_CLICKED); e.Skip(); });
	Bind(wxEVT_LEFT_DCLICK, [this](wxMouseEvent& e) { sendEvent(EVT_STATUS_LED_DOUBLE_CLICKED); e.Skip(); });
	Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent& e) { sendEvent(EVT_STATUS_LED_RIGHT_CLICKED); e.Skip(); });
}

void StatusLed::SetState(State state) {
	if (state_ == state) return;
	state_ = state;
	Refresh();
}

void StatusLed::SetColour(const wxColour& colour) {
	custom_ = colour;
	state_ = State::Custom;
	Refresh();
}

void StatusLed::SetHint(const wxString& hint) { SetToolTip(hint); }

void StatusLed::SetLabelText(const wxString& text) {
	label_ = text;
	Refresh();
}

void StatusLed::sendEvent(const wxEventType& type) {
	wxCommandEvent event(type, GetId());
	event.SetEventObject(this);
	ProcessWindowEvent(event); // handled here or by a parent (command events propagate)
}

void StatusLed::onPaint(wxPaintEvent&) {
	const wxSize client = GetClientSize();
	// A Refresh() can arrive before the first layout gave the control a size - painting a 0-sized
	// buffered DC asserts ("invalid bitmap size"), so skip it.
	if (client.GetWidth() <= 0 || client.GetHeight() <= 0) return;
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetParent() ? GetParent()->GetBackgroundColour() : GetBackgroundColour()));
	dc.Clear();

	wxColour colour;
	switch (state_) {
	case State::Green:  colour = wxColour(0, 200, 0); break;
	case State::Red:    colour = wxColour(230, 0, 0); break;
	case State::Yellow: colour = wxColour(255, 215, 0); break;
	case State::Blue:   colour = wxColour(0, 110, 255); break;
	case State::Orange: colour = wxColour(255, 140, 0); break;
	case State::Brown:  colour = wxColour(165, 42, 42); break;
	case State::Custom: colour = custom_; break;
	default:            colour = wxColour(190, 190, 190); break;
	}
	dc.SetBrush(wxBrush(colour));
	dc.SetPen(wxPen(wxColour(128, 128, 128), 1));
	dc.DrawRectangle(0, 0, client.GetWidth(), client.GetHeight());

	if (!label_.empty()) {
		wxFont font = GetFont();
		font.SetPixelSize(wxSize(0, std::max(6, client.GetHeight() * 2 / 3)));
		dc.SetFont(font);
		dc.SetTextForeground(colour.GetLuminance() > 0.6 ? *wxBLACK : *wxWHITE);
		const wxSize t = dc.GetTextExtent(label_);
		dc.DrawText(label_, (client.GetWidth() - t.GetWidth()) / 2, (client.GetHeight() - t.GetHeight()) / 2);
	}
}
