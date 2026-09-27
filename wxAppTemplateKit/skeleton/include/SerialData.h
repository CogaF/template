/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/*!
 * \file SerialData.h
 * \brief Serial data written by the user as text, in three notations, and received data shown as text.
 *
 * | Notation | What the user writes                        | Bytes                   |
 * |----------|---------------------------------------------|-------------------------|
 * | Hex      | `48 65 0D 0A` (hex digits only; the editor adds a space between bytes) | 48 65 0D 0A |
 * | ASCII    | `He` (printable ASCII characters only, 0x20-0x7E) | 48 65             |
 * | Mixed    | `He//0x0D//0x0A` - text, with `//0x` + two hex digits for one byte | 48 65 0D 0A |
 *
 * In Mixed, every `//0x` is followed by exactly TWO hex digits and stands for ONE byte; repeat it for
 * more bytes (`//0x0D//0x0A`). That keeps text right after a byte unambiguous: `//0x0DAB` is 0D 'A' 'B'.
 * The text `//0x` itself is written `//0x2F//0x2F0x`.
 *
 * Received bytes are shown with display(): printable ASCII as it is, control characters with their
 * name (`[0D=CR]`, `[09=TAB]`), the others as their hex value (`[C1]`).
 */
namespace SerialData {

	/*! \brief How the user writes the bytes of a message (see the file comment). */
	enum class DataFormat {
		Hex,   /*!< hex bytes, "48 65 6C" */
		Ascii, /*!< printable ASCII text, "Hel" */
		Mixed  /*!< text with //0xHH bytes, "Hel//0x0D" */
	};
	/*! \brief "hex" / "ascii" / "mixed" - the key stored in settings (never translated). */
	const char* formatKey(DataFormat format);
	/*! \brief The format with key formatKey(), or nullopt. */
	std::optional<DataFormat> formatFromKey(std::string_view key);

	/*! \brief The introducer of a byte in Mixed notation. */
	inline constexpr std::string_view kMixedBytePrefix = "//0x";

	/*! \brief Why a text is not valid in its notation. */
	enum class ParseError {
		None,           /*!< valid */
		NotHexDigit,    /*!< Hex: a character that is not a hex digit or a space */
		OddHexDigits,   /*!< Hex: the last byte has only one digit */
		NotAscii,       /*!< ASCII/Mixed: a character outside printable ASCII (0x20-0x7E) */
		BadMixedByte    /*!< Mixed: "//0x" not followed by two hex digits */
	};
	/*! \brief Outcome of parse(). */
	struct ParseResult {
		std::vector<uint8_t> bytes;           /*!< the bytes (valid only when ok()) */
		ParseError error = ParseError::None;  /*!< why not */
		size_t errorPosition = 0;             /*!< character (not byte) index of the problem in the text */
		/*! \brief true if the text was valid. */
		bool ok() const { return error == ParseError::None; }
	};

	/*!
	 * \brief The bytes written in text (UTF-8) in the given notation.
	 * An empty text gives no bytes and no error.
	 */
	ParseResult parse(std::string_view text, DataFormat format);

	/*!
	 * \brief bytes written in the given notation: Hex "48 65", Mixed "He//0x0D", ASCII "He".
	 * \return nullopt for ASCII when a byte is not printable ASCII (use Mixed for those).
	 */
	std::optional<std::string> toText(const std::vector<uint8_t>& bytes, DataFormat format);

	/*!
	 * \brief Tidies what the user typed in Hex notation: keeps the hex digits only (a "0x" in front
	 * of a byte is dropped), upper case, a space between bytes: "0x4a,5b6" -> "4A 5B 6".
	 * \param text   the text as typed or pasted.
	 * \param cursor in: caret position in text (characters); out: the same place in the result.
	 */
	std::string formatHexInput(std::string_view text, size_t& cursor);

	/*! \brief true for the printable ASCII characters 0x20 (space) to 0x7E ('~'). */
	constexpr bool isPrintable(uint8_t b) { return b >= 0x20 && b <= 0x7E; }

	/*! \brief Name and description of one byte value for the character table. */
	struct AsciiInfo {
		const char* name;        /*!< "SOH", "TAB", "A", "Space"; the character itself for most printable ones */
		const char* description; /*!< "Start of heading", "Exclamation mark", ... (English; shown through tr()) */
	};
	/*!
	 * \brief Name and description of a byte. Letters and digits get an empty description (the
	 * table builds "Capital letter A" / "Digit 5" from translatable formats); 0x80-0xFF have neither.
	 */
	AsciiInfo asciiInfo(uint8_t b);

	/*!
	 * \brief One byte for the console: the character if printable, "[0D=CR]" for a named control
	 * character, "[C1]" for any other byte.
	 */
	std::string displayByte(uint8_t b);
	/*! \brief Bytes for the console: "Hello[0D=CR][0A=LF]" (see displayByte()). */
	std::string display(const uint8_t* data, size_t size);
	/*! \brief Bytes for the console: "Hello[0D=CR][0A=LF]" (see displayByte()). */
	inline std::string display(const std::vector<uint8_t>& bytes) { return display(bytes.data(), bytes.size()); }
}
