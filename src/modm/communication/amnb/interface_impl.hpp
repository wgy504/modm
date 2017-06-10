/*
 * Copyright (c) 2009-2010, Martin Rosekeit
 * Copyright (c) 2009-2012, Fabian Greif
 * Copyright (c) 2011-2013, 2015, Niklas Hauser
 * Copyright (c) 2012-2013, Sascha Schade
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#ifndef	MODM_AMNB_INTERFACE_HPP
#	error	"Don't include this file directly, use 'interface.hpp' instead!"
#endif

#include <modm/architecture/driver/atomic/lock.hpp>

uint_fast16_t modm::amnb::Clock::time = 0;

modm::Timestamp
modm::amnb::Clock::now()
{
	uint_fast16_t tempTime;
	{
		atomic::Lock lock;
		tempTime = time;
	}
	
	return Timestamp(tempTime);
}

// ----------------------------------------------------------------------------
#ifdef __AVR__
#	include <util/crc16.h>
#endif

uint8_t
modm::amnb::crcUpdate(uint8_t crc, uint8_t data)
{
#ifdef __AVR__
	return _crc_ibutton_update(crc, data);
#else
	crc = crc ^ data;
	for (uint_fast8_t i = 0; i < 8; ++i)
	{
		if (crc & 0x01) {
			crc = (crc >> 1) ^ 0x8C;
		}
		else {
			crc >>= 1;
		}
	}
	return crc;
#endif
}

// ----------------------------------------------------------------------------
template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT> typename modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::State \
	modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::state = SYNC;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT> 
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::rx_buffer[maxPayloadLength + 3];

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT> 
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::tx_buffer[maxPayloadLength + 4];

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::crc;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::position;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::length;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::lengthOfReceivedMessage = 0;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::lengthOfTransmitMessage = 0;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::hasMessageToSend = false;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::rescheduleTransmit = false;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::transmitting = false;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
modm::ShortTimeout modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::resetTimer;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
modm::Timeout<modm::amnb::Clock> modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::rescheduleTimer;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::rescheduleTimeout;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::messageSent = false;

#if AMNB_TIMING_DEBUG
template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
modm::Timestamp modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::latency;

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::collisions;
#endif

// ----------------------------------------------------------------------------

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
void
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::initialize(int seed)
{
	srand(seed);
	rescheduleTimeout = static_cast<uint8_t>(rand());
	state = SYNC;
}

// ----------------------------------------------------------------------------

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::writeMessage()
{
	uint8_t check;
	transmitting = true;
	Device::resetErrorFlags();
	
	for (uint_fast8_t i=0; i < lengthOfTransmitMessage; ++i) {
		Device::write(tx_buffer[i]);
		
		// try and read the transmitted byte back but do not wait infinity
		uint16_t count(0);
		while (!Device::read(check) && (++count <= 1000)) ;
		
		// if the read timed out or framing error occurred or content mismatch
		if ((count > 1000) || Device::readErrorFlags() || (check != tx_buffer[i])) {
			// stop transmitting, signal the collision
			transmitting = false;
			rescheduleTransmit = true;
			Device::resetErrorFlags();
			// and wait for a random amount of time before sending again
			rescheduleTimer.restart(rescheduleTimeout % TIMEOUT);
#if AMNB_TIMING_DEBUG
			++collisions;
#endif
			return false;
		}
	}
	
#if AMNB_TIMING_DEBUG
	latency = modm::Clock::now() - latency;
#endif
	
	messageSent = true;
	hasMessageToSend = false;
	transmitting = false;
	return true;
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::sendMessage(uint8_t address, Flags flags, 
		uint8_t command,
		const void *payload, uint8_t payloadLength)
{
	 // dont overwrite the buffer when transmitting
	if (transmitting) return false;
	
	hasMessageToSend = false;
	messageSent = false;
	uint8_t crc;

#if AMNB_TIMING_DEBUG
	latency = modm::Clock::now();
#endif
	
	tx_buffer[0] = syncByte;
	tx_buffer[1] = payloadLength;
	tx_buffer[2] = address | flags;
	tx_buffer[3] = command;
	
	crc = crcUpdate(crcInitialValue, payloadLength);
	crc = crcUpdate(crc, address | flags);
	crc = crcUpdate(crc, command);
	
	const uint8_t *ptr = static_cast<const uint8_t *>(payload);
	uint_fast8_t i=0;
	for (; i < payloadLength; ++i)
	{
		crc = crcUpdate(crc, *ptr);
		tx_buffer[i+4] = *ptr;
		ptr++;
	}
	
	tx_buffer[i+4] = crc;
	
	lengthOfTransmitMessage = payloadLength + 5;
	hasMessageToSend = true;
	return true;
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT> template <typename T>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::sendMessage(uint8_t address, Flags flags,
		uint8_t command,
		const T& payload)
{
	return sendMessage(address, flags,
				command,
				reinterpret_cast<const void *>(&payload), sizeof(T));
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::sendMessage(uint8_t address, Flags flags, uint8_t command)
{
	return sendMessage(address, flags,
				command,
				0, 0);
}

// ----------------------------------------------------------------------------

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::isMessageAvailable()
{
	return (lengthOfReceivedMessage != 0);
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::getTransmittedAddress()
{
	return (tx_buffer[0] & 0x3f);
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::getTransmittedCommand()
{
	return tx_buffer[1];
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
modm::amnb::Flags
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::getTransmittedFlags()
{
	return static_cast<Flags>(tx_buffer[0] & 0xc0);
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::getAddress()
{
	return (rx_buffer[0] & 0x3f);
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::getCommand()
{
	return rx_buffer[1];
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::isResponse()
{
	return (rx_buffer[0] & 0x80);
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::isAcknowledge()
{
	return (rx_buffer[0] & 0x40);
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::isBusAvailable()
{
	return (state == SYNC) && !transmitting && !rescheduleTransmit;
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
bool
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::messageTransmitted()
{
	return messageSent;
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
const uint8_t*
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::getPayload()
{
	return rx_buffer+2;
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
uint8_t
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::getPayloadLength()
{
	return (lengthOfReceivedMessage - 3);
}

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
void
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::dropMessage()
{
	lengthOfReceivedMessage = 0;
}

// ----------------------------------------------------------------------------

template <typename Device, uint8_t PROBABILITY, uint8_t TIMEOUT>
void
modm::amnb::Interface<Device,PROBABILITY,TIMEOUT>::update()
{
	uint8_t byte;
	while (Device::read(byte))
	{
		if (Device::readErrorFlags())
		{
			// collision has been detected
			rescheduleTransmit = true;
			Device::resetErrorFlags();
			
			// erase the message in the buffer
			Device::flushReceiveBuffer();
			
			// and wait for a random amount of time before sending again
			rescheduleTimeout = static_cast<uint8_t>(rand());
			rescheduleTimer.restart(rescheduleTimeout % TIMEOUT);
			state = SYNC;
#if AMNB_TIMING_DEBUG
			++collisions;
#endif
			return;
		}
		
		switch (state)
		{
			case SYNC:
				if (byte == syncByte) state = LENGTH;
				break;
			
			case LENGTH:
				if (byte > maxPayloadLength) {
					state = SYNC;
				}
				else {
					length = byte + 3;		// +3 for header, command and crc byte
					position = 0;
					crc = crcUpdate(crcInitialValue, byte);
					state = DATA;
				}
				break;
			
			case DATA:
				rx_buffer[position++] = byte;
				crc = crcUpdate(crc, byte);
				
				if (position >= length)
				{
					if (crc == 0) lengthOfReceivedMessage = length;
					state = SYNC;
				}
				break;
			
			default:
				state = SYNC;
				break;
		}
		
		resetTimer.restart(resetTimeout);
	}
	if ((state != SYNC) && resetTimer.isExpired()) state = SYNC;
	
	// check if we have waited for a random amount of time
	if (rescheduleTransmit && rescheduleTimer.isExpired()) rescheduleTransmit = false;
	
	if (hasMessageToSend && !rescheduleTransmit && !transmitting && (state == SYNC)) {
		// if channel is free, send with probability P
		if (rescheduleTimeout < static_cast<uint8_t>(2.56f * PROBABILITY)) {
			writeMessage();
		}
		// otherwise reschedule
		else {
			rescheduleTransmit = true;
			rescheduleTimer.restart(rescheduleTimeout % TIMEOUT);
		}
		rescheduleTimeout = static_cast<uint8_t>(rand());
	}
}
