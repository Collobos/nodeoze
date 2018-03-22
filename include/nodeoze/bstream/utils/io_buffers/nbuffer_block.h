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
 * File:   nbuffer_block.h
 * Author: David Curtis
 *
 * Created on January 7, 2018, 11:00 AM
 */

#ifndef UTILS_NBUFFER_BLOCK_H
#define UTILS_NBUFFER_BLOCK_H

#include <nodeoze/nbuffer.h>
#include <nodeoze/bstream/utils/io_buffers/memory_block.h>

namespace nodeoze
{
namespace bstream
{
namespace utils
{
	
	class nbuffer_block : public expandable_block
	{
	public:
		inline nbuffer_block(std::size_t capacity)
		: m_nbuf(capacity)
		{}
		
		inline nbuffer_block(std::uint8_t* data, std::size_t size)
		: m_nbuf(data, size)
		{}
		
		inline nbuffer_block(nodeoze::buffer&& buf)
		: m_nbuf{std::move(buf)}
		{}

		
		virtual std::uint8_t *data() noexcept override
		{
			return m_nbuf.data();
		}
		
		virtual const std::uint8_t *cdata() const noexcept override
		{
			return m_nbuf.data();
		}
		
		virtual std::size_t capacity() const noexcept override
		{
			return m_nbuf.capacity();
		}

		virtual std::size_t size() const noexcept override
		{
			return m_nbuf.size();
		}

		virtual void set_size(std::size_t size) override
		{
			m_nbuf.size(size);
		}
		
		virtual memory_block::ptr make_mutable() override
		{
			memory_block::ptr p = std::unique_ptr<nbuffer_block>(new nbuffer_block(std::move(m_nbuf)));
//			memory_block::ptr p = std::unique_ptr<nbuffer_block>(this); // ?? would this work?
			return p;
		}
		
	private:

		virtual void resize(std::size_t new_size) override
		{
			m_nbuf.capacity(new_size);
		}

		nodeoze::buffer m_nbuf;
	};

} // namespace utils
} // bstream
} // nodeoze

#endif /* UTILS_NBUFFER_BLOCK_H */

