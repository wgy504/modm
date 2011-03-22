// coding: utf-8
// ----------------------------------------------------------------------------
/* Copyright (c) 2011, Roboterclub Aachen e.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Roboterclub Aachen e.V. nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ROBOTERCLUB AACHEN E.V. ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROBOTERCLUB AACHEN E.V. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id$
 */
// ----------------------------------------------------------------------------

#ifndef XPCC__AT45DB0X1D_HPP
	#error	"Don't include this file directly, use 'at45db0x1d.hpp' instead!"
#endif

template <typename Spi, typename Cs>
Spi xpcc::At45db0x1d<Spi, Cs>::spi;

template <typename Spi, typename Cs>
Cs xpcc::At45db0x1d<Spi, Cs>::cs;

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
bool
xpcc::At45db0x1d<Spi, Cs>::initialize()
{
	spi.initialize();
	
	cs.set();
	cs.setOutput();
	
	uint8_t status = readStatus();
	if (status == 0xff || status == 0) {
		// no device present
		return false;
	}
	
	if ((status & PAGE_SIZE) == 0)
	{
		// Page size is 264 (the default)
		// => set page size from 264 to 256 bytes
		cs.reset();
		
		// send page size change sequence (fixed sequence)
		spi.write(0x3d);
		spi.write(0x2a);
		spi.write(0x80);
		spi.write(0xa6);
		
		cs.set();
	}
	
	waitUntilReady();
	
	return true;
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::copyPageToBuffer(uint16_t pageAddress, Buffer buffer)
{
	cs.reset();
	
	if (buffer == BUFFER_0) {
		spi.write(MAIN_MEMORY_PAGE_TO_BUFFER_1_TRANSFER);
	}
	else {
		spi.write(MAIN_MEMORY_PAGE_TO_BUFFER_2_TRANSFER);
	}
	
	// set address
	spi.write(pageAddress >> 8);
	spi.write(pageAddress & 0xff);
	spi.write(0);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::comparePageToBuffer(uint16_t pageAddress, Buffer buffer)
{
	cs.reset();
	
	if (buffer == BUFFER_0) {
		spi.write(MAIN_MEMORY_PAGE_TO_BUFFER_1_COMPARE);
	}
	else {
		spi.write(MAIN_MEMORY_PAGE_TO_BUFFER_2_COMPARE);
	}
	
	// set address
	spi.write(pageAddress >> 8);
	spi.write(pageAddress & 0xff);
	spi.write(0);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
bool
xpcc::At45db0x1d<Spi, Cs>::isEqual()
{
	return ((readStatus() & COMP) == 0);
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::copyBufferToPage(Buffer buffer, uint16_t pageAddress)
{
	cs.reset();
	
	if (buffer == BUFFER_0) {
		spi.write(BUFFER_1_TO_MAIN_MEMORY_PAGE_PROGRAM_WITH_ERASE);
	}
	else {
		spi.write(BUFFER_2_TO_MAIN_MEMORY_PAGE_PROGRAM_WITH_ERASE);
	}
	
	// set address
	spi.write(pageAddress >> 8);
	spi.write(pageAddress & 0xff);
	spi.write(0);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::copyBufferToPageWithoutErase(Buffer buffer, uint16_t pageAddress)
{
	cs.reset();
	
	if (buffer == BUFFER_0) {
		spi.write(BUFFER_1_TO_MAIN_MEMORY_PAGE_PROGRAM_WITHOUT_ERASE);
	}
	else {
		spi.write(BUFFER_2_TO_MAIN_MEMORY_PAGE_PROGRAM_WITHOUT_ERASE);
	}
	
	// set address
	spi.write(pageAddress >> 8);
	spi.write(pageAddress & 0xff);
	spi.write(0);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::readFromBuffer(Buffer buffer,
		uint8_t address, uint8_t *data, uint8_t size)
{
	cs.reset();
	if (buffer == BUFFER_0) {
		spi.write(BUFFER_1_READ);
	}
	else {
		spi.write(BUFFER_2_READ);
	}
	
	// set address
	spi.write(0);
	spi.write(0);
	spi.write(address);
	
	// don't care byte
	spi.write(0);
	
	for (uint8_t i = 0; i < size; ++i) {
		*data++ = spi.write(0);
	}
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::writeToBuffer(Buffer buffer,
		uint8_t address, const uint8_t *data, uint8_t size)
{
	cs.reset();
	if (buffer == BUFFER_0) {
		spi.write(BUFFER_1_WRITE);
	}
	else {
		spi.write(BUFFER_2_WRITE);
	}
	
	// set address
	spi.write(0);
	spi.write(0);
	spi.write(address);
	
	for (uint8_t i = 0; i < size; ++i) {
		*data++ = spi.write(0);
	}
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::readFromMemory(uint32_t address, uint8_t *data, std::size_t size)
{
	cs.reset();
	spi.write(CONTINOUS_ARRAY_READ);
	
	// set address
	spi.write(address >> 16);
	spi.write(address >> 8);
	spi.write(address);
	
	for (std::size_t i = 0; i < size; ++i) {
		*data++ = spi.write(0);
	}
	cs.set();
}
		
// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::readPageFromMemory(uint32_t address, uint8_t *data, std::size_t size)
{
	cs.reset();
	spi.write(MAIN_MEMORY_PAGE_READ);
	
	// set address
	spi.write(address >> 16);
	spi.write(address >> 8);
	spi.write(address);
	
	// don't care
	spi.write(0);
	spi.write(0);
	spi.write(0);
	spi.write(0);
	
	for (std::size_t i = 0; i < size; ++i) {
		*data++ = spi.write(0);
	}
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::pageErase(uint16_t pageAddress)
{
	waitUntilReady();
	
	cs.reset();
	
	// set address
	spi.write(pageAddress >> 8);
	spi.write(pageAddress & 0xff);
	spi.write(0);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::pageRewrite(uint16_t pageAddress, Buffer buffer)
{
	cs.reset();
	
	if (buffer == BUFFER_0) {
		spi.write(BUFFER_1_PAGE_REWRITE);
	}
	else {
		spi.write(BUFFER_2_PAGE_REWRITE);
	}
	
	// set address
	spi.write(pageAddress >> 8);
	spi.write(pageAddress & 0xff);
	spi.write(0);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::blockErase(uint16_t blockAddress)
{
	waitUntilReady();
	
	cs.reset();
	
	// set address
	spi.write(blockAddress >> 8);
	spi.write(blockAddress & 0xf8);
	spi.write(0);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::chipErase()
{
	waitUntilReady();
	
	cs.reset();
	
	// send chip erase sequence (fixed sequence)
	spi.write(0xc7);
	spi.write(0x94);
	spi.write(0x80);
	spi.write(0x9a);
	
	cs.set();
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
bool
xpcc::At45db0x1d<Spi, Cs>::isReady()
{
	return (readStatus() & READY);
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
void
xpcc::At45db0x1d<Spi, Cs>::waitUntilReady()
{
	while (!isReady()) {
		// wait until the device is ready
	}
}

// ----------------------------------------------------------------------------
template <typename Spi, typename Cs>
uint8_t
xpcc::At45db0x1d<Spi, Cs>::readStatus()
{
	cs.reset();
	spi.write(READ_STATUS_REGISTER);
	uint8_t result = spi.write(0);		// dummy write to get result
	cs.set();
	
	return result;
}