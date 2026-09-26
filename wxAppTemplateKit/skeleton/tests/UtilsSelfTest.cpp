/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file UtilsSelfTest.cpp
 * \brief Self-test of the Utils helpers (checksums and SHA-256 against their published check values,
 * hex/bit/byte-order conversions, strings, files, math, time). Not part of the application build.
 *
 * Build it as a console program together with src/TimeUtils.cpp, src/HexUtils.cpp,
 * src/TextUtils.cpp and src/HashUtils.cpp (include folder: include), e.g. from a Developer Command Prompt:
 *
 *     cl /std:c++20 /EHsc /utf-8 /I include /I %WXWIN%\include /I %WXWIN%\include\msvc tests\UtilsSelfTest.cpp src\TimeUtils.cpp src\HexUtils.cpp src\TextUtils.cpp src\HashUtils.cpp
 *
 * It prints "all passed" and returns 0, or lists every failed check and returns 1.
 */
#include "HashUtils.h"
#include "HexUtils.h"
#include "MathUtils.h"
#include "TextUtils.h"
#include "TimeUtils.h"
#include <filesystem>
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace Utils;
static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); ++fails; } } while (0)
int main() {
	const char* nine = "123456789";
	const auto* b = reinterpret_cast<const uint8_t*>(nine);
	CHECK(Checksum::crc16Modbus(b, 9) == 0x4B37);
	CHECK(Checksum::crc16CcittFalse(b, 9) == 0x29B1);
	CHECK(Checksum::crc32(b, 9) == 0xCBF43926u);
	CHECK(Checksum::xor8(b, 9) == ('1'^'2'^'3'^'4'^'5'^'6'^'7'^'8'^'9'));
	CHECK(static_cast<uint8_t>(Checksum::sum8(b, 9) + Checksum::twosComplement8(b, 9)) == 0);
	CHECK(Hash::sha256Hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
	CHECK(Hash::sha256Hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
	CHECK(Hex::toHex(uint8_t(0x0A)) == "0A");
	CHECK(Hex::toHex(uint16_t(0xBEEF)) == "BEEF");
	CHECK(Hex::toHex(uint32_t(0x1F), true) == "0x0000001F");
	CHECK(Hex::toHex(uint64_t(1)) == "0000000000000001");
	CHECK(Hex::toHex(uint64_t(0x1F), 4, true) == "0x001F");
	CHECK(Hex::toBinary(uint8_t(5)) == "00000101");
	CHECK(Hex::toBinary(uint16_t(0xA5F0), 4) == "1010 0101 1111 0000");
	CHECK(Hex::toBinary(uint64_t(5), 3) == "101");
	CHECK(Hex::bytesToString({0x55, 0x01, 0xAB}) == "55 01 AB");
	CHECK(Hex::wordsToString(std::vector<uint16_t>{1, 0xABCD}) == "0001 ABCD");
	auto p = Hex::parseBytes("55 01 0a"); CHECK(p && *p == std::vector<uint8_t>({0x55, 0x01, 0x0A}));
	p = Hex::parseBytes("0x55,0x01"); CHECK(p && p->size() == 2 && (*p)[0] == 0x55);
	p = Hex::parseBytes("55010A"); CHECK(p && p->size() == 3);
	CHECK(!Hex::parseBytes("5G"));
	CHECK(!Hex::parseBytes("123"));
	CHECK(Hex::parseNumber("0x1F") == 31u && Hex::parseNumber("1Fh") == 31u && Hex::parseNumber("0b101") == 5u && Hex::parseNumber("42") == 42u);
	CHECK(!Hex::parseNumber("0x1G") && !Hex::parseNumber("99999999999999999999999"));
	CHECK(Bits::extract(0xABCDu, 4, 8) == 0xBCu);
	CHECK(Bits::insert(uint16_t(0xABCD), 4, 8, uint16_t(0x12)) == 0xA12D);
	CHECK(Bits::isSet(uint8_t(0x80), 7) && !Bits::isSet(uint8_t(0x80), 6) && !Bits::isSet(uint8_t(0x80), 9));
	CHECK(Bits::set(uint8_t(0), 3) == 8 && Bits::clear(uint8_t(0xFF), 0) == 0xFE && Bits::toggle(uint8_t(1), 0) == 0);
	CHECK(Bits::mask<uint8_t>(3) == 7 && Bits::mask<uint8_t>(8) == 0xFF && Bits::mask<uint64_t>(64) == ~0ull);
	CHECK(Bits::countOnes(uint32_t(0xF0F0)) == 8 && Bits::lowestSetBit(uint8_t(0x10)) == 4 && Bits::highestSetBit(uint16_t(0x0100)) == 8 && Bits::highestSetBit(uint8_t(0)) == -1);
	CHECK(Bits::reverse(uint8_t(0x01)) == 0x80 && Bits::byteSwap(uint16_t(0x1234)) == 0x3412 && Bits::byteSwap(uint32_t(0x11223344)) == 0x44332211u);
	uint8_t buf[4] = {0x12, 0x34, 0x56, 0x78};
	CHECK(Bits::readBE<uint16_t>(buf) == 0x1234 && Bits::readLE<uint16_t>(buf) == 0x3412 && Bits::readBE<uint32_t>(buf) == 0x12345678u);
	std::vector<uint8_t> out; Bits::appendBE(out, uint16_t(0xABCD)); Bits::appendLE(out, uint16_t(0xABCD));
	CHECK(out == std::vector<uint8_t>({0xAB, 0xCD, 0xCD, 0xAB}));
	CHECK(Bits::floatFromBits(0x3F800000u) == 1.0f && Bits::bitsFromFloat(-2.0f) == 0xC0000000u && Bits::doubleFromBits(Bits::bitsFromDouble(3.25)) == 3.25);
	CHECK(Str::trim("  a b \r\n") == "a b" && Str::split("a,,b", ',').size() == 3 && Str::split("a,,b", ',', false).size() == 2);
	CHECK(Str::join({"a", "b"}, ", ") == "a, b" && Str::toUpper("aZ1") == "AZ1" && Str::equalsIgnoreCase("Abc", "aBC"));
	CHECK(Str::replaceAll("a.b.c", ".", "--") == "a--b--c" && Str::ellipsize("abcdefgh", 6) == "abc...");
	CHECK(Str::toInt(" 42 ") == 42 && !Str::toInt("4x") && Str::toDouble("3.5") == 3.5 && !Str::toDouble("3,5"));
	CHECK(Files::sanitizeFileName("\\\\.\\COM10") == "COM10" && Files::sanitizeFileName("a<b>c") == "a_b_c" && Files::sanitizeFileName("CON") == "_CON");
	CHECK(Files::humanSize(512) == "512 B" && Files::humanSize(1536) == "1.5 KB");
	auto dir = std::filesystem::temp_directory_path() / "utilstest";
	std::filesystem::remove_all(dir);
	CHECK(Files::ensureDirectory(dir / "sub"));
	CHECK(Files::writeTextAtomic(dir / "sub" / "a.txt", "hello"));
	CHECK(Files::readText(dir / "sub" / "a.txt") == std::string("hello"));
	CHECK(Files::uniquePath(dir / "sub", "a", ".txt").filename() == "a_2.txt");
	CHECK(Files::appendLine(dir / "sub" / "a.txt", "x") && Files::size(dir / "sub" / "a.txt") == 7u);
	CHECK(Math::mapRange(5, 0, 10, 0, 100) == 50 && Math::mapRange(20, 0, 10, 0, 100, true) == 100 && Math::roundTo(3.14159, 2) == 3.14);
	CHECK(Math::nearlyEqual(0.1 + 0.2, 0.3) && Math::divideRoundUp(130, 64) == 3 && Math::percent(1, 4) == 25);
	CHECK(Time::durationString(93784005) == "1d 02:03:04.005" && Time::durationString(-1500, false) == "-00:00:01");
	const uint64_t now = Time::nowEpochMs();
	const uint64_t packed = Time::packTimestamp(now);
	CHECK(std::to_string(packed).size() == 17);
	CHECK(Time::unpackTimestamp(packed) == Time::toString(now));
	CHECK(Time::unpackTimestamp(260926140301123ull) == std::string("2026-09-26 14:03:01.123"));
	CHECK(!Time::unpackTimestamp(20261399000000000ull));
	CHECK(Time::nowString().size() == 23 && Time::fileNameStamp().size() == 19 && Time::yearString().size() == 4);
	Time::Stopwatch sw; Time::sleepMs(20); CHECK(sw.elapsedMs() >= 20 && sw.hasElapsed(15));
	printf("utils self-test: %s (%d failure(s))\n", fails ? "FAILED" : "all passed", fails);
	printf("examples: %s | %s | %s | %s\n", Time::nowString().c_str(), Hex::toBinary(uint16_t(0xA5F0), 4).c_str(), Hex::toHex(uint32_t(0xBEEF), true).c_str(), Time::durationString(3723004).c_str());
	printf("%s", Hex::dump({0x55,0x02,0x01,0x85,0x30,0x16,0x00,0x00,0x41,0x42,0x43,0x0A,0x0D,0x7F,0x20,0x21,0x22}).c_str());
	return fails ? 1 : 0;
}
