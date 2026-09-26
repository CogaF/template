/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

/*!
 * \file SerialLink.cpp
 * \brief Implementation of SerialLink.h.
 */

#include "SerialLink.h"
#include "HexUtils.h"
#include "Log.h"
#include "TimeUtils.h"

#include <chrono>
#include <thread>


const char* SerialLink::resultName(Result r) {
	switch (r) {
	case Result::Ok:          return "Ok";
	case Result::NotOpen:     return "NotOpen";
	case Result::Busy:        return "Busy";
	case Result::WriteFailed: return "WriteFailed";
	case Result::Timeout:     return "Timeout";
	case Result::Aborted:     return "Aborted";
	case Result::Error:       return "Error";
	}
	return "?";
}

SerialLink::~SerialLink() { close(); }

bool SerialLink::open(const SerialConfig& config, std::string* error) {
	abort_.store(true); // a transaction in progress gives up the lock quickly
	std::unique_lock<std::timed_mutex> lock(mutex_, std::chrono::milliseconds(kLockTimeoutMs));
	abort_.store(false);
	if (!lock.owns_lock()) {
		if (error) *error = "the port is busy";
		return false;
	}
	try {
		if (port_ && port_->isOpen()) port_->close();
		port_ = std::make_unique<serial::Serial>(config.port, config.baudrate, config.timeout(), config.bytesize,
			config.parity, config.stopbits, config.flowcontrol);
		if (!port_->isOpen()) port_->open();
		config_ = config;
		Log::info("SerialLink: opened " + config.describe());
		return true;
	}
	catch (const std::exception& e) {
		port_.reset();
		if (error) *error = e.what();
		Log::error("SerialLink: cannot open " + config.describe() + ": " + e.what());
		return false;
	}
}

void SerialLink::close() {
	abort_.store(true);
	std::unique_lock<std::timed_mutex> lock(mutex_, std::chrono::milliseconds(kLockTimeoutMs));
	abort_.store(false);
	try {
		if (port_ && port_->isOpen()) {
			port_->close();
			Log::info("SerialLink: closed " + config_.port);
		}
	}
	catch (const std::exception& e) {
		Log::warning(std::string("SerialLink: error closing the port: ") + e.what());
	}
	port_.reset();
}

bool SerialLink::isOpen() const {
	std::unique_lock<std::timed_mutex> lock(mutex_, std::chrono::milliseconds(kLockTimeoutMs));
	return lock.owns_lock() && port_ && port_->isOpen();
}

SerialConfig SerialLink::config() const {
	std::unique_lock<std::timed_mutex> lock(mutex_, std::chrono::milliseconds(kLockTimeoutMs));
	return config_;
}

SerialLink::Result SerialLink::transact(const std::vector<uint8_t>& tx, std::vector<uint8_t>& rx, int timeoutMs,
	const FrameComplete& complete, int busSettleMs) {
	rx.clear();
	std::unique_lock<std::timed_mutex> lock(mutex_, std::chrono::milliseconds(kLockTimeoutMs));
	if (!lock.owns_lock()) return Result::Busy;
	if (!port_ || !port_->isOpen()) return Result::NotOpen;
	try {
		port_->flushInput();
		if (busSettleMs > 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(busSettleMs));
			port_->flushInput();
		}
		const size_t written = port_->write(tx.data(), tx.size());
		if (Log::isEnabled(LogLevel::Trace)) Log::trace("SerialLink TX: " + Utils::Hex::bytesToString(tx));
		if (written != tx.size()) {
			Log::warning("SerialLink: wrote " + std::to_string(written) + " of " + std::to_string(tx.size()) + " bytes.");
			return Result::WriteFailed;
		}

		const uint64_t deadline = Utils::Time::nowMonotonicMs() + static_cast<uint64_t>(timeoutMs > 0 ? timeoutMs : 0);
		const uint64_t quietMs = config_.interByteTimeoutMs > 0 ? config_.interByteTimeoutMs : 10;
		uint64_t lastByteAt = 0;
		std::vector<uint8_t> chunk;
		for (;;) {
			if (abort_.load()) return Result::Aborted;
			const size_t avail = port_->available();
			if (avail > 0) {
				chunk.clear();
				port_->read(chunk, avail);
				rx.insert(rx.end(), chunk.begin(), chunk.end());
				lastByteAt = Utils::Time::nowMonotonicMs();
				if (complete) {
					const size_t frameLen = complete(rx);
					if (frameLen > 0 && rx.size() >= frameLen) {
						rx.resize(frameLen); // anything after the frame is not ours
						break;
					}
				}
				continue;
			}
			const uint64_t now = Utils::Time::nowMonotonicMs();
			// No framing given: the reply is over once the line has been quiet for a while.
			if (!complete && lastByteAt != 0 && now - lastByteAt >= quietMs) break;
			if (now >= deadline) {
				if (Log::isEnabled(LogLevel::Debug))
					Log::debug("SerialLink: timeout, " + std::to_string(rx.size()) + " byte(s) received: " + Utils::Hex::bytesToString(rx));
				return Result::Timeout;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		if (Log::isEnabled(LogLevel::Trace)) Log::trace("SerialLink RX: " + Utils::Hex::bytesToString(rx));
		return Result::Ok;
	}
	catch (const std::exception& e) {
		Log::error(std::string("SerialLink: transaction failed: ") + e.what());
		return Result::Error;
	}
}

SerialLink::Result SerialLink::write(const std::vector<uint8_t>& tx) {
	std::unique_lock<std::timed_mutex> lock(mutex_, std::chrono::milliseconds(kLockTimeoutMs));
	if (!lock.owns_lock()) return Result::Busy;
	if (!port_ || !port_->isOpen()) return Result::NotOpen;
	try {
		return port_->write(tx.data(), tx.size()) == tx.size() ? Result::Ok : Result::WriteFailed;
	}
	catch (const std::exception& e) {
		Log::error(std::string("SerialLink: write failed: ") + e.what());
		return Result::Error;
	}
}

size_t SerialLink::readAvailable(std::vector<uint8_t>& out) {
	std::unique_lock<std::timed_mutex> lock(mutex_, std::chrono::milliseconds(kLockTimeoutMs));
	if (!lock.owns_lock() || !port_ || !port_->isOpen()) return 0;
	try {
		const size_t avail = port_->available();
		if (avail == 0) return 0;
		std::vector<uint8_t> chunk;
		const size_t n = port_->read(chunk, avail);
		out.insert(out.end(), chunk.begin(), chunk.end());
		return n;
	}
	catch (const std::exception& e) {
		Log::error(std::string("SerialLink: read failed: ") + e.what());
		return 0;
	}
}
