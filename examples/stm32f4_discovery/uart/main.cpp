/*
 * Copyright (c) 2011, Georgi Grinshpun
 * Copyright (c) 2011-2012, Fabian Greif
 * Copyright (c) 2012, 2014, Sascha Schade
 * Copyright (c) 2013, Kevin Läufer
 * Copyright (c) 2013, 2015-2016, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <modm/architecture/platform.hpp>

// ----------------------------------------------------------------------------
/**
 * Very basic example of USART usage.
 * The ASCII sequence 'A', 'B', 'C, ... , 'Z', 'A', 'B', 'C', ...
 * is printed with 9600 baud, 8N1 at pin PA3.
 */
int
main()
{
	Board::initialize();

	Board::LedRed::set();

	// Enable USART 2
	GpioOutputA2::connect(Usart2::Tx);
	GpioInputA3::connect(Usart2::Rx, Gpio::InputType::PullUp);
	Usart2::initialize<Board::systemClock, 9600>(12);

	while (1)
	{
		static uint8_t c = 'A';
		Board::LedRed::toggle();
		Board::LedGreen::toggle();
		Usart2::write(c);
		++c;
		if (c > 'Z') {
			c = 'A';
		}
		modm::delayMilliseconds(500);
	}

	return 0;
}
