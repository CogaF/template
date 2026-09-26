/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "serial/serial.h"

/*!
 * \file SerialConfig.h
 * \brief Everything needed to open a serial port, with text persistence and port discovery.
 */
/*!
 * \brief Settings of one serial port (see the file comment).
 */
struct SerialConfig {
	std::string port; /*!< "COM3" (Windows), "/dev/ttyUSB0" (Linux) */
	uint32_t baudrate = 9600;                                       /*!< bits per second */
	serial::bytesize_t bytesize = serial::eightbits;               /*!< data bits (5-8) */
	serial::parity_t parity = serial::parity_none;                 /*!< parity bit */
	serial::stopbits_t stopbits = serial::stopbits_one;            /*!< stop bits */
	serial::flowcontrol_t flowcontrol = serial::flowcontrol_none;  /*!< flow control */
	// Timeouts in ms (see serial::Timeout): a read returns after interByte ms without a new byte,
	// or after readConstant + readMultiplier * bytes-requested ms in total.
	uint32_t interByteTimeoutMs = 10;       /*!< a read ends after this many ms without a new byte */
	uint32_t readTimeoutConstantMs = 100;   /*!< fixed part of the total read timeout */
	uint32_t readTimeoutMultiplierMs = 0;   /*!< per-byte part of the total read timeout */
	uint32_t writeTimeoutConstantMs = 100;  /*!< fixed part of the total write timeout */
	uint32_t writeTimeoutMultiplierMs = 0;  /*!< per-byte part of the total write timeout */

	/*! \brief true if a port name and a baud rate are set. */
	bool isValid() const { return !port.empty() && baudrate > 0; }

	/*!
	 * \brief serial::Timeout with a safety floor: on Windows a 0/0 constant+multiplier pair means
	 * "block forever" (COMMTIMEOUTS), which can hang a worker thread - and its join() at shutdown -
	 * for good. Such pairs are raised to kMinTimeoutMs.
	 */
	serial::Timeout timeout() const;
	/*! \brief Floor applied by timeout() to a 0/0 constant+multiplier pair. */
	static constexpr uint32_t kMinTimeoutMs = 50;

	/*! \brief "port=COM3;baud=115200;bytesize=8;parity=0;stopbits=1;flow=0;ib=10;rc=100;rm=0;wc=100;wm=0" */
	std::string toString() const;
	/*! \brief Parses toString()'s format; unknown/missing keys keep their defaults. nullopt if no port. */
	static std::optional<SerialConfig> fromString(const std::string& text);
	/*! \brief "COM3 115200 8N1" - for status bars and logs. */
	std::string describe() const;

	/*! \brief Serial ports present right now. */
	static std::vector<serial::PortInfo> listPorts();
	/*! \brief The usual rates, for choice controls. */
	static const std::vector<uint32_t>& standardBaudrates();
};
