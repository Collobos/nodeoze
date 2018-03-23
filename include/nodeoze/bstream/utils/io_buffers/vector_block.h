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
 * File:   vector_block.h
 * Author: David Curtis
 *
 * Created on January 7, 2018, 11:00 AM
 */

#ifndef UTILS_VECTOR_BLOCK_H
#define UTILS_VECTOR_BLOCK_H

#include <vector>
#include <nodeoze/bstream/utils/io_buffers/memory_block.h>

namespace nodeoze
{
namespace bstream
{
namespace utils
{
	
	class vector_block : public expandable_block
	{
	public:
		inline vector_block(std::size_t capacity)
		: m_vec(capacity, static_cast<std::uint8_t>(0)), m_pos{0ul}
		{}
		
		inline vector_block(std::uint8_t* data, std::size_t size)
		: m_vec{}, m_pos{size}
		{
			m_vec.reserve(size);
			for (auto i = 0ul; i < size; ++i)
			{
				m_vec.push_back(data[i]);
			}
		}
		
		inline vector_block(std::vector<std::uint8_t>&& vec)
		: m_vec{std::move(vec)}, m_pos{m_vec.size()}
		{}

		inline vector_block(std::vector<std::uint8_t>&& vec, std::size_t pos)
		: m_vec{std::move(vec)}, m_pos{pos}
		{}
/*
		inline vector_block(std::vector<std::uint8_t> const& vec)
		: m_vec{vec}, m_pos{m_vec.size()}
		{
		}
*/
		virtual std::uint8_t *data() noexcept override
		{
			return m_vec.data();
		}
		
		virtual const std::uint8_t *cdata() const noexcept override
		{
			return m_vec.data();
		}
		
		virtual std::size_t capacity() const noexcept override
		{
			return m_vec.size();
		}

		virtual std::size_t size() const noexcept override
		{
			return m_pos;
		}

		virtual void set_size(std::size_t pos) override
		{
			if (m_pos >= capacity())
			{
				throw std::out_of_range{"position exceeds vector size/capacity"};
			}
			m_pos = pos;
		}
		
		virtual memory_block::ptr make_mutable() override
		{
			memory_block::ptr p = std::unique_ptr<vector_block>(new vector_block(std::move(m_vec)));
			p->set_size(m_pos);
			m_pos = 0ul;
//			memory_block::ptr p = std::unique_ptr<vector_block>(this); // ?? would this work?
			return p;
		}
		
	private:

		virtual void resize(std::size_t new_size) override
		{
			m_vec.resize(new_size, static_cast<std::uint8_t>(0));
		}

		std::vector<std::uint8_t> m_vec;
		std::size_t m_pos;
	};

} // namespace utils
} // namespace bstream
} // namespace nodeoze

#endif /* UTILS_VECTOR_BLOCK_H */

