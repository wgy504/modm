/*
 * Copyright (c) 2011, Fabian Greif
 * Copyright (c) 2011-2016, Niklas Hauser
 * Copyright (c) 2014, Sascha Schade
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#ifndef MODM_TMP175_HPP
#define MODM_TMP175_HPP

#include <modm/architecture/interface/register.hpp>
#include <modm/architecture/interface/i2c_device.hpp>
#include <modm/processing/protothread.hpp>
#include "lm75.hpp"

namespace modm
{

// forward declaration for friending with tmp175::Data
template < class I2cMaster >
class Tmp175;

struct tmp175 : public lm75
{
protected:
	/// @cond
	enum class
	Config1 : uint8_t
	{
		OneShot = Bit7,
		// Resolution 6:5
		// Fault Queue 4:3
		Polarity = Bit2,
		ThermostatMode = Bit1,
		ShutdownMode = Bit0,
	};
	MODM_FLAGS8(Config1);
	/// @endcond

public:
	enum class
	Resolution : uint8_t
	{
		Bits9 = 0,				///< Conversion Time:  28ms
		Bits10 = Bit0,			///< Conversion Time:  55ms
		Bits11 = Bit1,			///< Conversion Time: 110ms
		Bits12 = Bit1 | Bit0	///< Conversion Time: 220ms
	};
protected:
	/// @cond
	typedef Configuration< Config1_t, Resolution, (Bit1 | Bit0), 5 > Resolution_t;
	/// @endcond
};

/**
 * TMP175 digital temperature sensor driver.
 *
 * The TMP175 is a digital temperature sensor with a two-wire interface
 * and measures temperature over a range of -40 to +125 deg Celsius with a
 * resolution of 1/16 (0.0625) deg C and an accuracy of up to 1.5 deg C.
 *
 * The sensor has a default refresh rate of 4Hz but can be raised up to
 * 30Hz by repeatedly manually starting a conversion (with startConversion()),
 * which lasts between 30ms and 240ms depending on resolution.
 *
 * @ingroup driver_temperature
 * @author	Niklas Hauser
 *
 * @tparam I2cMaster Asynchronous Interface
 */
template < typename I2cMaster >
class Tmp175 :	public tmp175, public Lm75< I2cMaster >,
				protected modm::pt::Protothread
{
public:
	/// Constructor, requires a tmp175::Data object,
	/// sets address to default of 0x48 (alternatives are 0x49, 0x4A and 0x4B).
	Tmp175(Data &data, uint8_t address=0x48);

	void modm_always_inline
	update()
	{ run(); }

	// @param	rate	Update rate in Hz: 1 to 33.
	void
	setUpdateRate(uint8_t rate);

	modm::ResumableResult<bool>
	setResolution(Resolution resolution);

	/// Writes the upper limit of the alarm.
	modm::ResumableResult<bool> modm_always_inline
	setUpperLimit(float temperature)
	{ return setLimitRegister(Register::TemperatureUpperLimit, temperature); }

	/// Writes the lower limit of the alarm.
	modm::ResumableResult<bool> modm_always_inline
	setLowerLimit(float temperature)
	{ return setLimitRegister(Register::TemperatureLowerLimit, temperature); }

	/// starts a temperature conversion right now
	modm::ResumableResult<bool>
	startConversion();

	inline Data&
	getData();

private:
	bool
	run();

	modm::ResumableResult<bool>
	writeConfiguration();

	modm::ResumableResult<bool>
	setLimitRegister(Register reg, float temperature);

	modm::ShortTimeout periodTimeout;
	modm::ShortTimeout conversionTimeout;
	uint16_t updateTime;
	uint8_t conversionTime;
};

} // namespace modm

#include "tmp175_impl.hpp"

#endif // MODM_TMP175_HPP
