/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file HexUtils.cpp
 * \brief Implementation of HexUtils.h.
 */

#include "HexUtils.h"

#include <cctype>

namespace Utils::Hex {

std::string toHex(uint64_t value, int minDigits, bool prefix) {
	static constexpr char kDigits[] = "0123456789ABCDEF";
	std::string s;
	do { s.insert(s.begin(), kDigits[value & 0x0F]); value >>= 4; } while (value != 0);
	if (static_cast<int>(s.size()) < minDigits) s.insert(0, static_cast<size_t>(minDigits) - s.size(), '0');
	return prefix ? "0x" + s : s;
}

std::string toBinary(uint64_t value, int bits, int groupEvery) {
	if (bits < 1) bits = 1;
	if (bits > 64) bits = 64;
	std::string s;
	for (int i = bits - 1; i >= 0; --i) {
		s += ((value >> i) & 1u) ? '1' : '0';
		if (groupEvery > 0 && i > 0 && i % groupEvery == 0) s += ' ';
	}
	return s;
}

std::string bytesToString(const uint8_t* data, size_t size, std::string_view separator) {
	static constexpr char kDigits[] = "0123456789ABCDEF";
	std::string out;
	out.reserve(size * (2 + separator.size()));
	for (size_t i = 0; i < size; ++i) {
		if (i) out += separator;
		out += kDigits[data[i] >> 4];
		out += kDigits[data[i] & 0x0F];
	}
	return out;
}

std::string dump(const std::vector<uint8_t>& bytes, size_t bytesPerLine) {
	if (bytesPerLine == 0) bytesPerLine = 16;
	std::string out;
	for (size_t off = 0; off < bytes.size(); off += bytesPerLine) {
		out += toHex(static_cast<uint64_t>(off), 4) + "  ";
		std::string ascii;
		for (size_t i = 0; i < bytesPerLine; ++i) {
			if (i == bytesPerLine / 2) out += ' ';
			if (off + i < bytes.size()) {
				const uint8_t b = bytes[off + i];
				out += toHex(b) + ' ';
				ascii += (b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.';
			}
			else {
				out += "   ";
			}
		}
		out += ' ' + ascii + '\n';
	}
	return out;
}

std::optional<std::vector<uint8_t>> parseBytes(std::string_view text) {
	std::vector<uint8_t> out;
	std::string digits;
	auto flush = [&]() -> bool {
		if (digits.empty()) return true;
		if (digits.size() % 2 != 0) return false;
		for (size_t i = 0; i < digits.size(); i += 2) out.push_back(static_cast<uint8_t>(std::stoi(digits.substr(i, 2), nullptr, 16)));
		digits.clear();
		return true;
	};
	for (size_t i = 0; i < text.size(); ++i) {
		const char c = text[i];
		if (c == '0' && i + 1 < text.size() && (text[i + 1] == 'x' || text[i + 1] == 'X') && digits.empty()) { ++i; continue; }
		if (c == ' ' || c == ',' || c == ':' || c == '-' || c == '\t' || c == ';') { if (!flush()) return std::nullopt; continue; }
		if (!std::isxdigit(static_cast<unsigned char>(c))) return std::nullopt;
		digits += c;
	}
	if (!flush()) return std::nullopt;
	return out;
}

std::optional<uint64_t> parseNumber(std::string_view text) {
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.remove_prefix(1);
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.remove_suffix(1);
	if (text.empty()) return std::nullopt;
	int base = 10;
	if (text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) { base = 16; text.remove_prefix(2); }
	else if (text.size() > 2 && text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) { base = 2; text.remove_prefix(2); }
	else if (text.size() > 1 && (text.back() == 'h' || text.back() == 'H')) { base = 16; text.remove_suffix(1); }
	uint64_t value = 0;
	for (char c : text) {
		int d;
		if (c >= '0' && c <= '9') d = c - '0';
		else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
		else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
		else return std::nullopt;
		if (d >= base) return std::nullopt;
		if (value > (UINT64_MAX - static_cast<uint64_t>(d)) / static_cast<uint64_t>(base)) return std::nullopt; // overflow
		value = value * static_cast<uint64_t>(base) + static_cast<uint64_t>(d);
	}
	return value;
}

} // namespace Utils::Hex

namespace Utils::Checksum {

uint8_t xor8(const uint8_t* data, size_t size) {
	uint8_t v = 0;
	for (size_t i = 0; i < size; ++i) v ^= data[i];
	return v;
}

uint8_t sum8(const uint8_t* data, size_t size) {
	uint8_t v = 0;
	for (size_t i = 0; i < size; ++i) v = static_cast<uint8_t>(v + data[i]);
	return v;
}

uint8_t twosComplement8(const uint8_t* data, size_t size) { return static_cast<uint8_t>(0x100 - sum8(data, size)); }

uint16_t crc16Modbus(const uint8_t* data, size_t size) {
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < size; ++i) {
		crc ^= data[i];
		for (int b = 0; b < 8; ++b) crc = (crc & 1) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001) : static_cast<uint16_t>(crc >> 1);
	}
	return crc;
}

uint16_t crc16CcittFalse(const uint8_t* data, size_t size) {
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < size; ++i) {
		crc ^= static_cast<uint16_t>(data[i] << 8);
		for (int b = 0; b < 8; ++b) crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
	}
	return crc;
}

uint32_t crc32(const uint8_t* data, size_t size) {
	uint32_t crc = 0xFFFFFFFFu;
	for (size_t i = 0; i < size; ++i) {
		crc ^= data[i];
		for (int b = 0; b < 8; ++b) crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
	}
	return ~crc;
}

} // namespace Utils::Checksum
