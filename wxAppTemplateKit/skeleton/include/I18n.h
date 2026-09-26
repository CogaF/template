// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#pragma once

#include <string>
#include <vector>
#include <wx/string.h>

/*!
 * \file I18n.h
 * \brief Run-time translation of user-visible text.
 *
 * Every text shown to the user is written in English in UVT.h and shown through tr(). A language is
 * a file "<data folder>/language-<code>.xml" (see languages/language-it.xml):
 *
 *     <?xml version="1.0" encoding="UTF-8"?>
 *     <translation language="it" name="Italiano">
 *         <entry key="English text" value="Testo tradotto"/>
 *     </translation>
 *
 * One <entry> per line; the key is the exact English text (with &amp; &lt; &gt; &quot; &apos; and
 * &#10; (newline) / &#9; (tab) escapes). A missing OR empty translation shows the English text, so
 * a new string never shows up blank. Adding a language needs no rebuild - drop a new file in.
 *
 * Keep translated text in wxString end to end: converting it to std::string (ToStdString()) uses the
 * C locale and returns an EMPTY string for non-Latin scripts (Arabic, Russian, Chinese, ...). Use
 * utf8_string() when a std::string is unavoidable (files, SQLite) and wxString::FromUTF8() back.
 */
namespace I18n {
	struct Language { std::string code; wxString displayName; };

	//! "en" (built in) plus every language-<code>.xml found in the data folder.
	std::vector<Language> available();
	//! Loads language-<code>.xml ("en" = English, no file). On any error keeps English and returns false.
	bool load(const std::string& code);
	//! Code of the language in use ("en" if none was loaded).
	std::string current();
	//! The translation of englishText in the current language, or englishText itself.
	wxString tr(const wxString& englishText);
}

//! Shortcut used everywhere: tr(UVT::SOME_TEXT).
inline wxString tr(const wxString& englishText) { return I18n::tr(englishText); }
