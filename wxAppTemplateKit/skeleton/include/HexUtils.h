/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/*!
 * \file HexUtils.h
 * \brief Numbers <-> hexadecimal / binary text, bit manipulation, byte order and checksums - the
 * everyday tools of a program that talks to devices.
 *
 *     Utils::Hex::toHex(uint8_t(0x0A))            -> "0A"        (width follows the type)
 *     Utils::Hex::toHex(uint32_t(0x1F), true)     -> "0x0000001F"
 *     Utils::Hex::toBinary(uint8_t(5))            -> "00000101"
 *     Utils::Hex::toBinary(uint16_t(0xA5F0), 4)   -> "1010 0101 1111 0000"
 *     Utils::Hex::bytesToString({0x55, 0x01})     -> "55 01"
 *     Utils::Hex::parseBytes("55 01 0A")          -> {0x55, 0x01, 0x0A}
 *     Utils::Bits::extract(0xABCDu, 4, 8)         -> 0xBC
 *     Utils::Bits::readBE<uint16_t>(buffer)       -> big-endian value at buffer[0..1]
 *     Utils::Checksum::crc16Modbus(frame)         -> Modbus RTU CRC
 */

namespace Utils::Hex {

	/*!
	 * \brief Hexadecimal text of an unsigned integer, upper case, zero-padded to the full width of
	 * its type (2 digits for uint8_t, 4 for uint16_t, 8 for uint32_t, 16 for uint64_t).
	 * \param value  the number.
	 * \param prefix true to prepend "0x".
	 */
	template <std::unsigned_integral T>
	std::string toHex(T value, bool prefix = false) {
		static constexpr char kDigits[] = "0123456789ABCDEF";
		std::string s(sizeof(T) * 2, '0');
		for (size_t i = s.size(); i-- > 0; value = static_cast<T>(value >> 4)) s[i] = kDigits[value & 0x0F];
		return prefix ? "0x" + s : s;
	}

	/*!
	 * \brief Hexadecimal text of value with at least minDigits digits (more if the value needs them).
	 * \param value     the number.
	 * \param minDigits pad with leading zeros up to this many digits.
	 * \param prefix    true to prepend "0x".
	 */
	std::string toHex(uint64_t value, int minDigits, bool prefix = false);

	/*!
	 * \brief Binary text of value with exactly bits digits (most significant first).
	 * \param value      the number.
	 * \param bits       number of digits (1-64); higher bits of value are ignored.
	 * \param groupEvery insert a space every groupEvery digits, counted from the right (0 = none).
	 */
	std::string toBinary(uint64_t value, int bits, int groupEvery = 0);

	/*! \brief Binary text of an unsigned integer with all the bits of its type, e.g. 8 for uint8_t. */
	template <std::unsigned_integral T>
	std::string toBinary(T value, int groupEvery = 0) {
		return toBinary(static_cast<uint64_t>(value), static_cast<int>(sizeof(T) * 8), groupEvery);
	}

	/*! \brief "55 01 0A" - bytes as hex pairs joined by separator. */
	std::string bytesToString(const uint8_t* data, size_t size, std::string_view separator = " ");
	/*! \brief "55 01 0A" - bytes as hex pairs joined by separator. */
	inline std::string bytesToString(const std::vector<uint8_t>& bytes, std::string_view separator = " ") {
		return bytesToString(bytes.data(), bytes.size(), separator);
	}

	/*! \brief Words (uint16_t/uint32_t/uint64_t) as full-width hex joined by separator, e.g. "0001 ABCD". */
	template <std::unsigned_integral T>
	std::string wordsToString(const std::vector<T>& words, std::string_view separator = " ") {
		std::string out;
		for (size_t i = 0; i < words.size(); ++i) {
			if (i) out += separator;
			out += toHex(words[i]);
		}
		return out;
	}

	/*!
	 * \brief Classic hex dump: offset, 16 bytes in hex, then the printable ASCII characters.
	 *     0000  55 02 01 85 30 16 00 00  00 00                    U...0.....
	 */
	std::string dump(const std::vector<uint8_t>& bytes, size_t bytesPerLine = 16);

	/*!
	 * \brief Parses hex bytes: "55 01 0a", "55010A", "0x55,0x01", "55:01:0A". Separators (space,
	 * comma, colon, dash, tab) and "0x" prefixes are optional.
	 * \return the bytes, or nullopt if a character is not hex or a byte has an odd number of digits.
	 */
	std::optional<std::vector<uint8_t>> parseBytes(std::string_view text);

	/*!
	 * \brief Parses one unsigned number written as hex ("0x1F", "1Fh"), binary ("0b1010") or
	 * decimal ("31"). nullopt if invalid or larger than 64 bits.
	 */
	std::optional<uint64_t> parseNumber(std::string_view text);
}

namespace Utils::Bits {

	/*! \brief A value with the lowest count bits set, e.g. mask<uint8_t>(3) = 0b00000111. */
	template <std::unsigned_integral T = uint64_t>
	constexpr T mask(unsigned count) {
		return count >= sizeof(T) * 8 ? static_cast<T>(~T{ 0 }) : static_cast<T>((T{ 1 } << count) - 1);
	}
	/*! \brief true if bit number bit (0 = least significant) of value is 1. */
	template <std::unsigned_integral T>
	constexpr bool isSet(T value, unsigned bit) { return bit < sizeof(T) * 8 && ((value >> bit) & T{ 1 }) != 0; }
	/*! \brief value with bit number bit set to 1. */
	template <std::unsigned_integral T>
	constexpr T set(T value, unsigned bit) { return static_cast<T>(value | static_cast<T>(T{ 1 } << bit)); }
	/*! \brief value with bit number bit set to 0. */
	template <std::unsigned_integral T>
	constexpr T clear(T value, unsigned bit) { return static_cast<T>(value & static_cast<T>(~static_cast<T>(T{ 1 } << bit))); }
	/*! \brief value with bit number bit inverted. */
	template <std::unsigned_integral T>
	constexpr T toggle(T value, unsigned bit) { return static_cast<T>(value ^ static_cast<T>(T{ 1 } << bit)); }
	/*! \brief value with bit number bit set to on. */
	template <std::unsigned_integral T>
	constexpr T assign(T value, unsigned bit, bool on) { return on ? set(value, bit) : clear(value, bit); }

	/*! \brief The length bits of value starting at bit position (0 = least significant), shifted down. */
	template <std::unsigned_integral T>
	constexpr T extract(T value, unsigned position, unsigned length) { return static_cast<T>((value >> position) & mask<T>(length)); }
	/*! \brief value with the length bits at position replaced by the low bits of field. */
	template <std::unsigned_integral T>
	constexpr T insert(T value, unsigned position, unsigned length, T field) {
		const T m = static_cast<T>(mask<T>(length) << position);
		return static_cast<T>((value & static_cast<T>(~m)) | (static_cast<T>(field << position) & m));
	}

	/*! \brief Number of bits set to 1. */
	template <std::unsigned_integral T>
	constexpr int countOnes(T value) { return std::popcount(value); }
	/*! \brief Position of the lowest bit set to 1, or -1 if value is 0. */
	template <std::unsigned_integral T>
	constexpr int lowestSetBit(T value) { return value ? std::countr_zero(value) : -1; }
	/*! \brief Position of the highest bit set to 1, or -1 if value is 0. */
	template <std::unsigned_integral T>
	constexpr int highestSetBit(T value) { return value ? static_cast<int>(sizeof(T) * 8) - 1 - std::countl_zero(value) : -1; }
	/*! \brief value with its bit order reversed (bit 0 <-> highest bit). */
	template <std::unsigned_integral T>
	constexpr T reverse(T value) {
		T out = 0;
		for (unsigned i = 0; i < sizeof(T) * 8; ++i, value >>= 1) out = static_cast<T>((out << 1) | (value & 1));
		return out;
	}

	/*! \brief value with its bytes in the opposite order (0x1234 -> 0x3412). */
	template <std::unsigned_integral T>
	constexpr T byteSwap(T value) {
		T out = 0;
		for (size_t i = 0; i < sizeof(T); ++i, value >>= 8) out = static_cast<T>((out << 8) | (value & 0xFF));
		return out;
	}

	/*! \brief Reads a big-endian (most significant byte first) T from data[0..sizeof(T)-1]. */
	template <std::unsigned_integral T>
	constexpr T readBE(const uint8_t* data) {
		T v = 0;
		for (size_t i = 0; i < sizeof(T); ++i) v = static_cast<T>((v << 8) | data[i]);
		return v;
	}
	/*! \brief Reads a little-endian (least significant byte first) T from data[0..sizeof(T)-1]. */
	template <std::unsigned_integral T>
	constexpr T readLE(const uint8_t* data) {
		T v = 0;
		for (size_t i = sizeof(T); i-- > 0;) v = static_cast<T>((v << 8) | data[i]);
		return v;
	}
	/*! \brief Appends value to out in big-endian order. */
	template <std::unsigned_integral T>
	void appendBE(std::vector<uint8_t>& out, T value) {
		for (size_t i = sizeof(T); i-- > 0;) out.push_back(static_cast<uint8_t>(value >> (8 * i)));
	}
	/*! \brief Appends value to out in little-endian order. */
	template <std::unsigned_integral T>
	void appendLE(std::vector<uint8_t>& out, T value) {
		for (size_t i = 0; i < sizeof(T); ++i) out.push_back(static_cast<uint8_t>(value >> (8 * i)));
	}

	/*! \brief The float whose IEEE-754 bit pattern is bits (devices often send floats as 32-bit words). */
	inline float floatFromBits(uint32_t bits) { return std::bit_cast<float>(bits); }
	/*! \brief The IEEE-754 bit pattern of value. */
	inline uint32_t bitsFromFloat(float value) { return std::bit_cast<uint32_t>(value); }
	/*! \brief The double whose IEEE-754 bit pattern is bits. */
	inline double doubleFromBits(uint64_t bits) { return std::bit_cast<double>(bits); }
	/*! \brief The IEEE-754 bit pattern of value. */
	inline uint64_t bitsFromDouble(double value) { return std::bit_cast<uint64_t>(value); }
}

/*!
 * \brief Checksums used by serial protocols. All take the bytes to cover; append the result to a
 * frame in the byte order the protocol specifies (Modbus RTU: CRC low byte first).
 */
namespace Utils::Checksum {
	/*! \brief XOR of all bytes (the "BCC" of Omron CompoWay/F and many others). */
	uint8_t xor8(const uint8_t* data, size_t size);
	/*! \brief Sum of all bytes, modulo 256. */
	uint8_t sum8(const uint8_t* data, size_t size);
	/*! \brief Two's complement of sum8() - the byte that makes the total sum 0 (Intel HEX style). */
	uint8_t twosComplement8(const uint8_t* data, size_t size);
	/*! \brief CRC-16/MODBUS (poly 0xA001 reflected, init 0xFFFF). */
	uint16_t crc16Modbus(const uint8_t* data, size_t size);
	/*! \brief CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, not reflected). */
	uint16_t crc16CcittFalse(const uint8_t* data, size_t size);
	/*! \brief CRC-32 (IEEE 802.3, as zip/Ethernet; poly 0xEDB88320 reflected). */
	uint32_t crc32(const uint8_t* data, size_t size);
	/*! \brief Sum of all bytes, modulo 65536. */
	uint16_t sum16(const uint8_t* data, size_t size);
	/*! \brief CRC-8 (CRC-8/SMBUS: poly 0x07, init 0x00, not reflected). */
	uint8_t crc8(const uint8_t* data, size_t size);
	/*! \brief CRC-8/MAXIM (Dallas 1-Wire: poly 0x31, init 0x00, reflected). */
	uint8_t crc8Maxim(const uint8_t* data, size_t size);
	/*! \brief CRC-16/ARC (also "CRC-16", "CRC-16/IBM": poly 0x8005, init 0x0000, reflected). */
	uint16_t crc16Arc(const uint8_t* data, size_t size);
	/*! \brief CRC-16/XMODEM (poly 0x1021, init 0x0000, not reflected). */
	uint16_t crc16Xmodem(const uint8_t* data, size_t size);
	/*! \brief CRC-32C (Castagnoli, as iSCSI/SCTP: poly 0x1EDC6F41, reflected). */
	uint32_t crc32c(const uint8_t* data, size_t size);

	/*!
	 * \brief Any CRC of up to 64 bits in the usual "Rocksoft" description (the one catalogues list).
	 * \param data    the bytes to cover.
	 * \param size    number of bytes.
	 * \param width   CRC width in bits (1-64).
	 * \param poly    generator polynomial, NOT reflected (e.g. 0x8005), without the top bit.
	 * \param init    initial register value.
	 * \param reflect true for reflected input and output (refin = refout = true).
	 * \param xorOut  value XORed into the final register.
	 */
	uint64_t crc(const uint8_t* data, size_t size, int width, uint64_t poly, uint64_t init, bool reflect, uint64_t xorOut);

	/*! \brief xor8() of a whole buffer. */
	inline uint8_t xor8(const std::vector<uint8_t>& b) { return xor8(b.data(), b.size()); }
	/*! \brief sum8() of a whole buffer. */
	inline uint8_t sum8(const std::vector<uint8_t>& b) { return sum8(b.data(), b.size()); }
	/*! \brief crc16Modbus() of a whole buffer. */
	inline uint16_t crc16Modbus(const std::vector<uint8_t>& b) { return crc16Modbus(b.data(), b.size()); }
	/*! \brief crc16CcittFalse() of a whole buffer. */
	inline uint16_t crc16CcittFalse(const std::vector<uint8_t>& b) { return crc16CcittFalse(b.data(), b.size()); }
	/*! \brief crc32() of a whole buffer. */
	inline uint32_t crc32(const std::vector<uint8_t>& b) { return crc32(b.data(), b.size()); }
}
