/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

/*!
 * \file HashUtils.h
 * \brief SHA-256 (FIPS 180-4), portable - no operating-system crypto API needed.
 *
 * Typical use: store the hash of a password instead of the password (compare
 * Utils::Hash::sha256Hex(entered) with the stored hex string). For real password storage add a salt
 * and many iterations (PBKDF2) - a plain hash only keeps the password out of the source code.
 */
namespace Utils::Hash {
	/*! \brief The 32-byte SHA-256 digest of data. */
	std::array<uint8_t, 32> sha256(const void* data, size_t size);
	/*! \brief The SHA-256 digest of text as 64 lowercase hex characters. */
	std::string sha256Hex(std::string_view text);
}
