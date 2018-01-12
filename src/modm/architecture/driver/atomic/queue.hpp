/*
 * Copyright (c) 2009-2012, Fabian Greif
 * Copyright (c) 2010, Martin Rosekeit
 * Copyright (c) 2012, 2016, Niklas Hauser
 * Copyright (c) 2016, Sascha Schade
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------

#ifndef	MODM_ATOMIC_QUEUE_HPP
#define	MODM_ATOMIC_QUEUE_HPP

#include <cstddef>
#include <stdint.h>
#include <modm/architecture/utils.hpp>
#include <modm/architecture/interface/accessor.hpp>
#include <modm/utils/template_metaprogramming.hpp>

namespace modm
{
	namespace atomic
	{
		/**
		 * \ingroup	atomic
		 * \brief	Interrupt save queue
		 *
		 * A maximum size of 254 is allowed for 8-bit mikrocontrollers.
		 * 
		 * \todo	This implementation should work but could be improved
		 */
		template<typename T,
				 std::size_t N>
		class Queue
		{
		public:
			// select the type of the index variables with some template magic :-)
			typedef typename modm::tmp::Select< (N >= 254),
												uint16_t,
												uint8_t >::Result Index;
			
			typedef Index Size;
			
		public:
			Queue();
			
			modm_always_inline bool
			isFull() const;
			
			modm_always_inline bool
			isNotFull() const { return not isFull(); }

			/**
			 * \returns	\c false if less than three elements
			 * 			can be stored in queue.
			 * 
			 * Only works with queue with more than three elements. 
			 */
			bool
			isNearlyFull() const;

			modm_always_inline bool
			isEmpty() const;

			modm_always_inline bool
			isNotEmpty() const { return not isEmpty(); }
			
			/**
			 * Check if the queue is nearly empty.
			 * 
			 * \returns	\c true if less than three elements are stored
			 * 			in the queue, \c false otherwise.
			 *
			 * Only works with queue with more than three elements.
			 * TODO: calculations are approximate and may include off-by-one errors.
			 */
			bool
			isNearlyEmpty() const;
			
			modm_always_inline Size
			getMaxSize() const;
			
			const T&
			get() const;
			
			bool
			push(const T& value);
			
			void
			pop();
	
		private:
			Index head;
			Index tail;
			
			T buffer[N+1];
		};
	}
}

#include "queue_impl.hpp"

#endif	// MODM_ATOMIC_QUEUE_HPP
