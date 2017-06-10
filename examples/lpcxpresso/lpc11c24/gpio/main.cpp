/*
 * Copyright (c) 2012, Sascha Schade
 * Copyright (c) 2014-2015, Kevin Läufer
 * Copyright (c) 2015-2016, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

/**
 * \brief	Minimal example for LPC11C24 on LPCXpresso board.
 *
 * The LED2 is lit when the device boots.
 * If the GPIO P3_2 (lower right corner) is tied low
 * the LED2 is switched off.
 *
 * Tested in hardware 2012-08-08 by Sascha Schade.
 * Tested again 2014-08-01.
 */

#include <modm/architecture/architecture.hpp>
#include "../lpcxpresso11c24.hpp"

int
main()
{
	defaultSystemClock::enable();
	SysTickTimer::initialize<defaultSystemClock>();

	// Set LED port pin to output
	Led::setOutput();
	Button::setInput(Gpio::InputType::PullUp);

	while (1)
	{
		// Each time we wake up...
		Led::set(Button::read());
		// Go to sleep to save power between timer interrupts
		__WFI();
	}

/*
	// Use all other defines once to verify that it is compilable
	But::setInput();
	But::setInput(modm::lpc::FLOATING);
	But::setInput(modm::lpc::PULLUP);
	But::setInput(modm::lpc::PULLDOWN);
	But::setInput(modm::lpc::REPEATER);

	Led::setOutput();
	Led::setOutput(true);
	Led::setOutput(false);
	Led::setOutput(modm::lpc::PUSH_PULL);
	Led::setOutput(modm::lpc::OPEN_DRAIN);
	Led::set();
	Led::reset();
	Led::toggle();

	GPIO_IO(Io, 0, 3);
	Io::setOutput();
	Io::setOutput(true);
	Io::setOutput(false);
	Io::setOutput(modm::lpc::PUSH_PULL);
	Io::setOutput(modm::lpc::OPEN_DRAIN);
	Io::setInput();
	Io::setInput(modm::lpc::FLOATING);
	Io::setInput(modm::lpc::PULLUP);
	Io::setInput(modm::lpc::PULLDOWN);
	Io::setInput(modm::lpc::REPEATER);
	Io::set();
	Io::reset();
	Io::toggle();
	Io::set(true);
	Io::set(false);
	Led::set(Io::read());
*/
}
