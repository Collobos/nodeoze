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
 * File:   anon_block.h
 * Author: David Curtis
 *
 * Created on January 7, 2018, 10:36 AM
 */

#ifndef UTILS_ANON_BLOCK_H
#define UTILS_ANON_BLOCK_H

#include <nodeoze/bstream/utils/io_buffers/memory_block.h>

namespace nodeoze
{
namespace bstream
{
namespace utils
{
	/*!	\class const_anon_block
	 *	\brief Unmanaged block of read-only memory
	 * 
	 *	The memory is not managed by const_anon_block; it is the responsibility of
	 *	the caller to ensure that the pointer provided to the constructor
	 *	remains valid for the extent of constructed instance.
	 */
	class const_anon_block : public const_memory_block
	{
	public:
		
		/*!	\brief Constructs an instance of const_anon_block
		 *	\param data pointer to the beginning of contiguous memory block
		 *	\param capacity	size of the memory block in bytes
		 */
		const_anon_block(const void* data, std::size_t size)
		: m_data{reinterpret_cast<const std::uint8_t*>(data)}, m_size{size} 
		{}
		
		virtual const std::uint8_t* cdata() const noexcept override
		{
			return m_data;
		}
		
		virtual std::size_t size() const noexcept override
		{
			return m_size;
		}
				
	private:
		const std::uint8_t *m_data;
		std::size_t m_size;
	};
	
} // namespace utils
} // namespace bstream
} // namespace nodeoze

#endif /* UTILS_ANON_BLOCK_H */

