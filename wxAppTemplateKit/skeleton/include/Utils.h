/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

/*!
 * \file Utils.h
 * \brief All general-purpose helpers in one include (the successor of the old pUtl class, split by
 * topic into small namespaces):
 *
 * | Namespace          | Header         | For                                                        |
 * |--------------------|----------------|------------------------------------------------------------|
 * | Utils::Time        | TimeUtils.h    | clocks, timestamps, durations, packed timestamps, sleeping |
 * | Utils::Hex         | HexUtils.h     | numbers/bytes <-> hex and binary text, hex dumps, parsing  |
 * | Utils::Bits        | HexUtils.h     | bit get/set/extract/insert, byte order, float bit patterns |
 * | Utils::Checksum    | HexUtils.h     | XOR/sum, CRC-16 (Modbus, CCITT), CRC-32                    |
 * | Utils::Str         | TextUtils.h    | trim, split, join, case, parse numbers                     |
 * | Utils::Files       | TextUtils.h    | safe file names, unique paths, atomic writes, sizes        |
 * | Utils::Math        | MathUtils.h    | range mapping, rounding, tolerant comparison               |
 * | Utils::Hash        | HashUtils.h    | SHA-256                                                    |
 * | Utils::Gui         | GuiUtils.h     | padding, colours, enable/disable panels, console text      |
 * | ThreadUtils        | ThreadUtils.h  | AliveGuard, CallLater, WorkerQueue                         |
 *
 * Including only the header you need keeps compile times down; this one is for convenience.
 */

#include "GuiUtils.h"
#include "HashUtils.h"
#include "HexUtils.h"
#include "MathUtils.h"
#include "TextUtils.h"
#include "ThreadUtils.h"
#include "TimeUtils.h"
