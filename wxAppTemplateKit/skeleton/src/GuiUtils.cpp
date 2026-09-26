/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file GuiUtils.cpp
 * \brief Implementation of GuiUtils.h.
 */

#include "GuiUtils.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/radiobut.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // std::min/std::max, not the windows.h macros
#endif
#include <windows.h>
#endif

namespace Utils::Gui {

wxString padText(const wxString& text, size_t width, char align) {
	if (text.length() >= width) return text;
	const size_t fill = width - text.length();
	if (align == '>') return wxString(' ', fill) + text;
	if (align == '^') return wxString(' ', fill / 2) + text + wxString(' ', fill - fill / 2);
	return text + wxString(' ', fill);
}

wxColour colourByIndex(int index) {
	using namespace Colours;
	static const std::vector<wxColour> palette = { Red, Green, Blue, Orange, Purple, Cyan, Pink, Brown, Teal, Olive,
		Magenta, DarkGreen, Navy, Gold, LightBlue, Indigo, Crimson, YellowGreen, Chocolate, DeepSkyBlue, DeepPink,
		SeaGreen, CornflowerBlue, Orchid, SlateGray, SandyBrown, MediumSlateBlue, Gray };
	if (index < 0) index = 0;
	return palette[static_cast<size_t>(index) % palette.size()];
}

void setInteractiveEnabled(wxWindow* root, bool enabled, wxWindowID keepId) {
	if (!root) return;
	for (wxWindow* child : root->GetChildren()) {
		if (keepId != wxID_NONE && child->GetId() == keepId) continue;
		if (wxDynamicCast(child, wxButton) || wxDynamicCast(child, wxTextCtrl) || wxDynamicCast(child, wxSpinCtrl) ||
			wxDynamicCast(child, wxSpinCtrlDouble) || wxDynamicCast(child, wxChoice) || wxDynamicCast(child, wxCheckBox) ||
			wxDynamicCast(child, wxComboBox) || wxDynamicCast(child, wxRadioButton) || wxDynamicCast(child, wxSlider)) {
			child->Enable(enabled);
		}
		setInteractiveEnabled(child, enabled, keepId);
	}
}

void appendToConsole(wxTextCtrl* console, const wxString& text, int maxLines, bool followNewest) {
	if (!console) return;
	console->Freeze();

	long topPos = 0;
	const bool haveTop = !followNewest && console->HitTest(wxPoint(3, 3), &topPos) != wxTE_HT_UNKNOWN;
	long selFrom = 0, selTo = 0;
	if (!followNewest) console->GetSelection(&selFrom, &selTo);

	console->AppendText(text);

	long removedChars = 0;
	while (console->GetNumberOfLines() > maxLines) {
		const long lineLength = console->GetLineLength(0);
		console->Remove(0, lineLength + 1); // +1: the newline too
		removedChars += lineLength + 1;
	}

	if (followNewest) {
		scrollToEnd(console);
	}
	else {
		// Selection first (it may scroll the caret into view), then put the view back where it was.
		console->SetSelection(std::max(0L, selFrom - removedChars), std::max(0L, selTo - removedChars));
		if (haveTop) {
			long wantX = 0, wantLine = 0, curPos = 0, curX = 0, curLine = 0;
			if (console->PositionToXY(std::max(0L, topPos - removedChars), &wantX, &wantLine) &&
				console->HitTest(wxPoint(3, 3), &curPos) != wxTE_HT_UNKNOWN &&
				console->PositionToXY(curPos, &curX, &curLine) && wantLine != curLine) {
				console->ScrollLines(static_cast<int>(wantLine - curLine));
			}
		}
	}
	console->Thaw();
}

void scrollToEnd(wxTextCtrl* console) {
	if (!console) return;
	console->SetInsertionPointEnd();
	console->ShowPosition(console->GetLastPosition());
}

void attachDebugConsole() {
#ifdef _WIN32
	if (!AllocConsole()) return; // already has one
	FILE* stream = nullptr;
	freopen_s(&stream, "CONOUT$", "w", stdout);
	freopen_s(&stream, "CONOUT$", "w", stderr);
	freopen_s(&stream, "CONIN$", "r", stdin);
	SetConsoleOutputCP(CP_UTF8); // UTF-8 text prints correctly
#endif
}

void clearDebugConsole() {
#ifdef _WIN32
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (out == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(out, &info)) return;
	const DWORD cells = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
	DWORD written = 0;
	const COORD home = { 0, 0 };
	FillConsoleOutputCharacterW(out, L' ', cells, home, &written);
	FillConsoleOutputAttribute(out, info.wAttributes, cells, home, &written);
	SetConsoleCursorPosition(out, home);
#endif
}

} // namespace Utils::Gui
