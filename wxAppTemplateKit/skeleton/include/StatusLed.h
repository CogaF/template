/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <wx/panel.h>

/*!
 * \file StatusLed.h
 * \brief A small coloured "LED" square for showing a state (port open, alarm, running, ...).
 */

/*! \brief Sent (to the LED itself) when the LED is clicked; event.GetEventObject() is the StatusLed. */
wxDECLARE_EVENT(EVT_STATUS_LED_CLICKED, wxCommandEvent);
/*! \brief Sent when the LED is double-clicked. */
wxDECLARE_EVENT(EVT_STATUS_LED_DOUBLE_CLICKED, wxCommandEvent);
/*! \brief Sent when the LED is right-clicked (e.g. to show a context menu). */
wxDECLARE_EVENT(EVT_STATUS_LED_RIGHT_CLICKED, wxCommandEvent);

/*!
 * \brief Coloured status indicator.
 *
 *     auto* led = new StatusLed(panel);
 *     led->SetState(StatusLed::State::Green);
 *     led->SetHint("Port open");                         // tooltip
 *     led->Bind(EVT_STATUS_LED_CLICKED, [](wxCommandEvent&) { ... });
 *
 * The events are sent to the LED itself and propagate to its parents like any command event, so
 * they can be bound on the LED or on a parent window.
 */
class StatusLed : public wxPanel {
public:
	/*! \brief The fixed colours; Custom uses the colour given to SetColour(). */
	enum class State { Off, Green, Red, Yellow, Blue, Orange, Brown, Custom };

	/*!
	 * \brief Creates the LED.
	 * \param parent the parent window.
	 * \param id     window id (used in the click events).
	 * \param state  initial colour.
	 * \param size the square's size in pixels (scaled for high-DPI screens).
	 */
	StatusLed(wxWindow* parent, wxWindowID id = wxID_ANY, State state = State::Off, const wxSize& size = wxSize(15, 15));

	/*! \brief Changes the colour (repaints only if it changed). */
	void SetState(State state);
	/*! \brief The current state. */
	State GetState() const { return state_; }
	/*! \brief true unless the state is Off. */
	bool IsOn() const { return state_ != State::Off; }
	/*! \brief Switches to State::Custom painted with colour. */
	void SetColour(const wxColour& colour);
	/*! \brief Tooltip text (keep it a wxString - translated text must not pass through std::string). */
	void SetHint(const wxString& hint);
	/*! \brief Optional short text drawn over the LED (e.g. "1", "A"). */
	void SetLabelText(const wxString& text);

private:
	void onPaint(wxPaintEvent& event);
	void sendEvent(const wxEventType& type);

	State state_;
	wxColour custom_ = *wxWHITE;
	wxString label_;
};
