// Copyright (C) 2026 Fation Coga
// SPDX-License-Identifier: LGPL-3.0-or-later
// This file is part of Template App - see COPYING and COPYING.LESSER.

#include "SerialConfigDialog.h"
#include "I18n.h"
#include "UVT.h"

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

SerialConfigDialog::SerialConfigDialog(wxWindow* parent, const SerialConfig& initial)
	: wxDialog(parent, wxID_ANY, tr(UVT::SERIAL_DIALOG_TITLE), wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	  initial_(initial) {
	auto* grid = new wxFlexGridSizer(2, wxSize(10, 6));
	grid->AddGrowableCol(1, 1);
	auto addRow = [&](const wxString& label, wxWindow* ctrl) {
		grid->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		grid->Add(ctrl, 1, wxEXPAND);
	};

	auto* portRow = new wxBoxSizer(wxHORIZONTAL);
	port_ = new wxComboBox(this, wxID_ANY);
	auto* refresh = new wxButton(this, wxID_ANY, tr(UVT::SERIAL_REFRESH_PORTS), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	portRow->Add(port_, 1, wxEXPAND | wxRIGHT, 4);
	portRow->Add(refresh, 0);
	grid->Add(new wxStaticText(this, wxID_ANY, tr(UVT::SERIAL_PORT)), 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(portRow, 1, wxEXPAND);
	fillPorts(initial.port);
	refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { fillPorts(port_->GetValue().utf8_string()); });

	baud_ = new wxComboBox(this, wxID_ANY);
	for (uint32_t rate : SerialConfig::standardBaudrates()) baud_->Append(wxString::Format("%u", rate));
	baud_->SetValue(wxString::Format("%u", initial.baudrate));
	addRow(tr(UVT::SERIAL_BAUDRATE), baud_);

	dataBits_ = new wxChoice(this, wxID_ANY);
	for (int bits = 5; bits <= 8; ++bits) dataBits_->Append(wxString::Format("%d", bits));
	dataBits_->SetSelection(static_cast<int>(initial.bytesize) - 5);
	addRow(tr(UVT::SERIAL_DATA_BITS), dataBits_);

	parity_ = new wxChoice(this, wxID_ANY);
	for (const wxString& p : { UVT::PARITY_NONE_LABEL, UVT::PARITY_ODD_LABEL, UVT::PARITY_EVEN_LABEL, UVT::PARITY_MARK_LABEL, UVT::PARITY_SPACE_LABEL }) parity_->Append(tr(p));
	parity_->SetSelection(static_cast<int>(initial.parity)); // parity_none=0 ... parity_space=4
	addRow(tr(UVT::SERIAL_PARITY), parity_);

	stopBits_ = new wxChoice(this, wxID_ANY);
	stopBits_->Append("1");
	stopBits_->Append("1.5");
	stopBits_->Append("2");
	stopBits_->SetSelection(initial.stopbits == serial::stopbits_two ? 2 : initial.stopbits == serial::stopbits_one_point_five ? 1 : 0);
	addRow(tr(UVT::SERIAL_STOP_BITS), stopBits_);

	flow_ = new wxChoice(this, wxID_ANY);
	for (const wxString& f : { UVT::FLOW_NONE, UVT::FLOW_SOFTWARE, UVT::FLOW_HARDWARE }) flow_->Append(tr(f));
	flow_->SetSelection(initial.flowcontrol == serial::flowcontrol_software ? 1 : initial.flowcontrol == serial::flowcontrol_hardware ? 2 : 0);
	addRow(tr(UVT::SERIAL_FLOW_CONTROL), flow_);

	auto spin = [this](uint32_t value) {
		return new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 60000, static_cast<int>(value));
	};
	interByte_ = spin(initial.interByteTimeoutMs);
	readTimeout_ = spin(initial.readTimeoutConstantMs);
	writeTimeout_ = spin(initial.writeTimeoutConstantMs);
	addRow(tr(UVT::SERIAL_INTER_BYTE_TIMEOUT), interByte_);
	addRow(tr(UVT::SERIAL_READ_TIMEOUT), readTimeout_);
	addRow(tr(UVT::SERIAL_WRITE_TIMEOUT), writeTimeout_);

	auto* top = new wxBoxSizer(wxVERTICAL);
	top->Add(grid, 1, wxEXPAND | wxALL, 12);
	top->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
	SetSizerAndFit(top);
	SetMinSize(wxSize(420, -1));
	CentreOnParent();
}

void SerialConfigDialog::fillPorts(const std::string& select) {
	port_->Clear();
	for (const serial::PortInfo& info : SerialConfig::listPorts()) {
		port_->Append(wxString::FromUTF8(info.port));
	}
	port_->SetValue(wxString::FromUTF8(select));
}

SerialConfig SerialConfigDialog::config() const {
	SerialConfig c = initial_;
	c.port = port_->GetValue().Trim().Trim(false).utf8_string();
	unsigned long baud = 0;
	if (baud_->GetValue().ToULong(&baud) && baud > 0) c.baudrate = static_cast<uint32_t>(baud);
	c.bytesize = static_cast<serial::bytesize_t>(dataBits_->GetSelection() + 5);
	c.parity = static_cast<serial::parity_t>(parity_->GetSelection());
	static const serial::stopbits_t kStop[] = { serial::stopbits_one, serial::stopbits_one_point_five, serial::stopbits_two };
	c.stopbits = kStop[stopBits_->GetSelection() < 0 ? 0 : stopBits_->GetSelection()];
	static const serial::flowcontrol_t kFlow[] = { serial::flowcontrol_none, serial::flowcontrol_software, serial::flowcontrol_hardware };
	c.flowcontrol = kFlow[flow_->GetSelection() < 0 ? 0 : flow_->GetSelection()];
	c.interByteTimeoutMs = static_cast<uint32_t>(interByte_->GetValue());
	c.readTimeoutConstantMs = static_cast<uint32_t>(readTimeout_->GetValue());
	c.writeTimeoutConstantMs = static_cast<uint32_t>(writeTimeout_->GetValue());
	return c;
}
