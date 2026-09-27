/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file SerialMessage.cpp
 * \brief Implementation of SerialMessage.h.
 */

#include "SerialMessage.h"
#include "HexUtils.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <set>

namespace SerialData {

namespace {
	const std::vector<ChecksumInfo> kChecksums = {
		{ ChecksumType::Sum8,            "sum8",            "SUM-8 (add)",          1, ByteOrder::BigEndian },
		{ ChecksumType::Sum16,           "sum16",           "SUM-16 (add)",         2, ByteOrder::BigEndian },
		{ ChecksumType::Xor8,            "xor8",            "XOR-8 (BCC)",          1, ByteOrder::BigEndian },
		{ ChecksumType::Lrc8,            "lrc8",            "LRC-8 (two's complement of the sum)", 1, ByteOrder::BigEndian },
		{ ChecksumType::Crc8,            "crc8",            "CRC-8 (SMBus)",        1, ByteOrder::BigEndian },
		{ ChecksumType::Crc8Maxim,       "crc8maxim",       "CRC-8/MAXIM (1-Wire)", 1, ByteOrder::BigEndian },
		{ ChecksumType::Crc16Modbus,     "crc16modbus",     "CRC-16/MODBUS",        2, ByteOrder::LittleEndian },
		{ ChecksumType::Crc16Arc,        "crc16arc",        "CRC-16/ARC (IBM)",     2, ByteOrder::LittleEndian },
		{ ChecksumType::Crc16CcittFalse, "crc16ccittfalse", "CRC-16/CCITT-FALSE",   2, ByteOrder::BigEndian },
		{ ChecksumType::Crc16Xmodem,     "crc16xmodem",     "CRC-16/XMODEM",        2, ByteOrder::BigEndian },
		{ ChecksumType::Crc32,           "crc32",           "CRC-32 (IEEE, zip)",   4, ByteOrder::LittleEndian },
		{ ChecksumType::Crc32c,          "crc32c",          "CRC-32C (Castagnoli)", 4, ByteOrder::LittleEndian },
	};

	// --- one-line text form: key=value;key=value, values %-escaped -------------------------------

	std::string escape(std::string_view s) {
		static constexpr char kHex[] = "0123456789ABCDEF";
		std::string out;
		for (char ch : s) {
			const auto c = static_cast<unsigned char>(ch);
			if (c < 0x20 || c >= 0x7F || std::string_view("%;=|,&").find(ch) != std::string_view::npos) {
				out += '%';
				out += kHex[c >> 4];
				out += kHex[c & 0x0F];
			}
			else out += ch;
		}
		return out;
	}

	std::optional<std::string> unescape(std::string_view s) {
		std::string out;
		for (size_t i = 0; i < s.size(); ++i) {
			if (s[i] != '%') { out += s[i]; continue; }
			if (i + 2 >= s.size()) return std::nullopt;
			const auto v = Utils::Hex::parseBytes(s.substr(i + 1, 2));
			if (!v || v->size() != 1) return std::nullopt;
			out += static_cast<char>((*v)[0]);
			i += 2;
		}
		return out;
	}

	std::vector<std::string_view> splitOn(std::string_view s, char sep) {
		std::vector<std::string_view> parts;
		size_t from = 0;
		for (;;) {
			const size_t at = s.find(sep, from);
			parts.push_back(s.substr(from, at == std::string_view::npos ? std::string_view::npos : at - from));
			if (at == std::string_view::npos) return parts;
			from = at + 1;
		}
	}

	std::map<std::string, std::string> readFields(std::string_view s) {
		std::map<std::string, std::string> fields;
		if (s.empty()) return fields;
		for (std::string_view part : splitOn(s, ';')) {
			const size_t eq = part.find('=');
			if (eq == std::string_view::npos) continue;
			fields[std::string(part.substr(0, eq))] = std::string(part.substr(eq + 1));
		}
		return fields;
	}

	template <typename T>
	bool readNumber(std::string_view s, T& out) {
		const auto r = std::from_chars(s.data(), s.data() + s.size(), out);
		return r.ec == std::errc() && r.ptr == s.data() + s.size();
	}

	std::string doubleText(double v) {
		char buf[64];
		const auto r = std::to_chars(buf, buf + sizeof(buf), v);
		return std::string(buf, r.ptr);
	}

	const char* orderKey(ByteOrder o) { return o == ByteOrder::LittleEndian ? "le" : "be"; }
	ByteOrder orderFromKey(std::string_view k) { return k == "le" ? ByteOrder::LittleEndian : ByteOrder::BigEndian; }

	const char* encodingKey(CounterEncoding e) {
		switch (e) {
		case CounterEncoding::AsciiDecimal: return "dec";
		case CounterEncoding::AsciiHex:     return "hex";
		default:                            return "bin";
		}
	}
	CounterEncoding encodingFromKey(std::string_view k) {
		if (k == "dec") return CounterEncoding::AsciiDecimal;
		if (k == "hex") return CounterEncoding::AsciiHex;
		return CounterEncoding::Binary;
	}

	std::string counterToString(const CounterSpec& c) {
		return std::to_string(c.index) + "," + std::to_string(c.width) + "," + encodingKey(c.encoding) + "," + orderKey(c.order) + ","
			+ std::to_string(c.start) + "," + std::to_string(c.end) + "," + std::to_string(c.step) + ","
			+ (c.rateMode == CounterRate::PerSecond ? "s" : "m") + "," + doubleText(c.rate);
	}

	std::optional<CounterSpec> counterFromString(std::string_view s) {
		const auto f = splitOn(s, ',');
		if (f.size() != 9) return std::nullopt;
		CounterSpec c;
		if (!readNumber(f[0], c.index) || !readNumber(f[1], c.width) || !readNumber(f[4], c.start) || !readNumber(f[5], c.end)
			|| !readNumber(f[6], c.step) || !readNumber(f[8], c.rate)) return std::nullopt;
		c.encoding = encodingFromKey(f[2]);
		c.order = orderFromKey(f[3]);
		c.rateMode = f[7] == "s" ? CounterRate::PerSecond : CounterRate::PerMessage;
		if (validate(c) != SpecError::None) return std::nullopt;
		return c;
	}

	std::string checksumToString(const ChecksumSpec& k) {
		std::string excluded;
		for (size_t i = 0; i < k.excluded.size(); ++i) excluded += (i ? "." : "") + std::to_string(k.excluded[i]);
		return std::string(k.enabled ? "1" : "0") + "," + checksumInfo(k.type).key + "," + std::to_string(k.insertAt) + ","
			+ excluded + "," + std::to_string(k.bytes) + "," + (k.upperPart ? "1" : "0") + "," + orderKey(k.order);
	}

	std::optional<ChecksumSpec> checksumFromString(std::string_view s) {
		const auto f = splitOn(s, ',');
		if (f.size() != 7) return std::nullopt;
		ChecksumSpec k;
		k.enabled = f[0] == "1";
		bool known = false;
		for (const ChecksumInfo& info : kChecksums)
			if (f[1] == info.key) { k.type = info.type; known = true; }
		if (!known || !readNumber(f[2], k.insertAt) || !readNumber(f[4], k.bytes)) return std::nullopt;
		if (!f[3].empty()) {
			for (std::string_view idx : splitOn(f[3], '.')) {
				size_t v = 0;
				if (!readNumber(idx, v)) return std::nullopt;
				k.excluded.push_back(v);
			}
		}
		k.upperPart = f[5] == "1";
		k.order = orderFromKey(f[6]);
		return k;
	}
}

// ------------------------------------------------------------------------------------------------
// Checksums
// ------------------------------------------------------------------------------------------------

const std::vector<ChecksumInfo>& checksumTypes() { return kChecksums; }

const ChecksumInfo& checksumInfo(ChecksumType type) {
	for (const ChecksumInfo& info : kChecksums)
		if (info.type == type) return info;
	return kChecksums.front();
}

uint64_t computeChecksum(ChecksumType type, const uint8_t* data, size_t size) {
	using namespace Utils::Checksum;
	switch (type) {
	case ChecksumType::Sum8:            return sum8(data, size);
	case ChecksumType::Sum16:           return sum16(data, size);
	case ChecksumType::Xor8:            return xor8(data, size);
	case ChecksumType::Lrc8:            return twosComplement8(data, size);
	case ChecksumType::Crc8:            return crc8(data, size);
	case ChecksumType::Crc8Maxim:       return crc8Maxim(data, size);
	case ChecksumType::Crc16Modbus:     return crc16Modbus(data, size);
	case ChecksumType::Crc16Arc:        return crc16Arc(data, size);
	case ChecksumType::Crc16CcittFalse: return crc16CcittFalse(data, size);
	case ChecksumType::Crc16Xmodem:     return crc16Xmodem(data, size);
	case ChecksumType::Crc32:           return crc32(data, size);
	case ChecksumType::Crc32c:          return crc32c(data, size);
	}
	return 0;
}

// ------------------------------------------------------------------------------------------------
// Counters
// ------------------------------------------------------------------------------------------------

int maxCounterWidth(CounterEncoding encoding) {
	switch (encoding) {
	case CounterEncoding::AsciiDecimal: return 20;
	case CounterEncoding::AsciiHex:     return 16;
	default:                            return 8;
	}
}

uint64_t maxCounterValue(const CounterSpec& c) {
	const int w = std::clamp(c.width, 1, maxCounterWidth(c.encoding));
	switch (c.encoding) {
	case CounterEncoding::AsciiDecimal: {
		if (w >= 20) return UINT64_MAX;
		uint64_t v = 1;
		for (int i = 0; i < w; ++i) v *= 10;
		return v - 1;
	}
	case CounterEncoding::AsciiHex:
		return Utils::Bits::mask<uint64_t>(static_cast<unsigned>(w * 4));
	default:
		return Utils::Bits::mask<uint64_t>(static_cast<unsigned>(w * 8));
	}
}

SpecError validate(const CounterSpec& c) {
	if (c.width < 1 || c.width > maxCounterWidth(c.encoding)) return SpecError::CounterWidth;
	const uint64_t maxV = maxCounterValue(c);
	if (c.start > maxV || c.end > maxV) return SpecError::CounterRange;
	if (c.step == 0) return SpecError::CounterStep;
	if ((c.end > c.start && c.step < 0) || (c.end < c.start && c.step > 0)) return SpecError::CounterDirection;
	if (!(c.rate > 0.0) || !std::isfinite(c.rate)) return SpecError::CounterRateValue;
	return SpecError::None;
}

uint64_t counterValue(const CounterSpec& c, uint64_t steps) {
	if (c.step == 0) return c.start;
	const uint64_t absStep = c.step < 0 ? uint64_t(0) - static_cast<uint64_t>(c.step) : static_cast<uint64_t>(c.step);
	const uint64_t distance = c.end >= c.start ? c.end - c.start : c.start - c.end;
	const uint64_t lastStep = distance / absStep; // values are start + k * step for k = 0 ... lastStep
	const uint64_t k = lastStep == UINT64_MAX ? steps : steps % (lastStep + 1);
	const uint64_t delta = k * absStep;           // <= distance: no overflow
	return c.end >= c.start ? c.start + delta : c.start - delta;
}

void writeCounter(std::vector<uint8_t>& message, const CounterSpec& c, uint64_t value) {
	const size_t w = static_cast<size_t>(std::clamp(c.width, 1, maxCounterWidth(c.encoding)));
	if (message.size() < c.index + w) message.resize(c.index + w, 0);
	uint8_t* out = message.data() + c.index;
	switch (c.encoding) {
	case CounterEncoding::AsciiDecimal:
		for (size_t i = w; i-- > 0; value /= 10) out[i] = static_cast<uint8_t>('0' + value % 10);
		break;
	case CounterEncoding::AsciiHex:
		for (size_t i = w; i-- > 0; value >>= 4) out[i] = static_cast<uint8_t>("0123456789ABCDEF"[value & 0x0F]);
		break;
	default:
		for (size_t i = 0; i < w; ++i) {
			const uint8_t b = static_cast<uint8_t>(value >> (8 * i)); // i-th least significant byte
			out[c.order == ByteOrder::LittleEndian ? i : w - 1 - i] = b;
		}
		break;
	}
}

// ------------------------------------------------------------------------------------------------
// Index lists
// ------------------------------------------------------------------------------------------------

std::optional<std::vector<size_t>> parseIndexList(std::string_view text) {
	std::set<size_t> out;
	for (std::string_view part : splitOn(text, ',')) {
		while (!part.empty() && part.front() == ' ') part.remove_prefix(1);
		while (!part.empty() && part.back() == ' ') part.remove_suffix(1);
		if (part.empty()) continue; // "1,,2" or a trailing comma
		const size_t dash = part.find('-');
		auto number = [](std::string_view s, size_t& v) {
			while (!s.empty() && s.front() == ' ') s.remove_prefix(1);
			while (!s.empty() && s.back() == ' ') s.remove_suffix(1);
			return !s.empty() && readNumber(s, v);
		};
		size_t a = 0, b = 0;
		if (dash == std::string_view::npos) {
			if (!number(part, a)) return std::nullopt;
			b = a;
		}
		else if (!number(part.substr(0, dash), a) || !number(part.substr(dash + 1), b) || b < a || b - a > 65536) {
			return std::nullopt;
		}
		for (size_t i = a; i <= b; ++i) out.insert(i);
	}
	return std::vector<size_t>(out.begin(), out.end());
}

std::string indexListToString(const std::vector<size_t>& indexes) {
	std::string s;
	for (size_t i = 0; i < indexes.size();) {
		size_t j = i;
		while (j + 1 < indexes.size() && indexes[j + 1] == indexes[j] + 1) ++j;
		if (!s.empty()) s += ", ";
		s += std::to_string(indexes[i]);
		if (j > i) s += "-" + std::to_string(indexes[j]);
		i = j + 1;
	}
	return s;
}

// ------------------------------------------------------------------------------------------------
// Building
// ------------------------------------------------------------------------------------------------

BuildResult buildWithoutChecksum(const MessageSpec& spec, const std::vector<uint64_t>& counterValues) {
	BuildResult r;
	ParseResult parsed = parse(spec.text, spec.format);
	if (!parsed.ok()) {
		r.error = parsed.error;
		r.errorPosition = parsed.errorPosition;
		return r;
	}
	r.bytes = std::move(parsed.bytes);
	for (size_t i = 0; i < spec.counters.size(); ++i)
		writeCounter(r.bytes, spec.counters[i], i < counterValues.size() ? counterValues[i] : spec.counters[i].start);
	return r;
}

BuildResult build(const MessageSpec& spec, const std::vector<uint64_t>& counterValues) {
	BuildResult r = buildWithoutChecksum(spec, counterValues);
	if (!r.ok() || !spec.checksum.enabled) return r;
	const ChecksumSpec& k = spec.checksum;

	std::vector<uint8_t> covered;
	covered.reserve(r.bytes.size());
	for (size_t i = 0; i < r.bytes.size(); ++i)
		if (!std::binary_search(k.excluded.begin(), k.excluded.end(), i)) covered.push_back(r.bytes[i]);
	const ChecksumInfo& info = checksumInfo(k.type);
	const uint64_t full = computeChecksum(k.type, covered.data(), covered.size());

	const int n = (k.bytes <= 0 || k.bytes > info.bytes) ? info.bytes : k.bytes;
	const uint64_t part = (n < info.bytes && k.upperPart) ? full >> (8 * (info.bytes - n)) : full;
	std::vector<uint8_t> sum(static_cast<size_t>(n));
	for (int i = 0; i < n; ++i) {
		const uint8_t b = static_cast<uint8_t>(part >> (8 * i)); // i-th least significant byte
		sum[static_cast<size_t>(k.order == ByteOrder::LittleEndian ? i : n - 1 - i)] = b;
	}
	const size_t at = (k.insertAt < 0 || static_cast<size_t>(k.insertAt) > r.bytes.size()) ? r.bytes.size() : static_cast<size_t>(k.insertAt);
	r.bytes.insert(r.bytes.begin() + static_cast<std::ptrdiff_t>(at), sum.begin(), sum.end());
	r.checksumAt = at;
	r.checksumBytes = sum.size();
	r.checksumValue = full;
	return r;
}

std::vector<uint64_t> MessageGenerator::currentValues(uint64_t nowMs) const {
	std::vector<uint64_t> values;
	values.reserve(spec_.counters.size());
	for (const CounterSpec& c : spec_.counters) {
		const double progress = c.rateMode == CounterRate::PerSecond
			? static_cast<double>(nowMs >= startMs_ ? nowMs - startMs_ : 0) / 1000.0 * c.rate
			: static_cast<double>(sent_) * c.rate;
		// The small epsilon keeps 10 * 0.1 at 1 step despite binary rounding.
		const double steps = std::floor(progress + 1e-9);
		values.push_back(counterValue(c, steps >= 1.8e19 ? UINT64_MAX : static_cast<uint64_t>(steps)));
	}
	return values;
}

BuildResult MessageGenerator::next(uint64_t nowMs) {
	BuildResult r = build(spec_, currentValues(nowMs));
	++sent_;
	return r;
}

// ------------------------------------------------------------------------------------------------
// Text form
// ------------------------------------------------------------------------------------------------

std::string toString(const MessageSpec& spec) {
	std::string counters;
	for (size_t i = 0; i < spec.counters.size(); ++i) counters += (i ? "&" : "") + counterToString(spec.counters[i]);
	return std::string("f=") + formatKey(spec.format) + ";t=" + escape(spec.text) + ";k=" + checksumToString(spec.checksum) + ";c=" + counters;
}

std::optional<MessageSpec> messageFromString(std::string_view text) {
	auto f = readFields(text);
	MessageSpec spec;
	const auto format = formatFromKey(f["f"]);
	const auto body = unescape(f["t"]);
	if (!format || !body) return std::nullopt;
	spec.format = *format;
	spec.text = *body;
	if (!f["k"].empty()) {
		const auto k = checksumFromString(f["k"]);
		if (!k) return std::nullopt;
		spec.checksum = *k;
	}
	if (!f["c"].empty()) {
		for (std::string_view part : splitOn(f["c"], '&')) {
			const auto c = counterFromString(part);
			if (!c) return std::nullopt;
			spec.counters.push_back(*c);
		}
	}
	return spec;
}

std::string toString(const AutoReplyRule& rule) {
	return "n=" + escape(rule.name) + ";e=" + (rule.enabled ? "1" : "0") + ";pf=" + formatKey(rule.patternFormat)
		+ ";p=" + escape(rule.patternText) + ";d=" + std::to_string(rule.delayMs) + ";r=" + escape(toString(rule.reply));
}

std::optional<AutoReplyRule> autoReplyFromString(std::string_view text) {
	auto f = readFields(text);
	AutoReplyRule rule;
	const auto name = unescape(f["n"]);
	const auto pattern = unescape(f["p"]);
	const auto format = formatFromKey(f["pf"]);
	const auto reply = unescape(f["r"]);
	if (!name || !pattern || !format || !reply || !readNumber(f["d"], rule.delayMs)) return std::nullopt;
	const auto spec = messageFromString(*reply);
	if (!spec) return std::nullopt;
	rule.name = *name;
	rule.enabled = f["e"] == "1";
	rule.patternFormat = *format;
	rule.patternText = *pattern;
	rule.reply = *spec;
	return rule;
}

std::string toString(const PeriodicMessage& p) {
	return "n=" + escape(p.name) + ";e=" + (p.enabled ? "1" : "0") + ";ms=" + std::to_string(p.periodMs) + ";m=" + escape(toString(p.message));
}

std::optional<PeriodicMessage> periodicFromString(std::string_view text) {
	auto f = readFields(text);
	PeriodicMessage p;
	const auto name = unescape(f["n"]);
	const auto message = unescape(f["m"]);
	if (!name || !message || !readNumber(f["ms"], p.periodMs) || p.periodMs <= 0) return std::nullopt;
	const auto spec = messageFromString(*message);
	if (!spec) return std::nullopt;
	p.name = *name;
	p.enabled = f["e"] == "1";
	p.message = *spec;
	return p;
}

// ------------------------------------------------------------------------------------------------
// Pattern matching
// ------------------------------------------------------------------------------------------------

void PatternMatcher::setPatterns(std::vector<std::vector<uint8_t>> patterns) {
	patterns_ = std::move(patterns);
	longest_ = 0;
	for (const auto& p : patterns_) longest_ = (std::max)(longest_, p.size());
	tail_.clear();
}

std::vector<size_t> PatternMatcher::feed(const uint8_t* data, size_t size) {
	std::vector<size_t> found;
	if (longest_ == 0 || size == 0) return found;
	const size_t old = tail_.size();
	tail_.insert(tail_.end(), data, data + size);
	// Every occurrence that ENDS in the new bytes, in the order they end - found once, never again.
	for (size_t end = old + 1; end <= tail_.size(); ++end) {
		for (size_t i = 0; i < patterns_.size(); ++i) {
			const auto& p = patterns_[i];
			if (p.empty() || p.size() > end) continue;
			if (std::equal(p.begin(), p.end(), tail_.begin() + static_cast<std::ptrdiff_t>(end - p.size()))) found.push_back(i);
		}
	}
	if (tail_.size() > longest_ - 1) tail_.erase(tail_.begin(), tail_.end() - static_cast<std::ptrdiff_t>(longest_ - 1));
	return found;
}

} // namespace SerialData
