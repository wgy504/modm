/*
 * Copyright (c) 2015, Kevin Läufer
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#ifndef MODM_CORTEX_CYCLE_COUNTER_HPP
#define MODM_CORTEX_CYCLE_COUNTER_HPP

#include "../../../device.hpp"

namespace modm
{
namespace cortex
{

/**
 * @brief		CPU Cycle Counter
 * @ingroup		cortex
 *
 * Is enabled in the cortex start-up code
 */
class CycleCounter
{
public:
	static inline uint32_t getCount() {
		return DWT->CYCCNT;
	}
};

} // namespace cortex
} // namespace modm
#endif // MODM_CORTEX_CYCLE_COUNTER_HPP
