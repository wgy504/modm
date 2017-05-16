/*
 * Copyright (c) 2011, Fabian Greif
 * Copyright (c) 2013, Kevin Läufer
 * Copyright (c) 2013-2016, Niklas Hauser
 * Copyright (c) 2014, Sascha Schade
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <modm/architecture/platform.hpp>

using namespace Board;

// ----------------------------------------------------------------------------
int
main()
{
	Board::initialize();
	LedD13::setOutput(modm::Gpio::Low);

	// Use the logging streams to print some messages.
	// Change MODM_LOG_LEVEL above to enable or disable these messages
	MODM_LOG_DEBUG   << "debug"   << modm::endl;
	MODM_LOG_INFO    << "info"    << modm::endl;
	MODM_LOG_WARNING << "warning" << modm::endl;
	MODM_LOG_ERROR   << "error"   << modm::endl;

	uint32_t counter(0);

	while (1)
	{
		LedGreen::toggle();
		modm::delayMilliseconds(Button::read() ? 125 : 500);

		LedOrange::toggle();
		modm::delayMilliseconds(Button::read() ? 125 : 500);

		LedRed::toggle();
		modm::delayMilliseconds(Button::read() ? 125 : 500);

		LedBlue::toggle();
		modm::delayMilliseconds(Button::read() ? 125 : 500);
		LedD13::toggle();

		MODM_LOG_INFO << "loop: " << counter++ << modm::endl;
	}

	return 0;
}
