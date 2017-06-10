/*
 * Copyright (c) 2013, Kevin Läufer
 * Copyright (c) 2013-2014, Sascha Schade
 * Copyright (c) 2013, 2016, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <modm/architecture/architecture.hpp>
#include "../stm32f4_loa_v2b.hpp"

int
main()
{
	defaultSystemClock::enable();

	LedWhite::setOutput(modm::Gpio::High);
	LedGreen::setOutput(modm::Gpio::Low);

	while (1)
	{
		LedWhite::toggle();
		LedGreen::toggle();
		modm::delayMilliseconds(100);
	}

	return 0;
}
