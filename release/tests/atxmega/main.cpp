/*
 * Copyright (c) 2011-2012, Fabian Greif
 * Copyright (c) 2012, 2016-2017, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#include <modm/architecture/architecture.hpp>

modm::platform::BufferedUartD0 uart(9600);

GPIO_OUTPUT(Out, A, 0);
GPIO_INPUT(In, A, 1);
GPIO_IO(Io, A, 2);

int
main()
{
	Out::setOutput();
	Out::setOutput(true);
	Out::set();
	Out::reset();	
	Out::toggle();
	
	In::setInput();
	In::read();
	
	Io::setOutput();
	Io::set();
	Io::setInput();
	Io::read();
	
	uart.write('x');
	
	while (1)
	{
	}
}
