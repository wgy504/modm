/*
 * Copyright (c) 2014-2015, 2017, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#ifndef MODM_INTERFACE_I2C_MASTER_HPP
#define MODM_INTERFACE_I2C_MASTER_HPP

#include "i2c.hpp"
#include "i2c_transaction.hpp"

namespace modm
{

/**
 * Interface of a I2C master.
 *
 * Performing transfers in background allows the use of DMA etc. and
 * enables the program to do other things while the operation is
 * going on.
 *
 * @author	Georgi Grinshpun
 * @author	Niklas Hauser
 * @ingroup i2c
 */
class I2cMaster : public ::modm::PeripheralDriver, public ::modm::I2c
{
public:
	/// Errors that can happen during master operation
	enum class
	Error : uint8_t
	{
		NoError,			///< No Error occurred
		SoftwareReset,		///< The master was reset in software
		AddressNack,		///< Address was transmitted and NACK received
		DataNack,			///< Data was transmitted and NACK received
		ArbitrationLost,	///< Arbitration was lost during writing or reading
		BusCondition,		///< Misplaced Start or Stop condition
		BusBusy,			///< Bus is busy during Start condition
		Unknown				///< Unknown error condition
	};

	/// Baudrate of the I2C bus. Most slaves only work in Standard or Fast mode.
	enum
	Baudrate : uint32_t
	{
		LowSpeed =   10000,	///< Low-Speed datarate of 10kHz
		Standard =  100000,	///< Standard datarate of 100kHz
		Fast     =  400000,	///< Fast datarate of 400kHz
		FastPlus = 1000000,	///< Fast Plus datarate of 1.0MHz (rarely supported)
		High     = 3400000	///< Super datarate of 3.4MHz (rarely supported)
	};

#ifdef __DOXYGEN__
public:
	/**
	 * Initializes the hardware and sets the datarate.
	 *
	 * It is strongly recommended to reset the slave devices on the bus
	 * after a master reset.
	 * This is usually done in the Gpio Scl connect method.
	 *
	 * @tparam	SystemClock
	 * 		the currently active system clock
	 * @tparam	baudrate
	 * 		the desired baudrate in Hz
	 * @tparam	tolerance
	 * 		the allowed absolute tolerance for the resulting baudrate
	 */
	template< class SystemClock, uint32_t baudrate=Baudrate::Standard,
			uint16_t tolerance = Tolerance::FivePercent >
	static void
	initialize();

	/**
	 * Requests transaction attachment.
	 *
	 * @param	transaction
	 *		object that inherits from the I2cTransaction class.
	 * @param	configuration
	 *		function which configures the master for this transaction.
	 * @return	Caller gains control if `true`. Call has no effect if `false`.
	 */
	static bool
	start(I2cTransaction *transaction, ConfigurationHandler handler = nullptr);

	/**
	 * Perform a software reset of the driver in case of an error and detach
     * the transaction object.
	 */
	static void
	reset();

	/**
	 * Check the error state of the driver.
	 *
	 * The error states are hardware and implementation specific.
	 */
	static Error
	getErrorState();
#endif
};

}	// namespace modm

#endif // MODM_INTERFACE_I2C_MASTER_HPP
