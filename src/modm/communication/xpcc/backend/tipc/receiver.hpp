/*
 * Copyright (c) 2009-2011, Fabian Greif
 * Copyright (c) 2009-2010, 2014, Martin Rosekeit
 * Copyright (c) 2012, 2014, Niklas Hauser
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#ifndef XPCC_TIPC_RECEIVER_HPP
#define XPCC_TIPC_RECEIVER_HPP

#include <queue>

#include <mutex>
#include <thread>
#include <memory>

#include <modm/container/smart_pointer.hpp>

#include "receiver_socket.hpp"

namespace xpcc
{
	namespace tipc
	{
		/**
		 * \brief	Receive Packets over the TIPC and store them.
		 *
		 * In a separate thread the packets are taken from the TIPC and saved local.
		 *
		 * \ingroup	tipc
		 * \author	Carsten Schmitt
		 */
		class Receiver
		{
		public:
			/**
			 * \param ignoreTipcPortId from this port all messages will be ignored, use this to ignore own transmitted messanges
			 *
			 * \see TransmitterSocket::getPortId
			 */
			Receiver(uint32_t ignoreTipcPortId);

			~Receiver();

			void
			setDomainId(unsigned int id);

			void
			addEventId(uint8_t id);

			void
			addReceiverId(uint8_t id);

			/// Check if a new packet has arrived
			bool
			hasPacket() const;

			/**
			 * \brief	Read the packet
			 *
			 * This is only valid if hasPacket() has returned \c true.
			 */
			const modm::SmartPointer&
			getPacket() const;

			/**
			 * \brief	Drop the current Packet
			 *
			 * This is only valid if hasPacket() has returned \c true.
			 */
			void
			dropPacket();

		private:
			typedef modm::SmartPointer		Payload;
			typedef std::mutex				Mutex;
			typedef std::lock_guard<Mutex>	MutexGuard;
			typedef	std::thread				Thread;

			bool
			isAlive();

			void*
			runReceiver();

			void
			update();

			ReceiverSocket tipcReceiverSocket_;
			uint32_t ignoreTipcPortId_;	// the tipc port ID from that all messages will be ignored
			unsigned int domainId_;

			std::queue<Payload>	packetQueue_;

			std::unique_ptr<Thread> receiverThread_;
			mutable Mutex receiverSocketLock_;
			mutable Mutex packetQueueLock_;

			bool isAlive_;

		};
	}
}

#endif // XPCC_TIPC_RECEIVER_HPP
