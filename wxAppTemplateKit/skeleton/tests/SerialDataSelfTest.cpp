/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file SerialDataSelfTest.cpp
 * \brief Self-test of the serial terminal engine (SerialData.h, SerialMessage.h): Hex / ASCII / Mixed
 * notation, console display, counters, checksums (against their published check values), message
 * building, settings text form and pattern matching. Not part of the application build.
 *
 * Build it as a console program, e.g. from a Developer Command Prompt:
 *
 *     cl /std:c++20 /EHsc /utf-8 /I include tests\SerialDataSelfTest.cpp src\SerialData.cpp src\SerialMessage.cpp src\HexUtils.cpp
 *
 * It prints "all passed" and returns 0, or lists every failed check and returns 1.
 */
#include "HexUtils.h"
#include "SerialMessage.h"
#include <cstdio>
using namespace SerialData;
using Bytes = std::vector<uint8_t>;
static int fails = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL line %d: %s\n", __LINE__, #cond); ++fails; } } while (0)

int main() {
	// --- checksums: published check values of "123456789" ---
	const Bytes nine = { '1', '2', '3', '4', '5', '6', '7', '8', '9' };
	CHECK(computeChecksum(ChecksumType::Crc8, nine.data(), 9) == 0xF4);
	CHECK(computeChecksum(ChecksumType::Crc8Maxim, nine.data(), 9) == 0xA1);
	CHECK(computeChecksum(ChecksumType::Crc16Modbus, nine.data(), 9) == 0x4B37);
	CHECK(computeChecksum(ChecksumType::Crc16Arc, nine.data(), 9) == 0xBB3D);
	CHECK(computeChecksum(ChecksumType::Crc16CcittFalse, nine.data(), 9) == 0x29B1);
	CHECK(computeChecksum(ChecksumType::Crc16Xmodem, nine.data(), 9) == 0x31C3);
	CHECK(computeChecksum(ChecksumType::Crc32, nine.data(), 9) == 0xCBF43926u);
	CHECK(computeChecksum(ChecksumType::Crc32c, nine.data(), 9) == 0xE3069283u);
	CHECK(computeChecksum(ChecksumType::Sum8, nine.data(), 9) == 0xDD);   // 477 mod 256
	CHECK(computeChecksum(ChecksumType::Sum16, nine.data(), 9) == 477);
	CHECK(computeChecksum(ChecksumType::Lrc8, nine.data(), 9) == 0x23);
	CHECK(Utils::Checksum::crc(nine.data(), 9, 16, 0x8005, 0xFFFF, true, 0) == 0x4B37); // generic = Modbus

	// --- notation ---
	auto p = parse("48 65 0d", DataFormat::Hex);
	CHECK(p.ok() && p.bytes == Bytes({ 0x48, 0x65, 0x0D }));
	p = parse("48 6", DataFormat::Hex);
	CHECK(p.error == ParseError::OddHexDigits && p.errorPosition == 3);
	p = parse("48 6G", DataFormat::Hex);
	CHECK(p.error == ParseError::NotHexDigit && p.errorPosition == 4);
	p = parse("Hi!", DataFormat::Ascii);
	CHECK(p.ok() && p.bytes == Bytes({ 'H', 'i', '!' }));
	p = parse("H\xC3\xA9llo", DataFormat::Ascii); // "Héllo"
	CHECK(p.error == ParseError::NotAscii && p.errorPosition == 1);
	p = parse("\xC3\xA9x\t", DataFormat::Ascii);
	CHECK(p.error == ParseError::NotAscii && p.errorPosition == 0);
	p = parse("OK//0x0D//0x0a", DataFormat::Mixed);
	CHECK(p.ok() && p.bytes == Bytes({ 'O', 'K', 0x0D, 0x0A }));
	p = parse("//0x0DAB", DataFormat::Mixed);
	CHECK(p.ok() && p.bytes == Bytes({ 0x0D, 'A', 'B' }));
	p = parse("ab//0xZ1", DataFormat::Mixed);
	CHECK(p.error == ParseError::BadMixedByte && p.errorPosition == 2);
	p = parse("a/0x41/", DataFormat::Mixed);
	CHECK(p.ok() && p.bytes.size() == 7);
	CHECK(parse("", DataFormat::Hex).ok() && parse("", DataFormat::Hex).bytes.empty());

	CHECK(toText({ 0x48, 0x0D }, DataFormat::Hex) == "48 0D");
	CHECK(toText({ 0x48, 0x0D }, DataFormat::Mixed) == "H//0x0D");
	CHECK(!toText({ 0x48, 0x0D }, DataFormat::Ascii));
	CHECK(toText({ 'H', 'i' }, DataFormat::Ascii) == "Hi");
	// round trips through Mixed, including a literal "//0x41" text
	for (const Bytes& b : { Bytes{ '/', '/', '0', 'x', '4', '1' }, Bytes{ '/', '/', '/', '0', 'x', 'F', 'F', '/' }, Bytes{ 0, 0xFF, '/', 0x7F } }) {
		const auto t = toText(b, DataFormat::Mixed);
		CHECK(t && parse(*t, DataFormat::Mixed).bytes == b);
	}

	size_t cursor = 3;
	CHECK(formatHexInput("0x4a,5b6", cursor) == "4A 5B 6" && cursor == 1); // caret after "0x4" -> after "4"
	cursor = 4;
	CHECK(formatHexInput("abcd", cursor) == "AB CD" && cursor == 5);
	cursor = 2;
	CHECK(formatHexInput("abcd", cursor) == "AB CD" && cursor == 2);
	cursor = 0;
	CHECK(formatHexInput("", cursor).empty() && cursor == 0);

	CHECK(display({ 'H', 'i', 0x0D, 0x0A, 0x09, 0xC1, 0x7F, 0x00 }) == "Hi[0D=CR][0A=LF][09=TAB][C1][7F=DEL][00=NUL]");
	CHECK(std::string(asciiInfo(0x01).name) == "SOH" && std::string(asciiInfo(' ').name) == "Space");
	CHECK(std::string(asciiInfo('A').name) == "A" && std::string(asciiInfo('z').name) == "z" && std::string(asciiInfo('7').name) == "7");
	CHECK(std::string(asciiInfo('~').description) == "Tilde" && std::string(asciiInfo(0xC1).name).empty());

	// --- counters ---
	CounterSpec c;
	c.start = 250; c.end = 255; c.step = 2;
	CHECK(counterValue(c, 0) == 250 && counterValue(c, 1) == 252 && counterValue(c, 2) == 254 && counterValue(c, 3) == 250);
	c.start = 10; c.end = 0; c.step = -3; // counting down
	CHECK(counterValue(c, 1) == 7 && counterValue(c, 3) == 1 && counterValue(c, 4) == 10);
	c.step = 3; CHECK(validate(c) == SpecError::CounterDirection);
	c.start = 0; c.end = 10; c.step = 4; CHECK(counterValue(c, 2) == 8 && counterValue(c, 3) == 0);
	c.start = 0; c.end = UINT64_MAX; c.step = 1; c.width = 8;
	CHECK(counterValue(c, 5) == 5 && validate(c) == SpecError::None);
	c.start = UINT64_MAX - 1; c.end = UINT64_MAX; c.step = 1;
	CHECK(counterValue(c, 1) == UINT64_MAX && counterValue(c, 2) == UINT64_MAX - 1 && counterValue(c, 1000000000001ull) == UINT64_MAX);
	c.start = 5; c.end = 1000; c.step = 1; c.width = 1;
	CHECK(validate(c) == SpecError::CounterRange);
	c.width = 9; CHECK(validate(c) == SpecError::CounterWidth);
	c.width = 2; c.step = 0; CHECK(validate(c) == SpecError::CounterStep);
	c.step = 1; c.rate = 0; CHECK(validate(c) == SpecError::CounterRateValue);

	Bytes m = { 0xAA, 0xBB };
	CounterSpec w; w.index = 1; w.width = 4; w.order = ByteOrder::BigEndian;
	writeCounter(m, w, 0x01020304);
	CHECK(m == Bytes({ 0xAA, 0x01, 0x02, 0x03, 0x04 }));
	w.order = ByteOrder::LittleEndian;
	writeCounter(m, w, 0x01020304);
	CHECK(m == Bytes({ 0xAA, 0x04, 0x03, 0x02, 0x01 }));
	w.encoding = CounterEncoding::AsciiDecimal; w.index = 0; w.width = 3;
	writeCounter(m, w, 42);
	CHECK(m == Bytes({ '0', '4', '2', 0x02, 0x01 }));
	w.encoding = CounterEncoding::AsciiHex; w.width = 4;
	writeCounter(m, w, 0x2A);
	CHECK(m == Bytes({ '0', '0', '2', 'A', 0x01 }));

	// --- index lists ---
	CHECK(parseIndexList(" 5-7, 1 ,3,3") == std::vector<size_t>({ 1, 3, 5, 6, 7 }));
	CHECK(parseIndexList("") == std::vector<size_t>());
	CHECK(!parseIndexList("1,a") && !parseIndexList("7-5") && !parseIndexList("-1"));
	CHECK(indexListToString({ 1, 3, 5, 6, 7 }) == "1, 3, 5-7");

	// --- building: Modbus "read holding registers" with a CRC ---
	MessageSpec spec;
	spec.format = DataFormat::Hex;
	spec.text = "01 03 00 00 00 0A";
	spec.checksum.enabled = true;
	spec.checksum.type = ChecksumType::Crc16Modbus;
	spec.checksum.order = ByteOrder::LittleEndian;
	auto r = build(spec, {});
	CHECK(r.ok() && r.bytes == Bytes({ 0x01, 0x03, 0x00, 0x00, 0x00, 0x0A, 0xC5, 0xCD })); // well-known frame
	// excluding bytes + a part of the result + an index
	spec.text = "02 41 42 03";
	spec.checksum = {};
	spec.checksum.enabled = true;
	spec.checksum.type = ChecksumType::Xor8;
	spec.checksum.excluded = { 0 };      // STX not covered
	r = build(spec, {});
	CHECK(r.ok() && r.bytes == Bytes({ 0x02, 0x41, 0x42, 0x03, 0x41 ^ 0x42 ^ 0x03 }));
	spec.checksum.type = ChecksumType::Crc16Modbus;
	spec.checksum.excluded = {};
	spec.checksum.bytes = 1;
	spec.checksum.upperPart = true;
	spec.checksum.insertAt = 1;
	r = build(spec, {});
	const uint16_t crc = Utils::Checksum::crc16Modbus(Bytes({ 0x02, 0x41, 0x42, 0x03 }));
	CHECK(r.ok() && r.bytes == Bytes({ 0x02, static_cast<uint8_t>(crc >> 8), 0x41, 0x42, 0x03 }) && r.checksumAt == 1 && r.checksumBytes == 1);
	spec.checksum.upperPart = false;
	r = build(spec, {});
	CHECK(r.bytes[1] == static_cast<uint8_t>(crc & 0xFF));
	spec.checksum.bytes = 0; spec.checksum.order = ByteOrder::BigEndian; spec.checksum.insertAt = 99; // beyond = at the end
	r = build(spec, {});
	CHECK(r.bytes.size() == 6 && r.bytes[4] == (crc >> 8) && r.bytes[5] == (crc & 0xFF));
	spec.text = "02 4";
	CHECK(build(spec, {}).error == ParseError::OddHexDigits);

	// --- generator: per-message and per-second counters ---
	MessageSpec g;
	g.format = DataFormat::Mixed;
	g.text = "T=//0x00";
	CounterSpec perMsg; perMsg.index = 2; perMsg.start = 0; perMsg.end = 2; perMsg.rate = 0.5; // every second message
	CounterSpec perSec; perSec.index = 3; perSec.start = 100; perSec.end = 200; perSec.step = 10;
	perSec.rateMode = CounterRate::PerSecond; perSec.rate = 0.1;                                 // one step every 10 s
	g.counters = { perMsg, perSec };
	MessageGenerator gen(g);
	gen.restart(1000);
	Bytes seen;
	for (int i = 0; i < 6; ++i) seen.push_back(gen.next(1000).bytes[2]);
	CHECK(seen == Bytes({ 0, 0, 1, 1, 2, 2 }));
	CHECK(gen.next(1000 + 9999).bytes[3] == 100);
	CHECK(gen.next(1000 + 10000).bytes[3] == 110);
	CHECK(gen.next(1000 + 100000).bytes[3] == 200);
	CHECK(gen.next(1000 + 110000).bytes[3] == 100);

	// --- text form (settings) ---
	AutoReplyRule rule;
	rule.name = "Ping; reply=pong %&|,";
	rule.patternFormat = DataFormat::Mixed;
	rule.patternText = "PING//0x0D";
	rule.delayMs = 25;
	rule.reply = g;
	rule.reply.checksum.enabled = true;
	rule.reply.checksum.type = ChecksumType::Crc32c;
	rule.reply.checksum.excluded = { 0, 4, 5 };
	rule.reply.checksum.bytes = 2;
	rule.reply.checksum.upperPart = true;
	const auto back = autoReplyFromString(toString(rule));
	CHECK(back && back->name == rule.name && back->patternText == rule.patternText && back->delayMs == 25
		&& back->reply.counters.size() == 2 && back->reply.counters[1].rate == 0.1 && back->reply.counters[1].rateMode == CounterRate::PerSecond
		&& back->reply.checksum.excluded == rule.reply.checksum.excluded && back->reply.checksum.type == ChecksumType::Crc32c
		&& back->reply.checksum.upperPart && back->reply.checksum.bytes == 2 && back->reply.text == g.text);
	PeriodicMessage per;
	per.name = "Keep alive"; per.periodMs = 1500; per.enabled = false; per.message = spec;
	const auto perBack = periodicFromString(toString(per));
	CHECK(perBack && perBack->name == per.name && perBack->periodMs == 1500 && !perBack->enabled && perBack->message.text == spec.text);
	CHECK(!autoReplyFromString("garbage") && !periodicFromString("n=x;e=1;ms=abc;m="));

	// --- pattern matching across reads ---
	PatternMatcher pm;
	pm.setPatterns({ { 'A', 'B', 'C' }, { 'C' }, {} });
	const Bytes part1 = { 'x', 'A', 'B' }, part2 = { 'C', 'A', 'B', 'C', 'C' };
	CHECK(pm.feed(part1.data(), part1.size()).empty());
	CHECK(pm.feed(part2.data(), part2.size()) == std::vector<size_t>({ 0, 1, 0, 1, 1 }));
	CHECK(pm.feed(part1.data(), 1).empty()); // "x": no old occurrence reported again

	if (fails == 0) printf("all passed\n");
	return fails ? 1 : 0;
}
