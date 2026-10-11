/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file AboutDialog.cpp
 * \brief Implementation of AboutDialog.h.
 */

#include "AboutDialog.h"
#include "AppInfo.h"
#include "BuildInfo.h"
#include "DataDir.h"
#include "I18n.h"
#include "UVT.h"
#include "Version.h"

#include "sqlite3.h"

#include <cctype>
#include <filesystem>
#include <vector>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/filename.h>
#include <wx/hyperlink.h>
#include <wx/icon.h>
#include <wx/log.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/utils.h>
#include <wx/version.h>

namespace {
	wxString u(const char* text) { return wxString::FromUTF8(text); }
	wxString u(const std::string& text) { return wxString::FromUTF8(text); }
	bool has(const char* text) { return text && *text; }

	/*! \brief A read-only multi-line text box. */
	wxTextCtrl* readOnlyText(wxWindow* parent, const wxString& text, const wxFont& font = wxNullFont) {
		auto* box = new wxTextCtrl(parent, wxID_ANY, text, wxDefaultPosition, wxDefaultSize,
			wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxBORDER_NONE);
		if (font.IsOk()) box->SetFont(font);
		box->SetInsertionPoint(0); // show the start, not the end
		return box;
	}

	/*! \brief A label in a bold font scaled by scale (the font is set before the text, so the label is measured with it). */
	wxStaticText* bold(wxWindow* parent, const wxString& text, float scale = 1.0f) {
		auto* t = new wxStaticText(parent, wxID_ANY, wxString());
		t->SetFont(t->GetFont().Bold().Scaled(scale));
		t->SetLabel(text);
		return t;
	}

	/*! \brief A section title. */
	wxStaticText* heading(wxWindow* parent, const wxString& text) { return bold(parent, text, 1.15f); }

	/*! \brief "3.3.1" - the wxWidgets version running (the DLL's, for a DLL build). */
	wxString wxRunningVersion() {
		const wxVersionInfo v = wxGetLibraryVersionInfo();
		return wxString::Format("%d.%d.%d", v.GetMajor(), v.GetMinor(), v.GetMicro());
	}

	/*! \brief A clickable link (web, mailto or file). */
	wxHyperlinkCtrl* link(wxWindow* parent, const wxString& label, const wxString& url) {
		auto* l = new wxHyperlinkCtrl(parent, wxID_ANY, label, url);
		l->SetToolTip(url);
		return l;
	}

	/*! \brief A link that opens a folder in Explorer (created first if it does not exist yet). */
	wxHyperlinkCtrl* folderLink(wxWindow* parent, const std::filesystem::path& folder) {
		const wxString path(folder.wstring());
		auto* l = new wxHyperlinkCtrl(parent, wxID_ANY, path, "file:///" + path);
		l->Bind(wxEVT_HYPERLINK, [path](wxHyperlinkEvent&) {
			if (!wxDirExists(path)) wxFileName::Mkdir(path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
			wxLaunchDefaultApplication(path);
		});
		return l;
	}

	/*! \brief A mailto: link to AppInfo::kContact with subject and body, percent-encoded. */
	wxString mailto(const wxString& subject, const wxString& body) {
		auto encode = [](const wxString& s) {
			wxString out;
			const wxScopedCharBuffer utf8 = s.utf8_str();
			for (const char* p = utf8.data(); *p; ++p) {
				const unsigned char c = static_cast<unsigned char>(*p);
				if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') out += static_cast<char>(c);
				else out += wxString::Format("%%%02X", c);
			}
			return out;
		};
		return "mailto:" + u(AppInfo::kContact) + "?subject=" + encode(subject) + "&body=" + encode(body);
	}

	/*! \brief "Template App v0.1.0-rc.1" - the subject of every mail to the developer. */
	wxString mailSubject() { return u(AppInfo::kName) + " v" + u(GetAppVersion()); }

	/*! \brief A two-column label / value grid; rows with an empty value are skipped. */
	class InfoGrid {
	public:
		InfoGrid(wxWindow* page, int vgap) : page_(page), grid_(new wxFlexGridSizer(2, vgap, page->FromDIP(14))) {
			grid_->AddGrowableCol(1);
		}
		void row(const wxString& label, wxWindow* value) {
			grid_->Add(bold(page_, label), 0, wxALIGN_CENTER_VERTICAL);
			grid_->Add(value, 1, wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL);
		}
		void text(const wxString& label, const char* value) {
			if (has(value)) row(label, new wxStaticText(page_, wxID_ANY, u(value)));
		}
		void url(const wxString& label, const char* value) {
			if (has(value)) row(label, link(page_, u(value), u(value)));
		}
		void mail(const wxString& label) {
			if (has(AppInfo::kContact)) row(label, link(page_, u(AppInfo::kContact), mailto(mailSubject(), "")));
		}
		wxFlexGridSizer* sizer() const { return grid_; }
	private:
		wxWindow* page_;
		wxFlexGridSizer* grid_;
	};
}

AboutDialog::AboutDialog(wxWindow* parent, const wxString& databaseFile)
	: wxDialog(parent, wxID_ANY, wxString::Format(tr(UVT::ABOUT_TITLE_FMT), u(AppInfo::kName)), wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), databaseFile_(databaseFile)
{
	auto* top = new wxBoxSizer(wxVERTICAL);
	auto* book = new wxNotebook(this, wxID_ANY);
	book->AddPage(buildAboutPage(book), tr(UVT::ABOUT_PAGE_ABOUT));
	book->AddPage(buildDeveloperPage(book), tr(UVT::ABOUT_PAGE_DEVELOPER));
	book->AddPage(buildDependenciesPage(book), tr(UVT::ABOUT_PAGE_DEPENDENCIES));
	book->AddPage(buildChangesPage(book), tr(UVT::ABOUT_PAGE_CHANGES));
	book->AddPage(buildSystemPage(book), tr(UVT::ABOUT_PAGE_SYSTEM));
	// A page built while hidden may have measured its bold labels too small (GTK): measure again when shown.
	book->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [book](wxBookCtrlEvent& event) {
		if (wxWindow* page = book->GetPage(event.GetSelection())) {
			for (wxWindow* child : page->GetChildren()) child->InvalidateBestSize();
			page->Layout();
		}
		event.Skip();
	});
	top->Add(book, 1, wxEXPAND | wxALL, FromDIP(8));

	auto* buttons = new wxBoxSizer(wxHORIZONTAL);
	buttons->AddStretchSpacer();
	auto* close = new wxButton(this, wxID_OK, tr(UVT::CLOSE_BTN));
	close->SetDefault();
	buttons->Add(close, 0);
	top->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));

	SetSizer(top);
	SetMinSize(FromDIP(wxSize(640, 480)));
	SetSize(FromDIP(wxSize(760, 560)));
	SetEscapeId(wxID_OK);
	CentreOnParent();
}

wxWindow* AboutDialog::buildAboutPage(wxNotebook* book) {
	auto* page = new wxScrolledWindow(book);
	page->SetScrollRate(0, FromDIP(10));
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	const int gap = FromDIP(8);

	// Header: icon, name, version.
	auto* header = new wxBoxSizer(wxHORIZONTAL);
#ifdef __WXMSW__
	wxIcon icon;
	{
		wxLogNull quiet; // a resource icon: no message if a size is missing
		icon = wxIcon("appicon", wxBITMAP_TYPE_ICO_RESOURCE, FromDIP(64), FromDIP(64));
	}
	if (icon.IsOk())
		header->Add(new wxStaticBitmap(page, wxID_ANY, wxBitmapBundle::FromBitmap(wxBitmap(icon))), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));
#endif
	auto* titles = new wxBoxSizer(wxVERTICAL);
	titles->Add(bold(page, u(AppInfo::kName), 1.8f), 0);
	titles->Add(new wxStaticText(page, wxID_ANY, wxString::Format(tr(UVT::ABOUT_VERSION_FMT), u(GetAppVersion()), u(GetBuildDate()))), 0, wxTOP, FromDIP(2));
	header->Add(titles, 1, wxALIGN_CENTER_VERTICAL);
	sizer->Add(header, 0, wxEXPAND | wxALL, gap);

	if (has(AppInfo::kDescription)) {
		auto* description = new wxStaticText(page, wxID_ANY, tr(u(AppInfo::kDescription)));
		description->Wrap(FromDIP(660));
		sizer->Add(description, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, gap);
	}
	if (has(AppInfo::kTagline)) {
		auto* tagline = new wxStaticText(page, wxID_ANY, wxString());
		tagline->SetFont(tagline->GetFont().Italic());
		tagline->SetLabel(tr(u(AppInfo::kTagline)));
		tagline->Wrap(FromDIP(660));
		sizer->Add(tagline, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, gap);
	}

	InfoGrid grid(page, FromDIP(5));
	grid.text(tr(UVT::ABOUT_AUTHOR), AppInfo::kAuthor);
	grid.mail(tr(UVT::ABOUT_CONTACT));
	grid.url(tr(UVT::ABOUT_PROJECT_PAGE), AppInfo::kWebsite);
	grid.url(tr(UVT::ABOUT_REPORT_PROBLEM), AppInfo::kIssues);
	grid.text(tr(UVT::ABOUT_COPYRIGHT), AppInfo::kCopyright);
	if (has(AppInfo::kWebsite) && has(AppInfo::kLicensePath))
		grid.row(tr(UVT::ABOUT_LICENSE), link(page, u(AppInfo::kLicense), u(AppInfo::kWebsite) + u(AppInfo::kLicensePath)));
	else
		grid.text(tr(UVT::ABOUT_LICENSE), AppInfo::kLicense);
	grid.row(tr(UVT::ABOUT_DATA_FOLDER), folderLink(page, DataDir::path()));
	sizer->Add(grid.sizer(), 0, wxEXPAND | wxALL, gap);

	sizer->Add(new wxStaticLine(page), 0, wxEXPAND | wxLEFT | wxRIGHT, gap);
	sizer->Add(new wxStaticText(page, wxID_ANY, tr(UVT::ABOUT_FEATURES)), 0, wxEXPAND | wxALL, gap);
	sizer->Add(new wxStaticText(page, wxID_ANY, tr(UVT::ABOUT_MADE_WITH)), 0, wxALL, gap);

	page->SetSizer(sizer);
	page->FitInside();
	return page;
}

wxWindow* AboutDialog::buildDeveloperPage(wxNotebook* book) {
	auto* page = new wxPanel(book);
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	const int gap = FromDIP(8);

	sizer->Add(bold(page, u(AppInfo::kAuthor), 1.6f), 0, wxALL, gap);
	sizer->Add(new wxStaticText(page, wxID_ANY, tr(UVT::ABOUT_DEVELOPER_ROLE)), 0, wxLEFT | wxRIGHT | wxBOTTOM, gap);

	InfoGrid grid(page, FromDIP(6));
	grid.mail(tr(UVT::ABOUT_EMAIL));
	grid.url(tr(UVT::ABOUT_DEVELOPER_PAGE), AppInfo::kDeveloperPage);
	grid.url(tr(UVT::ABOUT_THIS_PROJECT), AppInfo::kWebsite);
	grid.url(tr(UVT::ABOUT_ISSUES), AppInfo::kIssues);
	sizer->Add(grid.sizer(), 0, wxEXPAND | wxALL, gap);

	// Support: an e-mail with the details filled in, else the issues page, else nothing to offer.
	sizer->Add(new wxStaticLine(page), 0, wxEXPAND | wxALL, gap);
	sizer->Add(heading(page, tr(UVT::ABOUT_SUPPORT)), 0, wxLEFT | wxRIGHT, gap);
	const bool byMail = has(AppInfo::kContact), byIssues = has(AppInfo::kIssues);
	auto* help = new wxStaticText(page, wxID_ANY,
		tr(byMail ? UVT::ABOUT_SUPPORT_MAIL_TEXT : byIssues ? UVT::ABOUT_SUPPORT_ISSUES_TEXT : UVT::ABOUT_SUPPORT_NONE_TEXT));
	help->Wrap(FromDIP(640));
	sizer->Add(help, 0, wxALL, gap);
	if (byMail || byIssues) {
		auto* report = new wxButton(page, wxID_ANY, tr(UVT::ABOUT_REPORT_BUTTON));
		report->Bind(wxEVT_BUTTON, [this, byMail](wxCommandEvent&) {
			if (byMail)
				wxLaunchDefaultBrowser(mailto(mailSubject() + " - " + tr(UVT::ABOUT_MAIL_SUBJECT),
					tr(UVT::ABOUT_MAIL_BODY) + "\n\n\n---- " + tr(UVT::ABOUT_MAIL_DETAILS) + " ----\n" + systemInfoText(databaseFile_)));
			else
				wxLaunchDefaultBrowser(u(AppInfo::kIssues));
		});
		sizer->Add(report, 0, wxALL, gap);
	}
	page->SetSizer(sizer);
	return page;
}

wxWindow* AboutDialog::buildDependenciesPage(wxNotebook* book) {
	auto* page = new wxScrolledWindow(book);
	page->SetScrollRate(0, FromDIP(10));
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	const int gap = FromDIP(8);
	sizer->Add(new wxStaticText(page, wxID_ANY, tr(UVT::ABOUT_LIBRARIES_INTRO)), 0, wxALL, gap);

	struct Dependency {
		wxString name, home, version, licence, licenceUrl, use;
	};
#ifdef SERIAL_USE_DLL
	const wxString serialLinkage = tr(UVT::DEP_SERIAL_DLL);
#else
	const wxString serialLinkage = tr(UVT::DEP_SERIAL_STATIC);
#endif
	std::vector<Dependency> deps = {
		{ "wxWidgets", "https://www.wxwidgets.org/",
		  wxString::Format(tr(UVT::DEP_WX_VERSION_FMT), wxString(wxVERSION_NUM_DOT_STRING), wxRunningVersion()),
		  "wxWindows Library Licence 3.1", "https://www.wxwidgets.org/about/licence/", tr(UVT::DEP_USE_WX) },
		{ "SQLite", "https://www.sqlite.org/", u(sqlite3_libversion()), tr(UVT::DEP_PUBLIC_DOMAIN), "https://www.sqlite.org/copyright.html",
		  tr(UVT::DEP_USE_SQLITE) },
		{ "serial (wjwwood)", "https://github.com/wjwwood/serial", serialLinkage, "MIT", "https://github.com/wjwwood/serial/blob/main/LICENSE",
		  tr(UVT::DEP_USE_SERIAL) },
	};
#ifdef _MSC_VER
	deps.push_back({ "Microsoft Visual C++ runtime", "https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist",
		wxString::Format(tr(UVT::DEP_COMPILER_FMT), _MSC_VER / 100, _MSC_VER % 100), "Visual C++ Redistributable",
		"https://visualstudio.microsoft.com/license-terms/", tr(UVT::DEP_USE_RUNTIME) });
#endif

	auto* grid = new wxFlexGridSizer(4, FromDIP(8), FromDIP(14));
	grid->AddGrowableCol(3);
	for (const wxString* title : { &UVT::DEP_LIBRARY, &UVT::DEP_VERSION, &UVT::DEP_LICENCE, &UVT::DEP_USED_FOR }) grid->Add(bold(page, tr(*title)), 0);
	for (const Dependency& d : deps) {
		grid->Add(link(page, d.name, d.home), 0, wxALIGN_TOP);
		auto* version = new wxStaticText(page, wxID_ANY, d.version);
		version->Wrap(FromDIP(200));
		grid->Add(version, 0, wxALIGN_TOP);
		grid->Add(link(page, d.licence, d.licenceUrl), 0, wxALIGN_TOP);
		auto* use = new wxStaticText(page, wxID_ANY, d.use);
		use->Wrap(FromDIP(190));
		grid->Add(use, 1, wxALIGN_TOP);
	}
	sizer->Add(grid, 0, wxEXPAND | wxALL, gap);
	page->SetSizer(sizer);
	page->FitInside();
	return page;
}

wxWindow* AboutDialog::buildChangesPage(wxNotebook* book) {
	auto* page = new wxPanel(book);
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	wxString text;
	for (const AppVersionHistoryEntry& entry : kAppVersionHistory) {
		text += wxString::Format("%s  (%s)\n", u(entry.version), u(entry.date));
		wxString notes = u(entry.notes);
		notes.Replace("\n", "\n  - ");
		text += "  - " + notes + "\n\n";
	}
	sizer->Add(readOnlyText(page, text), 1, wxEXPAND | wxALL, FromDIP(8));
	page->SetSizer(sizer);
	return page;
}

wxString AboutDialog::systemInfoText(const wxString& databaseFile) {
	// Always English: this text goes to the developer in support e-mails.
	wxString text;
	text << "Application: " << u(AppInfo::kName) << "\n";
	text << "Version: " << u(GetAppVersion()) << " (build " << GetBuildNumber() << ", " << u(GetCompactVersion()) << ")\n";
	text << "Built: " << u(GetBuildDate()) << "\n";
	text << "Source (git): " << u(GetGitDescribe()) << "\n";
	text << "wxWidgets: " << wxRunningVersion() << "\n";
	text << "SQLite: " << u(sqlite3_libversion()) << "\n";
	text << "Operating system: " << wxGetOsDescription() << "\n";
	text << "Platform: " << (sizeof(void*) == 8 ? "64 bit" : "32 bit") << "\n";
	text << "Language: " << u(I18n::current()) << "\n";
	text << "Executable folder: " << wxString(DataDir::exeDirectory().wstring()) << "\n";
	text << "Data folder: " << wxString(DataDir::path().wstring()) << "\n";
	if (!databaseFile.empty()) text << "Database: " << databaseFile << "\n";
	return text;
}

wxWindow* AboutDialog::buildSystemPage(wxNotebook* book) {
	auto* page = new wxPanel(book);
	auto* sizer = new wxBoxSizer(wxVERTICAL);
	const int gap = FromDIP(8);
	const wxString info = systemInfoText(databaseFile_);
	sizer->Add(readOnlyText(page, info, wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE))), 1, wxEXPAND | wxALL, gap);

	auto* folders = new wxFlexGridSizer(2, FromDIP(4), FromDIP(12));
	folders->Add(bold(page, tr(UVT::ABOUT_EXE_FOLDER)), 0, wxALIGN_CENTER_VERTICAL);
	folders->Add(folderLink(page, DataDir::exeDirectory()), 0);
	folders->Add(bold(page, tr(UVT::ABOUT_DATA_FOLDER)), 0, wxALIGN_CENTER_VERTICAL);
	folders->Add(folderLink(page, DataDir::path()), 0);
	sizer->Add(folders, 0, wxLEFT | wxRIGHT, gap);

	auto* copy = new wxButton(page, wxID_ANY, tr(UVT::ABOUT_COPY_CLIPBOARD));
	copy->Bind(wxEVT_BUTTON, [info](wxCommandEvent&) {
		if (wxTheClipboard->Open()) {
			wxTheClipboard->SetData(new wxTextDataObject(info));
			wxTheClipboard->Close();
		}
	});
	sizer->Add(copy, 0, wxALL, gap);
	page->SetSizer(sizer);
	return page;
}
