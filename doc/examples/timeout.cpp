/*
 * Copyright (c) 2010, Fabian Greif
 * Copyright (c) 2015, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

// create a new 20 ms timeout
ShortTimeout delay(20);

while (!delay.isExpired()) {
	// wait until the 20 ms have passed
}

delay.restart(40);	// reset to 40 ms

delay.stop();		// stop timer
