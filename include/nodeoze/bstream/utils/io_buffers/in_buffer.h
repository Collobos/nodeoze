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
 * File:   in_buffer.h
 * Author: David Curtis
 *
 * Created on January 7, 2018, 11:30 AM
 */

#ifndef UTILS_IN_BUFFER_H
#define UTILS_IN_BUFFER_H

#include <nodeoze/bstream/utils/io_buffers/out_buffer.h>
#include <nodeoze/bstream/utils/io_buffers/dump.h>
#include <sstream>
#include <boost/endian/conversion.hpp>

namespace nodeoze
{
namespace bstream
{
namespace utils
{
	
    class in_buffer
    {
    public:
        
        using max_unsigned = std::uint64_t;
        using max_signed = std::int64_t;

        inline in_buffer()
        : block_{nullptr}, data_{nullptr}, pos_{0ul}, size_{0UL}
		{}
		
		in_buffer(const_memory_block::ptr&& block, std::size_t size) 
		: block_{std::move(block)}, data_{block_->cdata()}, pos_{0ul}, size_{size}
		{}
		
		in_buffer(const_memory_block::ptr&& block) 
		: block_{std::move(block)}, data_{block_->cdata()}, pos_{0ul}, size_{block_->size()}
		{}

		virtual ~in_buffer() {}

		memory_block::ptr release_mutable()
		{
			size_ = 0ul;
			data_ = nullptr;
			auto p = block_->make_mutable();
			block_ = nullptr;
			return p;
		}

		const_memory_block::ptr release()
		{
 			size_ = 0ul;
			data_ = nullptr;
			auto p = block_->make_mutable();
			block_ = nullptr;
			return p;
		}

		in_buffer(out_buffer&& obuf)
		{
			hijack(std::move(obuf));
		}
		
		void capture(const_memory_block::ptr&& block, std::size_t size)
		{
			block_ = std::move(block);
			data_ = block_->cdata();
			size_ = size;
			pos_ = 0ul;
		}
		
		void capture(const_memory_block::ptr&& block)
		{
			block_ = std::move(block);
			data_ = block_->cdata();
			size_ = block_->size();
			pos_ = 0ul;
		}
		
		void hijack(out_buffer&& obuf)
		{
			capture(obuf.release());
		}
		
		template<class T>
		in_buffer(T* blkp, typename std::enable_if<std::is_base_of<memory_block, T>::value,int>::type = 0)
		: in_buffer{std::unique_ptr<T>(blkp)} {}
		
        inline std::size_t remaining() const noexcept
        {
            return size_ - pos_;
        }

        inline bool has_remaining(std::size_t nbytes = 1) const noexcept
        {
            return remaining() >= nbytes;
        }
        
        inline std::size_t position() const noexcept
        {
            return pos_;
        }
        
        inline void position(std::size_t pos)
        {
            if (pos > size_)
            {
				throw std::invalid_argument("position out of range for buffer");
            }
            pos_ = pos;
        }
        
        inline size_t size() const noexcept
        {
            return size_;
        }

        inline const std::uint8_t* data() const noexcept
        {
            return data_;
        }
        
        virtual void rewind() noexcept
        {
            pos_ = 0;
        }
		
        inline std::uint8_t 
        get()
        {
            if (!has_remaining(1))
            {
                throw std::out_of_range("get() past end of buffer");
            }
            return data_[pos_++];
        }

        inline std::uint8_t 
        peek()
        {
            if (!has_remaining(1))
            {
                throw std::out_of_range("peek() past end of buffer");
            }
            return data_[pos_];
        }
		
		union bytes_2
		{
			std::uint8_t bytes[2];
			std::uint16_t u16;
		};
		
		union bytes_4
		{
			std::uint8_t bytes[4];
			std::uint32_t u32;
		};
		
		union bytes_8
		{
			std::uint8_t bytes[8];
			std::uint64_t u64;
		};
		
		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof(U) == 1, U>::type 
		get_arithmetic_as()
		{
			constexpr std::size_t usize = sizeof(U);
			
			if (!has_remaining(usize))
			{
				throw std::out_of_range("get_arithmetic_as() past end of buffer");
			}
			
			return reinterpret_cast<const U&>(data_[pos_++]);
		}
		
		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof(U) == 2, U>::type 
		get_arithmetic_as(boost::endian::order buffer_order = boost::endian::order::big)
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof(U);
			
			if (!has_remaining(usize))
			{
				throw std::out_of_range("get_arithmetic_as() past end of buffer");
			}
			
			bytes_2 b;
			if (reverse_order)
			{
				b.bytes[1] = data_[pos_];
				b.bytes[0] = data_[pos_+1];
			}
			else
			{
				b.bytes[0] = data_[pos_];
				b.bytes[1] = data_[pos_+2];
			}
			pos_+= usize;
			return reinterpret_cast<const U&>(b.u16);
		}
		
		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof(U) == 4, U>::type 
		get_arithmetic_as(boost::endian::order buffer_order = boost::endian::order::big)
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof(U);
			
			if (!has_remaining(usize))
			{
				throw std::out_of_range("get_arithmetic_as() past end of buffer");
			}
			
			bytes_4 b;
			if (reverse_order)
			{
				b.bytes[3] = data_[pos_];
				b.bytes[2] = data_[pos_+1];
				b.bytes[1] = data_[pos_+2];
				b.bytes[0] = data_[pos_+3];
			}
			else
			{
				b.bytes[0] = data_[pos_];
				b.bytes[1] = data_[pos_+1];
				b.bytes[2] = data_[pos_+2];
				b.bytes[3] = data_[pos_+3];
			}
			pos_+= usize;
			return reinterpret_cast<const U&>(b.u32);
		}
		
		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof(U) == 8, U>::type 
		get_arithmetic_as(boost::endian::order buffer_order = boost::endian::order::big)
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof(U);
			
			if (!has_remaining(usize))
			{
				throw std::out_of_range("get_arithmetic_as() past end of buffer");
			}
			
			bytes_8 b;
			if (reverse_order)
			{
				b.bytes[7] = data_[pos_];
				b.bytes[6] = data_[pos_+1];
				b.bytes[5] = data_[pos_+2];
				b.bytes[4] = data_[pos_+3];
				b.bytes[3] = data_[pos_+4];
				b.bytes[2] = data_[pos_+5];
				b.bytes[1] = data_[pos_+6];
				b.bytes[0] = data_[pos_+7];
			}
			else
			{
				b.bytes[0] = data_[pos_];
				b.bytes[1] = data_[pos_+1];
				b.bytes[2] = data_[pos_+2];
				b.bytes[3] = data_[pos_+3];
				b.bytes[4] = data_[pos_+4];
				b.bytes[5] = data_[pos_+5];
				b.bytes[6] = data_[pos_+6];
				b.bytes[7] = data_[pos_+7];
			}
			pos_+= usize;
			return reinterpret_cast<const U&>(b.u64);
		}
		
/*
        template<class U>
        inline U get_arithmetic_as()
        {
            static_assert(std::is_scalar<U>::value, "get_arithmetic_as template parameter must be an arithmetic type");

            constexpr std::size_t usize = sizeof(U);

            if (!has_remaining(usize))
            {
                throw std::out_of_range("get_arithmetic_as() past end of buffer");
            }

            switch(usize)
            {
                case 1:
                {
                    return reinterpret_cast<const U&>(data_[pos_++]);
                }
                case 2:
                {
                    std::uint16_t value = 
                            (static_cast<std::uint16_t>(data_[pos_]) << 8) 
                            | (static_cast<std::uint16_t>(data_[pos_ + 1]));
                    pos_ += usize;
                    return reinterpret_cast<U&>(value);
                }
                case 4:
                {
                    std::uint32_t value = 
                            (static_cast<std::uint32_t>(data_[pos_]) << 24) 
                            | (static_cast<std::uint32_t>(data_[pos_ + 1]) << 16) 
                            | (static_cast<std::uint32_t>(data_[pos_ + 2]) << 8) 
                            | (static_cast<std::uint32_t>(data_[pos_ + 3]));
                    pos_ += usize;
                    return reinterpret_cast<U&>(value);
                }
                case 8:
                {
                    std::uint64_t value = 
                            (static_cast<std::uint64_t>(data_[pos_]) << 56) 
                            | (static_cast<std::uint64_t>(data_[pos_ + 1]) << 48) 
                            | (static_cast<std::uint64_t>(data_[pos_ + 2]) << 40) 
                            | (static_cast<std::uint64_t>(data_[pos_ + 3]) << 32)
                            | (static_cast<std::uint64_t>(data_[pos_ + 4]) << 24) 
                            | (static_cast<std::uint64_t>(data_[pos_ + 5]) << 16) 
                            | (static_cast<std::uint64_t>(data_[pos_ + 6]) << 8) 
                            | (static_cast<std::uint64_t>(data_[pos_ + 7]));
                    pos_ += usize;
                    return reinterpret_cast<U&>(value);
                }
                default:
                {
					std::string msg("unexpected size for arithmetic type: ");
					msg.append(std::to_string(usize));
					throw std::logic_error(msg);
                }
            }
        }
*/
        inline const std::uint8_t* 
        get_bytes(std::size_t nbytes)
        {
            if (!has_remaining(nbytes))
            {
                throw std::out_of_range("get_bytes() past end of buffer");
            }
            auto p = &(data_[pos_]);
            pos_ += nbytes;
            return p;
        }


        inline void dump( std::ostream& os, std::size_t offset, std::size_t nbytes) const
        {
            utils::dump(os, &(data_[offset]), nbytes);
        }

        inline void dump(std::ostream& os) const
        {
            dump(os, 0, size_);
        }
		
		inline std::string strdump(std::size_t offset, std::size_t nbytes) const
		{
			std::ostringstream oss;
			dump(oss, offset, nbytes);
			return oss.str(); 
		}
		
		inline std::string strdump() const
		{
			return strdump(0, size_);
		}
		
	protected:
        const_memory_block::ptr block_;
        const std::uint8_t *data_;
        std::size_t pos_ = 0UL;
		std::size_t size_ = 0UL;
    };

} // namespace utils
} // namespace bstream
} // namespace nodeoze

#endif /* UTILS_IN_BUFFER_H */

