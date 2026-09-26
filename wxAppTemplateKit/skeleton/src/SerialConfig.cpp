/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file SerialConfig.cpp
 * \brief Implementation of SerialConfig.h.
 */

#include "SerialConfig.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <map>
#include <sstream>

serial::Timeout SerialConfig::timeout() const {
	auto floorPair = [](uint32_t& constant, uint32_t& multiplier) {
		if (constant == 0 && multiplier == 0) constant = kMinTimeoutMs;
	};
	uint32_t rc = readTimeoutConstantMs, rm = readTimeoutMultiplierMs;
	uint32_t wc = writeTimeoutConstantMs, wm = writeTimeoutMultiplierMs;
	floorPair(rc, rm);
	floorPair(wc, wm);
	return serial::Timeout(interByteTimeoutMs, rc, rm, wc, wm);
}

std::string SerialConfig::toString() const {
	return std::format("port={};baud={};bytesize={};parity={};stopbits={};flow={};ib={};rc={};rm={};wc={};wm={}",
		port, baudrate, static_cast<int>(bytesize), static_cast<int>(parity), static_cast<int>(stopbits),
		static_cast<int>(flowcontrol), interByteTimeoutMs, readTimeoutConstantMs, readTimeoutMultiplierMs,
		writeTimeoutConstantMs, writeTimeoutMultiplierMs);
}

std::optional<SerialConfig> SerialConfig::fromString(const std::string& text) {
	std::map<std::string, std::string> kv;
	std::stringstream ss(text);
	std::string item;
	while (std::getline(ss, item, ';')) {
		const size_t eq = item.find('=');
		if (eq != std::string::npos) kv[item.substr(0, eq)] = item.substr(eq + 1);
	}
	SerialConfig c;
	auto num = [&](const char* key, uint32_t fallback) -> uint32_t {
		const auto it = kv.find(key);
		if (it == kv.end()) return fallback;
		uint32_t v = fallback;
		std::from_chars(it->second.data(), it->second.data() + it->second.size(), v);
		return v;
	};
	c.port = kv.count("port") ? kv["port"] : std::string();
	if (c.port.empty()) return std::nullopt;
	c.baudrate = num("baud", c.baudrate);
	c.bytesize = static_cast<serial::bytesize_t>(num("bytesize", c.bytesize));
	c.parity = static_cast<serial::parity_t>(num("parity", c.parity));
	c.stopbits = static_cast<serial::stopbits_t>(num("stopbits", c.stopbits));
	c.flowcontrol = static_cast<serial::flowcontrol_t>(num("flow", c.flowcontrol));
	c.interByteTimeoutMs = num("ib", c.interByteTimeoutMs);
	c.readTimeoutConstantMs = num("rc", c.readTimeoutConstantMs);
	c.readTimeoutMultiplierMs = num("rm", c.readTimeoutMultiplierMs);
	c.writeTimeoutConstantMs = num("wc", c.writeTimeoutConstantMs);
	c.writeTimeoutMultiplierMs = num("wm", c.writeTimeoutMultiplierMs);
	return c;
}

std::string SerialConfig::describe() const {
	const char parityChar[] = { 'N', 'O', 'E', 'M', 'S' };
	const char p = (parity >= 0 && parity <= 4) ? parityChar[parity] : '?';
	const char* sb = stopbits == serial::stopbits_two ? "2" : stopbits == serial::stopbits_one_point_five ? "1.5" : "1";
	return std::format("{} {} {}{}{}", port.empty() ? std::string("-") : port, baudrate, static_cast<int>(bytesize), p, sb);
}

std::vector<serial::PortInfo> SerialConfig::listPorts() {
	std::vector<serial::PortInfo> ports;
	try { ports = serial::list_ports(); } catch (...) {}
	std::sort(ports.begin(), ports.end(), [](const serial::PortInfo& a, const serial::PortInfo& b) {
		// COM2 before COM10
		if (a.port.size() != b.port.size()) return a.port.size() < b.port.size();
		return a.port < b.port;
	});
	return ports;
}

const std::vector<uint32_t>& SerialConfig::standardBaudrates() {
	static const std::vector<uint32_t> rates = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 };
	return rates;
}
