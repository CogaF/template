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

#include "SerialData.h"

/*!
 * \file SerialMessage.h
 * \brief Messages built from a template: the text the user wrote (SerialData.h), plus incrementing
 * counters and a checksum - the engine behind the auto replies and the periodic messages.
 *
 * A message is built in three steps (build()):
 *  1. the text is turned into bytes (Hex / ASCII / Mixed notation);
 *  2. every counter WRITES its current value over the bytes from its index on (the message is
 *     extended with zero bytes if it is too short) - so the text shows where the value goes, e.g.
 *     "01 06 00 10 00 00" with a 2-byte counter at index 4;
 *  3. the checksum is calculated over the resulting bytes (except the excluded indexes) and
 *     INSERTED at its index (default: at the end).
 * All indexes count from 0.
 *
 * A counter goes from start towards end by step and then starts again from start (0, 2, 4 ... for
 * 0-5 by 2; counting down when end is below start and step negative). It advances either per message
 * sent (rate 1 = every message, 0.5 = every second message, 2 = two steps per message) or per second
 * (rate 0.1 = one step every 10 s), see MessageGenerator.
 */
namespace SerialData {

	/*! \brief Order of the bytes of a multi-byte value on the line. */
	enum class ByteOrder {
		BigEndian,   /*!< most significant byte first (Modbus registers, "network order") */
		LittleEndian /*!< least significant byte first (Modbus CRC, Intel/PC memory) */
	};

	/*! \brief The checksums a message can carry. */
	enum class ChecksumType {
		Sum8,            /*!< sum of the bytes, 8 bits */
		Sum16,           /*!< sum of the bytes, 16 bits */
		Xor8,            /*!< XOR of the bytes (BCC) */
		Lrc8,            /*!< two's complement of the 8-bit sum (LRC, Modbus ASCII, Intel HEX) */
		Crc8,            /*!< CRC-8 (SMBus) */
		Crc8Maxim,       /*!< CRC-8/MAXIM (Dallas 1-Wire) */
		Crc16Modbus,     /*!< CRC-16/MODBUS */
		Crc16Arc,        /*!< CRC-16/ARC (IBM) */
		Crc16CcittFalse, /*!< CRC-16/CCITT-FALSE */
		Crc16Xmodem,     /*!< CRC-16/XMODEM */
		Crc32,           /*!< CRC-32 (IEEE, zip) */
		Crc32c           /*!< CRC-32C (Castagnoli) */
	};
	/*! \brief Facts about one ChecksumType. */
	struct ChecksumInfo {
		ChecksumType type;      /*!< the checksum */
		const char* key;        /*!< settings key ("crc16modbus"), never translated */
		const char* name;       /*!< display name ("CRC-16/MODBUS"), a technical name - not translated */
		int bytes;              /*!< size of the full result in bytes */
		ByteOrder usualOrder;   /*!< the order protocols normally send it in */
	};
	/*! \brief All checksum types, in the order the dialog lists them. */
	const std::vector<ChecksumInfo>& checksumTypes();
	/*! \brief The facts about type. */
	const ChecksumInfo& checksumInfo(ChecksumType type);
	/*! \brief The full checksum of data (as many bytes as checksumInfo(type).bytes). */
	uint64_t computeChecksum(ChecksumType type, const uint8_t* data, size_t size);

	/*! \brief A checksum added to a message (see the file comment). */
	struct ChecksumSpec {
		bool enabled = false;                   /*!< false = no checksum */
		ChecksumType type = ChecksumType::Sum8; /*!< algorithm */
		int insertAt = -1;                      /*!< index where its bytes are inserted; -1 = at the end (follows the length) */
		std::vector<size_t> excluded;           /*!< indexes (of the message before insertion) left out of the calculation */
		int bytes = 0;                          /*!< bytes to send; 0 or the full size = all; fewer = a part of the result */
		bool upperPart = false;                 /*!< with fewer bytes: the most significant ones (true) or the least (false) */
		ByteOrder order = ByteOrder::BigEndian; /*!< order of the sent bytes */
	};

	/*! \brief How a counter value is written into the message. */
	enum class CounterEncoding {
		Binary,       /*!< width bytes, in the chosen byte order */
		AsciiDecimal, /*!< width decimal digits, zero-padded ("0042") */
		AsciiHex      /*!< width hex digits, upper case, zero-padded ("002A") */
	};
	/*! \brief What makes a counter advance. */
	enum class CounterRate {
		PerMessage, /*!< rate steps for every message sent by its generator */
		PerSecond   /*!< rate steps per second since the generator was (re)started */
	};
	/*! \brief An incrementing value written into a message (see the file comment). */
	struct CounterSpec {
		size_t index = 0;                                  /*!< first byte it occupies */
		int width = 1;                                     /*!< bytes (Binary, 1-8) or characters (ASCII, 1-20 / 1-16) */
		CounterEncoding encoding = CounterEncoding::Binary; /*!< how it is written */
		ByteOrder order = ByteOrder::BigEndian;            /*!< byte order (Binary, width > 1) */
		uint64_t start = 0;                                /*!< first value */
		uint64_t end = 255;                                /*!< the limit: the value never goes past it (below start to count down) */
		int64_t step = 1;                                  /*!< added at every step; negative when end is below start */
		CounterRate rateMode = CounterRate::PerMessage;    /*!< what makes it advance */
		double rate = 1.0;                                 /*!< steps per message or per second (> 0, fractions allowed) */
	};

	/*! \brief Why a CounterSpec or ChecksumSpec is not usable. */
	enum class SpecError {
		None,             /*!< valid */
		CounterWidth,     /*!< width outside the range of its encoding */
		CounterRange,     /*!< start or end does not fit in width */
		CounterStep,      /*!< step is 0 */
		CounterDirection, /*!< step goes away from end (positive with end below start, or negative with end above) */
		CounterRateValue, /*!< rate is not a positive number */
		ChecksumBytes,    /*!< more checksum bytes than the algorithm produces */
		ChecksumExcluded  /*!< the exclusion list is not valid */
	};
	/*! \brief Checks a counter. */
	SpecError validate(const CounterSpec& counter);
	/*! \brief Largest width for an encoding (8 bytes, 20 decimal digits, 16 hex digits). */
	int maxCounterWidth(CounterEncoding encoding);
	/*! \brief Largest value the counter's width and encoding can hold. */
	uint64_t maxCounterValue(const CounterSpec& counter);
	/*!
	 * \brief The counter's value after `steps` steps: start + k * step, where k counts 0, 1, ... up to
	 * the last value that does not pass end, then again from 0.
	 */
	uint64_t counterValue(const CounterSpec& counter, uint64_t steps);
	/*! \brief Writes value over message[index ... index + width - 1], extending message with zeros if needed. */
	void writeCounter(std::vector<uint8_t>& message, const CounterSpec& counter, uint64_t value);

	/*! \brief "1, 3, 5-7" -> {1, 3, 5, 6, 7} (spaces allowed, sorted, no duplicates); nullopt if invalid. */
	std::optional<std::vector<size_t>> parseIndexList(std::string_view text);
	/*! \brief {1, 3, 5, 6, 7} -> "1, 3, 5-7". */
	std::string indexListToString(const std::vector<size_t>& indexes);

	/*! \brief A message template: text + counters + checksum. */
	struct MessageSpec {
		DataFormat format = DataFormat::Hex; /*!< notation of text */
		std::string text;                    /*!< the message as written by the user (UTF-8) */
		std::vector<CounterSpec> counters;   /*!< counters written into it, in order */
		ChecksumSpec checksum;               /*!< optional checksum */
	};

	/*! \brief Outcome of build(). */
	struct BuildResult {
		std::vector<uint8_t> bytes;          /*!< the message (valid only when ok()) */
		ParseError error = ParseError::None; /*!< the text is not valid in its notation */
		size_t errorPosition = 0;            /*!< where (character index) */
		size_t checksumAt = 0;               /*!< index of the first checksum byte (if any) */
		size_t checksumBytes = 0;            /*!< number of checksum bytes (0 = none) */
		uint64_t checksumValue = 0;          /*!< full checksum before taking a part of it */
		/*! \brief true if the message could be built. */
		bool ok() const { return error == ParseError::None; }
	};
	/*!
	 * \brief Builds a message with the given counter values (one per spec.counters entry; missing
	 * ones use the counter's start).
	 */
	BuildResult build(const MessageSpec& spec, const std::vector<uint64_t>& counterValues);
	/*! \brief The bytes the checksum of spec covers, before insertion (text + counters). */
	BuildResult buildWithoutChecksum(const MessageSpec& spec, const std::vector<uint64_t>& counterValues);

	/*!
	 * \brief Produces the successive messages of a MessageSpec, advancing its counters.
	 * Not thread-safe: each thread uses its own.
	 */
	class MessageGenerator {
	public:
		/*! \brief A generator for spec, started at time 0. */
		explicit MessageGenerator(MessageSpec spec = {}) : spec_(std::move(spec)) {}
		/*! \brief Starts again from the start values; per-second counters count from nowMs. */
		void restart(uint64_t nowMs) { startMs_ = nowMs; sent_ = 0; }
		/*! \brief The counter values the next message would carry at nowMs (monotonic ms). */
		std::vector<uint64_t> currentValues(uint64_t nowMs) const;
		/*! \brief Builds the next message and counts it as sent (advancing the per-message counters). */
		BuildResult next(uint64_t nowMs);
		/*! \brief The template. */
		const MessageSpec& spec() const { return spec_; }
		/*! \brief Messages produced since the last restart. */
		uint64_t sentCount() const { return sent_; }

	private:
		MessageSpec spec_;
		uint64_t startMs_ = 0;
		uint64_t sent_ = 0;
	};

	/*! \brief An automatic reply: when pattern is received, send reply. */
	struct AutoReplyRule {
		std::string name;                           /*!< shown in lists and in the console */
		bool enabled = true;                        /*!< false = kept but not used */
		DataFormat patternFormat = DataFormat::Hex; /*!< notation of patternText */
		std::string patternText;                    /*!< the bytes to recognise */
		MessageSpec reply;                          /*!< what to send back */
		int delayMs = 0;                            /*!< wait this long before replying */
	};
	/*! \brief A message sent again and again. */
	struct PeriodicMessage {
		std::string name;    /*!< shown in lists and in the console */
		bool enabled = true; /*!< false = kept but not sent */
		int periodMs = 1000; /*!< time between two messages */
		MessageSpec message; /*!< what to send */
	};

	/*! \brief One-line text form of a MessageSpec for settings files (fromString() reads it back). */
	std::string toString(const MessageSpec& spec);
	/*! \brief A MessageSpec from toString(); nullopt if the text is damaged. */
	std::optional<MessageSpec> messageFromString(std::string_view text);
	/*! \brief One-line text form of an AutoReplyRule. */
	std::string toString(const AutoReplyRule& rule);
	/*! \brief An AutoReplyRule from toString(); nullopt if the text is damaged. */
	std::optional<AutoReplyRule> autoReplyFromString(std::string_view text);
	/*! \brief One-line text form of a PeriodicMessage. */
	std::string toString(const PeriodicMessage& periodic);
	/*! \brief A PeriodicMessage from toString(); nullopt if the text is damaged. */
	std::optional<PeriodicMessage> periodicFromString(std::string_view text);

	/*!
	 * \brief Finds byte patterns in a stream that arrives in pieces: a pattern split between two
	 * reads is still found, and every occurrence is reported exactly once.
	 */
	class PatternMatcher {
	public:
		/*! \brief Sets the patterns to look for (empty ones never match) and forgets past bytes. */
		void setPatterns(std::vector<std::vector<uint8_t>> patterns);
		/*! \brief Forgets the bytes seen so far. */
		void reset() { tail_.clear(); }
		/*!
		 * \brief Adds received bytes; returns the index of the pattern of every occurrence that ends
		 * in them, in the order the occurrences end (a pattern found twice is listed twice).
		 */
		std::vector<size_t> feed(const uint8_t* data, size_t size);

	private:
		std::vector<std::vector<uint8_t>> patterns_;
		std::vector<uint8_t> tail_; // the last (longest pattern - 1) bytes seen
		size_t longest_ = 0;
	};
}
