/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <wx/colour.h>
#include <wx/string.h>
#include <wx/window.h>

class wxTextCtrl;

/*!
 * \file GuiUtils.h
 * \brief wxWidgets helpers: text padding, a colour palette for series, enabling/disabling whole
 * panels, a bounded console text control, and a debug console window.
 */
namespace Utils::Gui {

	/*!
	 * \brief Pads text with spaces to width characters: align '<' left, '>' right, '^' centred
	 * (like std::format's "{:>N}"), working on the wxString itself.
	 *
	 * Never pad (or store) translated text through std::string / ToStdString(): that conversion uses
	 * the C locale and returns an EMPTY string for non-Latin scripts (Arabic, Russian, Chinese, ...).
	 */
	wxString padText(const wxString& text, size_t width, char align = '<');

	/*! \brief Named colours of the series palette (good contrast on light and dark backgrounds). */
	namespace Colours {
		inline const wxColour Red(255, 0, 0); /*!< Red (RGB 255, 0, 0) */
		inline const wxColour Green(0, 170, 0); /*!< Green (RGB 0, 170, 0) */
		inline const wxColour Blue(0, 90, 255); /*!< Blue (RGB 0, 90, 255) */
		inline const wxColour Orange(255, 165, 0); /*!< Orange (RGB 255, 165, 0) */
		inline const wxColour Purple(128, 0, 128); /*!< Purple (RGB 128, 0, 128) */
		inline const wxColour Cyan(0, 190, 190); /*!< Cyan (RGB 0, 190, 190) */
		inline const wxColour Pink(255, 105, 180); /*!< Pink (RGB 255, 105, 180) */
		inline const wxColour Brown(165, 42, 42); /*!< Brown (RGB 165, 42, 42) */
		inline const wxColour Teal(0, 128, 128); /*!< Teal (RGB 0, 128, 128) */
		inline const wxColour Olive(128, 128, 0); /*!< Olive (RGB 128, 128, 0) */
		inline const wxColour Magenta(255, 0, 255); /*!< Magenta (RGB 255, 0, 255) */
		inline const wxColour DarkGreen(0, 128, 0); /*!< DarkGreen (RGB 0, 128, 0) */
		inline const wxColour Navy(0, 0, 128); /*!< Navy (RGB 0, 0, 128) */
		inline const wxColour Gold(255, 215, 0); /*!< Gold (RGB 255, 215, 0) */
		inline const wxColour LightBlue(135, 206, 250); /*!< LightBlue (RGB 135, 206, 250) */
		inline const wxColour Indigo(75, 0, 130); /*!< Indigo (RGB 75, 0, 130) */
		inline const wxColour Crimson(220, 20, 60); /*!< Crimson (RGB 220, 20, 60) */
		inline const wxColour YellowGreen(154, 205, 50); /*!< YellowGreen (RGB 154, 205, 50) */
		inline const wxColour Chocolate(210, 105, 30); /*!< Chocolate (RGB 210, 105, 30) */
		inline const wxColour DeepSkyBlue(0, 191, 255); /*!< DeepSkyBlue (RGB 0, 191, 255) */
		inline const wxColour DeepPink(255, 20, 147); /*!< DeepPink (RGB 255, 20, 147) */
		inline const wxColour SeaGreen(46, 139, 87); /*!< SeaGreen (RGB 46, 139, 87) */
		inline const wxColour CornflowerBlue(100, 149, 237); /*!< CornflowerBlue (RGB 100, 149, 237) */
		inline const wxColour Orchid(218, 112, 214); /*!< Orchid (RGB 218, 112, 214) */
		inline const wxColour SlateGray(112, 128, 144); /*!< SlateGray (RGB 112, 128, 144) */
		inline const wxColour SandyBrown(244, 164, 96); /*!< SandyBrown (RGB 244, 164, 96) */
		inline const wxColour MediumSlateBlue(123, 104, 238); /*!< MediumSlateBlue (RGB 123, 104, 238) */
		inline const wxColour Gray(128, 128, 128); /*!< Gray (RGB 128, 128, 128) */
	}
	/*! \brief A distinct colour for series number index (0, 1, 2, ...), repeating after the palette ends. */
	wxColour colourByIndex(int index);

	/*!
	 * \brief Enables or disables every interactive control (buttons, text boxes, spins, choices,
	 * check/radio boxes, sliders) inside root, recursively. The window with id keepId (and all it
	 * contains) is left alone - e.g. a Close button while a long operation runs.
	 */
	void setInteractiveEnabled(wxWindow* root, bool enabled, wxWindowID keepId = wxID_NONE);

	/*!
	 * \brief Appends text to a read-only multiline console, keeping at most maxLines lines.
	 *
	 * followNewest = true keeps the view on the last line. followNewest = false keeps the line at the
	 * top of the view and the user's selection where they were (shifted by whatever was trimmed
	 * above), so a message can be read or copied while new ones keep arriving - the scroll bar does
	 * not jump. Painting is frozen during the operation.
	 */
	void appendToConsole(wxTextCtrl* console, const wxString& text, int maxLines, bool followNewest = true);
	/*! \brief Moves the view of console to its last line. */
	void scrollToEnd(wxTextCtrl* console);

	/*!
	 * \brief Opens a console window next to the GUI and routes stdout/stderr to it (Windows; does
	 * nothing elsewhere). Useful in Debug builds for printf-style tracing.
	 */
	void attachDebugConsole();
	/*! \brief Clears the console opened by attachDebugConsole() (Windows). */
	void clearDebugConsole();
}
