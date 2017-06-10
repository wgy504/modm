/*
 * Copyright (c) 2012-2013, Fabian Greif
 * Copyright (c) 2012-2014, Sascha Schade
 * Copyright (c) 2013-2014, 2016-2017, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

/**
 * Display with parallel port on FSMC
 * ==================================
 *
 * This example is to demonstrate a display with a parallel data bus connected
 * to the flexible static memory controller (FSMC).
 *
 * Using the FSMC will show the highest performance in updating the display
 * content.
 *
 * Hardware used
 * -------------
 *  o STM32F4 breakout board (stm32f407_breakout.brd)
 *  o Siemens S75 Display
 *
 * Connection
 * ----------
 * See siemens_s75.hpp for the connection between the display and the STM32.
 * As the FSMC pins must be used there is no choice for D0 to D7, Cs and Wr.
 *
 * Reset can be connected to any GPIO. Cd can be connected to any of the
 * FSMC Address pins (A0 .. A25) when the address of TftMemoryBus8Bit is
 * adjusted.
 *
 * Expected outcome
 * ----------------
 * Whe the program runs a "Hello world" should marquee over the screen.
 *
 * Alternative use
 * ---------------
 * The S75 display driver works with any parallel bus. To demonstrate this
 * feature the FSMC is not used and a parallel bus is constructed from
 * GPIOs.
 *
 * See also
 * --------
 * examples/lpcxpresso/lpc11c24/display/s75
 *
 */

#include <modm/architecture/architecture.hpp>
#include <modm/driver/bus/tft_memory_bus.hpp>
#include <modm/driver/display/siemens_s75.hpp>

// ----------------------------------------------------------------------------
using namespace modm::platform;
using namespace modm::platform::fsmc;

typedef GpioOutputA8 Led;

namespace lcd
{
    typedef GpioD14 D0;
	typedef GpioD15 D1;
	typedef GpioD0  D2;
	typedef GpioD1  D3;
	typedef GpioE7  D4;
	typedef GpioE8  D5;
	typedef GpioE9  D6;
	typedef GpioE10 D7;

	// The Command / Data Pin is mapped to an address pin of the FSMC.
	// GPIO_OUTPUT(Cd,    D, 11);	// Command / Data,  FSMC: A16
	typedef GpioOutputE2 Cd;		// Command / Data,  FSMC: A23

	typedef GpioOutputD7 Cs;		// Chip Select,     FSMC: NE1
	typedef GpioOutputD5 Wr;		// Write operation, FSMC: NWE

	typedef GpioOutputE3 Reset;     // Reset, not part of FSMC

	// FSMC
	typedef modm::TftMemoryBus8Bit ParallelBus;

	// non-FSMC: GPIO
//	typedef modm::gpio::Port<D7, D6, D5, D4, D3, D2, D1, D0> Port;
//	typedef modm::TftMemoryBus8BitGpio<Port, Cs, modm::gpio::Unused, Wr, Cd> ParallelBus;

	// Display
	typedef modm::SiemensS75LandscapeRight<lcd::ParallelBus, lcd::Reset> Display;
}

/**
 * Base Address: 0x60000000 for FSMC Bank 1 (first bank)
 * Offset for A16 : (1 << 16)
 * Offset for A23 : (1 << 23)
 */
lcd::ParallelBus
parallelBus(
		(volatile uint8_t *)  0x60000000,
		(volatile uint8_t *) (0x60000000 + (1 << 23)));

// non-FSMC: GPIO
//lcd::ParallelBus
//parallelBus;

lcd::Display display(parallelBus);

typedef SystemClock<Pll<ExternalCrystal<MHz25>, MHz168, MHz48> > defaultSystemClock;

// ----------------------------------------------------------------------------
int
main()
{
	// Switch STM32F4 to 168 MHz (HSE clocked by an 25 MHz external clock)
    defaultSystemClock::enable();

	Led::setOutput(modm::Gpio::Low);

	lcd::Reset::setOutput(modm::Gpio::Low);

	//------------------------------------------------------

	// FSMC
	modm::platform::Fsmc::initialize();

	// A23
    lcd::Cd::connect(Fsmc::A23, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);

	// FSMC_NE1
    lcd::Cs::connect(Fsmc::Ne1, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);

	// FSMC_NWE
	lcd::Wr::connect(Fsmc::Nwe, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);

    lcd::D0::connect(Fsmc::D0, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);
    lcd::D1::connect(Fsmc::D1, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);
    lcd::D2::connect(Fsmc::D2, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);
    lcd::D3::connect(Fsmc::D3, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);
    lcd::D4::connect(Fsmc::D4, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);
    lcd::D5::connect(Fsmc::D5, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);
    lcd::D6::connect(Fsmc::D6, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);
    lcd::D7::connect(Fsmc::D7, modm::platform::Gpio::OutputType::PushPull, modm::platform::Gpio::OutputSpeed::MHz100);


	modm::platform::fsmc::NorSram::AsynchronousTiming timing = {
		// read
		0,	// readAddressSetup
		15,	// readAddressHold
		0,	// readDataPhase

		// write
		0,	// writeAddressSetup
		0,	// writeAddressHold
		6,	// writeDataPhase

		// bus turn around
		0
	};

	modm::platform::fsmc::NorSram::configureAsynchronousRegion(
			modm::platform::fsmc::NorSram::CHIP_SELECT_1, /* NE1 */
			modm::platform::fsmc::NorSram::NO_MULTIPLEX_8BIT,
			modm::platform::fsmc::NorSram::SRAM_ROM,
			modm::platform::fsmc::NorSram::MODE_A,
			timing);

	modm::platform::fsmc::NorSram::enableRegion(modm::platform::fsmc::NorSram::CHIP_SELECT_1);

	//------------------------------------------------------

	// non-FSMC: GPIO
//	lcd::Cd::setOutput();
//	lcd::Cs::setOutput();
//	lcd::Wr::setOutput();
//
//	lcd::D0::setInput();
//	lcd::D1::setInput();
//	lcd::D2::setInput();
//	lcd::D3::setInput();
//	lcd::D4::setInput();
//	lcd::D5::setInput();
//	lcd::D6::setInput();
//	lcd::D7::setInput();
//
//	lcd::ParallelBus::initialize();

	//------------------------------------------------------

	display.initialize();
	display.setFont(modm::font::Assertion);

	while (1)
	{
		static uint8_t x = 0;
		display.clear();
		display.setCursor(x, 5);
		display << "Hello";
		display.setCursor(46, 24);
		display << "World! abcdefghijk";

		// finished, copy to LCD
		Led::set();
		display.update();
		Led::reset();

		modm::delayMilliseconds(20);

		if (++x > 170)
		{
			x = 0;
		}
	}

	return 0;
}
