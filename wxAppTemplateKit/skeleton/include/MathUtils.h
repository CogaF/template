/*
 * Copyright (C) 2026 Fation Coga
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * This file is part of Template App - see COPYING and COPYING.LESSER.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>

/*!
 * \file MathUtils.h
 * \brief Small numeric helpers: range mapping (e.g. value -> pixel), rounding, tolerant comparison.
 */
namespace Utils::Math {

	/*!
	 * \brief Maps value from [inMin, inMax] to [outMin, outMax] linearly - e.g. a reading to a pixel
	 * position on a graph (the old pUtl::toScreen()), a 0..4095 ADC count to 0..10 V.
	 * \param value        the number to map.
	 * \param inMin        start of the input range.
	 * \param inMax        end of the input range.
	 * \param outMin       start of the output range.
	 * \param outMax       end of the output range.
	 * \param clampToRange true to keep the result inside [outMin, outMax].
	 * \return outMin if inMin == inMax.
	 */
	inline double mapRange(double value, double inMin, double inMax, double outMin, double outMax, bool clampToRange = false) {
		if (inMax == inMin) return outMin;
		double r = outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin);
		if (clampToRange) r = std::clamp(r, (std::min)(outMin, outMax), (std::max)(outMin, outMax));
		return r;
	}

	/*! \brief value rounded to decimals digits after the point (2 -> 3.14159 becomes 3.14). */
	inline double roundTo(double value, int decimals) {
		const double f = std::pow(10.0, decimals);
		return std::round(value * f) / f;
	}

	/*! \brief true if a and b differ by at most absTol, or by at most relTol of the larger magnitude. */
	inline bool nearlyEqual(double a, double b, double absTol = 1e-9, double relTol = 1e-9) {
		const double diff = std::fabs(a - b);
		return diff <= absTol || diff <= relTol * (std::max)(std::fabs(a), std::fabs(b));
	}

	/*! \brief Percentage of part in total (0 if total is 0). */
	inline double percent(double part, double total) { return total == 0.0 ? 0.0 : 100.0 * part / total; }

	/*! \brief Integer division rounding up, e.g. how many 64-byte packets hold n bytes. */
	template <std::integral T>
	constexpr T divideRoundUp(T numerator, T denominator) { return (numerator + denominator - 1) / denominator; }
}
