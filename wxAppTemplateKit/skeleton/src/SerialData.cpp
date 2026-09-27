/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file SerialData.cpp
 * \brief Implementation of SerialData.h.
 */

#include "SerialData.h"
#include "HexUtils.h"

namespace SerialData {

namespace {
	int hexValue(char c) {
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'a' && c <= 'f') return c - 'a' + 10;
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		return -1;
	}

	/*! \brief true for the first byte of a UTF-8 character (not a continuation byte). */
	bool startsCharacter(char c) { return (static_cast<unsigned char>(c) & 0xC0) != 0x80; }

	/*! \brief Character index of byte offset `offset` in UTF-8 text. */
	size_t charIndex(std::string_view text, size_t offset) {
		size_t n = 0;
		for (size_t i = 0; i < offset && i < text.size(); ++i)
			if (startsCharacter(text[i])) ++n;
		return n;
	}

	ParseResult fail(std::string_view text, size_t offset, ParseError error) {
		ParseResult r;
		r.error = error;
		r.errorPosition = charIndex(text, offset);
		return r;
	}

	// Control characters 0x00-0x1F, then DEL.
	const AsciiInfo kControl[32] = {
		{ "NUL", "Null" }, { "SOH", "Start of heading" }, { "STX", "Start of text" }, { "ETX", "End of text" },
		{ "EOT", "End of transmission" }, { "ENQ", "Enquiry" }, { "ACK", "Acknowledge" }, { "BEL", "Bell" },
		{ "BS", "Backspace" }, { "TAB", "Horizontal tab" }, { "LF", "Line feed" }, { "VT", "Vertical tab" },
		{ "FF", "Form feed" }, { "CR", "Carriage return" }, { "SO", "Shift out" }, { "SI", "Shift in" },
		{ "DLE", "Data link escape" }, { "DC1", "Device control 1 (XON)" }, { "DC2", "Device control 2" },
		{ "DC3", "Device control 3 (XOFF)" }, { "DC4", "Device control 4" }, { "NAK", "Negative acknowledge" },
		{ "SYN", "Synchronous idle" }, { "ETB", "End of transmission block" }, { "CAN", "Cancel" },
		{ "EM", "End of medium" }, { "SUB", "Substitute" }, { "ESC", "Escape" }, { "FS", "File separator" },
		{ "GS", "Group separator" }, { "RS", "Record separator" }, { "US", "Unit separator" },
	};
	const AsciiInfo kDel = { "DEL", "Delete" };

	struct Punctuation { char c; const char* name; const char* description; };
	const Punctuation kPunctuation[] = {
		{ ' ', "Space", "Space" }, { '!', "!", "Exclamation mark" }, { '"', "\"", "Quotation mark" },
		{ '#', "#", "Number sign" }, { '$', "$", "Dollar sign" }, { '%', "%", "Percent sign" },
		{ '&', "&", "Ampersand" }, { '\'', "'", "Apostrophe" }, { '(', "(", "Left parenthesis" },
		{ ')', ")", "Right parenthesis" }, { '*', "*", "Asterisk" }, { '+', "+", "Plus sign" },
		{ ',', ",", "Comma" }, { '-', "-", "Hyphen-minus" }, { '.', ".", "Full stop" }, { '/', "/", "Slash" },
		{ ':', ":", "Colon" }, { ';', ";", "Semicolon" }, { '<', "<", "Less-than sign" }, { '=', "=", "Equals sign" },
		{ '>', ">", "Greater-than sign" }, { '?', "?", "Question mark" }, { '@', "@", "Commercial at" },
		{ '[', "[", "Left square bracket" }, { '\\', "\\", "Backslash" }, { ']', "]", "Right square bracket" },
		{ '^', "^", "Circumflex accent" }, { '_', "_", "Underscore" }, { '`', "`", "Grave accent" },
		{ '{', "{", "Left curly bracket" }, { '|', "|", "Vertical bar" }, { '}', "}", "Right curly bracket" },
		{ '~', "~", "Tilde" },
	};

	// Names of the letters and digits: one static string per character.
	const char kPrintableNames[] = "0\0" "1\0" "2\0" "3\0" "4\0" "5\0" "6\0" "7\0" "8\0" "9\0"
		"A\0" "B\0" "C\0" "D\0" "E\0" "F\0" "G\0" "H\0" "I\0" "J\0" "K\0" "L\0" "M\0" "N\0" "O\0" "P\0" "Q\0" "R\0" "S\0" "T\0" "U\0" "V\0" "W\0" "X\0" "Y\0" "Z\0"
		"a\0" "b\0" "c\0" "d\0" "e\0" "f\0" "g\0" "h\0" "i\0" "j\0" "k\0" "l\0" "m\0" "n\0" "o\0" "p\0" "q\0" "r\0" "s\0" "t\0" "u\0" "v\0" "w\0" "x\0" "y\0" "z";
}

const char* formatKey(DataFormat format) {
	switch (format) {
	case DataFormat::Hex:   return "hex";
	case DataFormat::Ascii: return "ascii";
	case DataFormat::Mixed: return "mixed";
	}
	return "hex";
}

std::optional<DataFormat> formatFromKey(std::string_view key) {
	if (key == "hex") return DataFormat::Hex;
	if (key == "ascii") return DataFormat::Ascii;
	if (key == "mixed") return DataFormat::Mixed;
	return std::nullopt;
}

ParseResult parse(std::string_view text, DataFormat format) {
	ParseResult r;
	switch (format) {
	case DataFormat::Hex: {
		int high = -1;          // first digit of a byte in progress
		size_t highOffset = 0;
		for (size_t i = 0; i < text.size(); ++i) {
			const char c = text[i];
			if (c == ' ' || c == '\t') {
				if (high >= 0) return fail(text, highOffset, ParseError::OddHexDigits);
				continue;
			}
			const int v = hexValue(c);
			if (v < 0) return fail(text, i, ParseError::NotHexDigit);
			if (high < 0) { high = v; highOffset = i; }
			else { r.bytes.push_back(static_cast<uint8_t>(high * 16 + v)); high = -1; }
		}
		if (high >= 0) return fail(text, highOffset, ParseError::OddHexDigits);
		return r;
	}
	case DataFormat::Ascii:
		for (size_t i = 0; i < text.size(); ++i) {
			if (!isPrintable(static_cast<uint8_t>(text[i]))) return fail(text, i, ParseError::NotAscii);
			r.bytes.push_back(static_cast<uint8_t>(text[i]));
		}
		return r;
	case DataFormat::Mixed:
		for (size_t i = 0; i < text.size();) {
			if (text.substr(i, kMixedBytePrefix.size()) == kMixedBytePrefix) {
				const size_t d = i + kMixedBytePrefix.size();
				const int h = d < text.size() ? hexValue(text[d]) : -1;
				const int l = d + 1 < text.size() ? hexValue(text[d + 1]) : -1;
				if (h < 0 || l < 0) return fail(text, i, ParseError::BadMixedByte);
				r.bytes.push_back(static_cast<uint8_t>(h * 16 + l));
				i = d + 2;
				continue;
			}
			if (!isPrintable(static_cast<uint8_t>(text[i]))) return fail(text, i, ParseError::NotAscii);
			r.bytes.push_back(static_cast<uint8_t>(text[i]));
			++i;
		}
		return r;
	}
	return r;
}

std::optional<std::string> toText(const std::vector<uint8_t>& bytes, DataFormat format) {
	switch (format) {
	case DataFormat::Hex:
		return Utils::Hex::bytesToString(bytes);
	case DataFormat::Ascii: {
		std::string s;
		for (uint8_t b : bytes) {
			if (!isPrintable(b)) return std::nullopt;
			s += static_cast<char>(b);
		}
		return s;
	}
	case DataFormat::Mixed: {
		std::string s;
		for (size_t i = 0; i < bytes.size(); ++i) {
			const uint8_t b = bytes[i];
			// A literal "//0x" followed by two hex digits would read back as a byte: write its first '/' as a byte.
			const bool looksLikeByte = b == '/' && i + 5 < bytes.size() && bytes[i + 1] == '/' && bytes[i + 2] == '0'
				&& bytes[i + 3] == 'x' && hexValue(static_cast<char>(bytes[i + 4])) >= 0 && hexValue(static_cast<char>(bytes[i + 5])) >= 0;
			if (isPrintable(b) && !looksLikeByte) s += static_cast<char>(b);
			else s += std::string(kMixedBytePrefix) + Utils::Hex::toHex(b);
		}
		return s;
	}
	}
	return std::nullopt;
}

std::string formatHexInput(std::string_view text, size_t& cursor) {
	std::string digits;
	size_t digitsBeforeCursor = 0;
	size_t chars = 0; // character index of text[i]
	for (size_t i = 0; i < text.size(); ++i) {
		if (!startsCharacter(text[i])) continue;
		const bool beforeCursor = chars < cursor;
		++chars;
		const char c = text[i];
		// "0x" / "0X" in front of a byte (pasted C-style bytes): drop both characters.
		if (c == '0' && i + 1 < text.size() && (text[i + 1] == 'x' || text[i + 1] == 'X')) continue;
		if (hexValue(c) < 0) continue;
		digits += static_cast<char>(c >= 'a' && c <= 'f' ? c - 'a' + 'A' : c);
		if (beforeCursor) ++digitsBeforeCursor;
	}
	std::string out;
	for (size_t k = 0; k < digits.size(); ++k) {
		if (k > 0 && k % 2 == 0) out += ' ';
		out += digits[k];
	}
	const size_t n = digitsBeforeCursor;
	cursor = n == 0 ? 0 : (n - 1) + (n - 1) / 2 + 1;
	return out;
}

AsciiInfo asciiInfo(uint8_t b) {
	if (b < 0x20) return kControl[b];
	if (b == 0x7F) return kDel;
	if (b > 0x7F) return { "", "" };
	for (const Punctuation& p : kPunctuation)
		if (p.c == static_cast<char>(b)) return { p.name, p.description };
	int slot = -1;
	if (b >= '0' && b <= '9') slot = b - '0';
	else if (b >= 'A' && b <= 'Z') slot = 10 + (b - 'A');
	else if (b >= 'a' && b <= 'z') slot = 36 + (b - 'a');
	if (slot < 0) return { "", "" };
	return { kPrintableNames + slot * 2, "" };
}

std::string displayByte(uint8_t b) {
	if (isPrintable(b)) return std::string(1, static_cast<char>(b));
	const std::string hex = Utils::Hex::toHex(b);
	if (b < 0x20 || b == 0x7F) return "[" + hex + "=" + asciiInfo(b).name + "]";
	return "[" + hex + "]";
}

std::string display(const uint8_t* data, size_t size) {
	std::string s;
	s.reserve(size);
	for (size_t i = 0; i < size; ++i) s += displayByte(data[i]);
	return s;
}

} // namespace SerialData
