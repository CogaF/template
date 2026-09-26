/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file I18n.cpp
 * \brief Implementation of I18n.h.
 */

#include "I18n.h"
#include "DataDir.h"
#include "Log.h"

#include <fstream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <wx/dir.h>

namespace {
	std::shared_mutex g_mutex;
	std::map<wxString, wxString> g_dict;
	std::string g_code = "en";

	/*! \brief Value of attribute `name="..."` in one XML line, raw (still escaped). false if absent. */
	bool attribute(const std::string& line, const std::string& name, std::string& out) {
		const std::string key = " " + name + "=\"";
		const size_t start = line.find(key);
		if (start == std::string::npos) return false;
		const size_t b = start + key.size();
		const size_t e = line.find('"', b);
		if (e == std::string::npos) return false;
		out = line.substr(b, e - b);
		return true;
	}

	wxString unescape(const std::string& raw) {
		std::string s;
		s.reserve(raw.size());
		for (size_t i = 0; i < raw.size(); ++i) {
			if (raw[i] != '&') { s += raw[i]; continue; }
			const size_t semi = raw.find(';', i);
			if (semi == std::string::npos) { s += raw[i]; continue; }
			const std::string ent = raw.substr(i + 1, semi - i - 1);
			if (ent == "amp") s += '&';
			else if (ent == "lt") s += '<';
			else if (ent == "gt") s += '>';
			else if (ent == "quot") s += '"';
			else if (ent == "apos") s += '\'';
			else if (ent == "#10") s += '\n';
			else if (ent == "#9") s += '\t';
			else if (ent == "#13") s += '\r';
			else { s += raw.substr(i, semi - i + 1); } // unknown: keep as is
			i = semi;
		}
		return wxString::FromUTF8(s);
	}

	std::filesystem::path languageFile(const std::string& code) { return DataDir::file("language-" + code + ".xml"); }
}

namespace I18n {

std::vector<Language> available() {
	std::vector<Language> out = { { "en", "English" } };
	wxDir dir(DataDir::wx());
	if (!dir.IsOpened()) return out;
	wxString name;
	for (bool more = dir.GetFirst(&name, "language-*.xml", wxDIR_FILES); more; more = dir.GetNext(&name)) {
		const std::string code = name.Mid(9, name.length() - 9 - 4).utf8_string();
		if (code.empty() || code == "en") continue;
		wxString display = wxString::FromUTF8(code);
		std::ifstream in(languageFile(code));
		std::string line, raw;
		for (int i = 0; i < 5 && std::getline(in, line); ++i) {
			if (line.find("<translation") != std::string::npos && attribute(line, "name", raw)) { display = unescape(raw); break; }
		}
		out.push_back({ code, display });
	}
	return out;
}

bool load(const std::string& code) {
	if (code.empty() || code == "en") {
		std::unique_lock lock(g_mutex);
		g_dict.clear();
		g_code = "en";
		return true;
	}
	std::ifstream in(languageFile(code));
	if (!in.is_open()) {
		Log::warning("I18n::load(): language file for \"" + code + "\" not found - using English.");
		load("en");
		return false;
	}
	// Parsed into a local map first: a broken file never leaves a half-loaded mix of two languages.
	std::map<wxString, wxString> parsed;
	std::string line, key, value;
	bool first = true;
	while (std::getline(in, line)) {
		if (first && line.size() >= 3 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3); // UTF-8 BOM
		first = false;
		if (line.find("<entry") == std::string::npos) continue;
		if (!attribute(line, "key", key) || !attribute(line, "value", value)) {
			Log::warning("I18n::load(): malformed entry in language-" + code + ".xml: " + line);
			continue;
		}
		parsed[unescape(key)] = unescape(value);
	}
	std::unique_lock lock(g_mutex);
	g_dict = std::move(parsed);
	g_code = code;
	Log::info("I18n::load(): language \"" + code + "\" loaded, " + std::to_string(g_dict.size()) + " entries.");
	return true;
}

std::string current() {
	std::shared_lock lock(g_mutex);
	return g_code;
}

wxString tr(const wxString& englishText) {
	std::shared_lock lock(g_mutex);
	const auto it = g_dict.find(englishText);
	return (it != g_dict.end() && !it->second.empty()) ? it->second : englishText;
}

} // namespace I18n
