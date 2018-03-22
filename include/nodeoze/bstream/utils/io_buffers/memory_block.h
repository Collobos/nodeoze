/*
 * The MIT License
 *
 * Copyright 2018 David Curtis.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* 
 * File:   memory_block.h
 * Author: David Curtis
 *
 * Created on January 7, 2018, 10:30 AM
 */

#ifndef UTILS_MEMORY_BLOCK_H
#define UTILS_MEMORY_BLOCK_H

#include <memory>
#include <cstdint>
#include <iostream>
#include <assert.h>

namespace nodeoze
{
namespace bstream
{
namespace utils
{
	class memory_block;
	
	/*!	\class const_memory_block
	 *	\brief Abstract base class for a block of read-only memory 
	 * 
	 *	\c \b The base interface for a block of read-only memory. A block is viewed 
	 *	as an array of \c std::uint8_t elements. 
	 * 
	 *	\author David Curtis
	 */
	class const_memory_block
	{
	public:
		using ptr = std::unique_ptr<const_memory_block>;
		
		virtual ~const_memory_block() {}
		
		/*! \brief Returns a pointer to the first element in the memory block.
		 *	\return pointer to the first element (of type std::uint8_t) in the block
		 */
		virtual const std::uint8_t *cdata() const noexcept = 0;
		
		/*! \brief Returns the size of the memory block.
		 *	\return the size of the block, , as the number of \c std::uint8_t elements 
		 */
		virtual std::size_t size() const noexcept = 0;
		
		/*! \brief Returns a unique pointer to a mutable memory block with the same contents as this block.
		 * 
		 *	In effect, this method is a dynamic downcast to a mutable block type.
		 *	If the derived implementation supports this capability, the returned block will have
		 *	the same contents and size as this block. If the 
		 *	capability is not supported, the return value is \c nullptr.
		 *	The actual type of memory block is determined by the implementation.
		 * 
		 *	The calling context should assume that this member function (if supports) effects
		 *	a move to the returned block. After invocation, this instance will be empty&mdash;the
		 *	size() returns zero, and data() returns nullptr.
		 * 
		 *	\return a unique pointer to a mutable memory block, or null if not supported.
		 */
		virtual std::unique_ptr<memory_block> make_mutable()
		{
			return nullptr;
		}
	};
	
	/*!	\class memory_block
	 *	\brief Abstract base class for a block of mutable memory 
	 * 
	 *	\c \b The base interface for a block of mutable memory. A block is viewed 
	 *	as an array of \c std::uint8_t elements. 
	 * 
	 *	\author David Curtis
	 */
	class memory_block : public const_memory_block
	{
	public:
		
		using ptr = std::unique_ptr<memory_block>;
		
		virtual ~memory_block() {}
		
		virtual std::uint8_t *data() noexcept = 0;

		virtual void set_size(std::size_t size) = 0;

		virtual std::size_t capacity() const noexcept = 0;
		
        virtual bool is_expandable() const noexcept
		{
			return false;
		}
		
		virtual void expand(std::size_t)
		{
			assert(false);
		}
		
	};

	class expandable_block : public memory_block
	{
	public:
        virtual bool is_expandable() const noexcept override
		{
			return true;
		}
		
		virtual void expand(std::size_t required_capacity) override
		{
			static constexpr float cushion_factor = 1.5;			
			resize(static_cast<std::size_t>(required_capacity * cushion_factor));
		}
		
	protected:
		
		virtual void resize(std::size_t new_size) = 0;
	};
	
} // namespace utils
} // namespace bstream
} // namespace nodeoze

#endif /* UTILS_MEMORY_BLOCK_H */

