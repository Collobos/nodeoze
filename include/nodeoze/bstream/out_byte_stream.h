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
 * File:   out_buffer.h
 * Author: David Curtis
 *
 * Created on January 7, 2018, 11:22 AM
 */

#ifndef BSTREAM_OUT_BYTE_STREAM_H
#define BSTREAM_OUT_BYTE_STREAM_H

#include <type_traits>
#include <nodeoze/bstream/utils/dump.h>
#include <nodeoze/nbuffer.h>
#include <sstream>
#include <boost/endian/conversion.hpp>
#include <assert.h>

namespace nodeoze
{
namespace bstream
{
	class in_byte_stream;

	class out_byte_stream
	{
	public:

		void debug_state() const
		{
			m_buf.debug_state();
			std::cout << "out_byte_stream - m_data: " << (void*) m_data << ", m_capacity: " << m_capacity << ", m_pos: " << m_pos << std::endl;
			std::cout.flush();
		}

		friend class in_byte_stream;

		inline out_byte_stream() 
		: 
		m_buf{}, 
		m_data{ nullptr }, 
		m_capacity{ 0ul }, 
		m_pos{ 0ul } 
		{}

		out_byte_stream( out_byte_stream const& rhs )
		: 
		m_buf{ rhs.m_buf }, 
		m_data{ m_buf.mutable_data() }, 
		m_capacity{ m_buf.size() }, 
		m_pos{ rhs.position() }
		{}

		out_byte_stream( out_byte_stream&& rhs )
		: 
		m_buf{ std::move( rhs.m_buf ) }, 
		m_data{ m_buf.mutable_data() }, 
		m_capacity{ m_buf.size() }, 
		m_pos{ rhs.position() }
		{
			rhs.m_data = nullptr;
			rhs.m_capacity = 0;
			rhs.m_pos = 0;
		}

		out_byte_stream( std::size_t capacity, buffer::policy pol = buffer::policy::copy_on_write )
		: 
		m_buf{ capacity, pol }, 
		m_data{ m_buf.mutable_data() }, 
		m_capacity{ m_buf.size() }, 
		m_pos{ 0ul }
		{}

		out_byte_stream( buffer const& buf )
		: 
		m_buf{ buf }, 
		m_data{ m_buf.mutable_data() }, 
		m_capacity{ m_buf.size() }, 
		m_pos{ 0ul }
		{}

		out_byte_stream( buffer&& buf )
		: 
		m_buf{ std::move( buf ) }, 
		m_data{ m_buf.mutable_data() }, 
		m_capacity{ m_buf.size() }, 
		m_pos{ 0ul }
		{}

		virtual ~out_byte_stream() {}

		inline void
		use( buffer&& buf )
		{
			m_buf = std::move( buf );
			m_data = m_buf.mutable_data();
			m_capacity = m_buf.size();
			m_pos = 0ul;
		}

		inline buffer const&
		get_buffer() const
		{
			return m_buf;
		}

		virtual out_byte_stream& 
		clear() noexcept
		{
			m_pos = 0ul;
			return *this;
		}

		inline std::size_t 
		remaining() const noexcept
		{
			std::size_t result = 0;
			if ( m_capacity > 0 )
			{
				assert( m_pos <= m_capacity );
				result = m_capacity - m_pos;
			}
			return result;
		}

		inline bool 
		has_remaining( std::size_t n = 1 ) const noexcept
		{
			return remaining() >= n ;
		}

		inline out_byte_stream& 
		put( std::uint8_t byte )
		{
			accommodate_put( 1 );
			m_data[ m_pos++ ] = byte;
			return *this;                
		}

		inline out_byte_stream& 
		put( const out_byte_stream& source )
		{
			std::size_t nbytes = source.size();
			return put( source.m_data, nbytes );
		}

		inline out_byte_stream& 
		put( const void* src, std::size_t nbytes )
		{
			accommodate_put( nbytes );
			void* dst = m_data + m_pos;
			std::memcpy( dst, src, nbytes );
			m_pos += nbytes;
			return *this;                
		}

		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof( U ) == 1, out_byte_stream&>::type 
		put_arithmetic( U value )
		{
			accommodate_put( sizeof( U ) );
			m_data[ m_pos ] = reinterpret_cast<std::uint8_t const&>( value );
			++m_pos;
			return *this;
		}
		
		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof( U ) == 2, out_byte_stream&>::type 
		put_arithmetic( U value, boost::endian::order buffer_order = boost::endian::order::big )
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof( U );
			using canonical_type = std::uint16_t;
			accommodate_put( sizeof( U ) );

			union
			{
				std::uint8_t bytes[ usize ];
				canonical_type canonical_value;
			} val;

			val.canonical_value = reinterpret_cast<canonical_type const&>( value );
			if ( reverse_order )
			{
				m_data[ m_pos ] = val.bytes[ 1 ];
				m_data[ m_pos+1 ] = val.bytes[ 0 ];
			}
			else
			{
				m_data[ m_pos ] = val.bytes[ 0 ];
				m_data[ m_pos+1 ] = val.bytes[ 1 ];
			}
			m_pos += usize;
			return *this;
		}
		
		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof( U ) == 4, out_byte_stream&>::type 
		put_arithmetic( U value, boost::endian::order buffer_order = boost::endian::order::big )
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof( U );
			using canonical_type = std::uint32_t;
			accommodate_put( sizeof( U ) );

			union
			{
				std::uint8_t bytes[ usize ];
				canonical_type canonical_value;
			} val;

			val.canonical_value = reinterpret_cast<canonical_type const&>( value );
			if ( reverse_order )
			{
				m_data[ m_pos ] = val.bytes[ 3 ];
				m_data[ m_pos+1 ] = val.bytes[ 2 ];
				m_data[ m_pos+2 ] = val.bytes[ 1 ];
				m_data[ m_pos+3 ] = val.bytes[ 0 ];
			}
			else
			{
				m_data[ m_pos ] = val.bytes[ 0 ];
				m_data[ m_pos+1 ] = val.bytes[ 1 ];
				m_data[ m_pos+2 ] = val.bytes[ 2 ];
				m_data[ m_pos+3 ] = val.bytes[ 3 ];
			}
			m_pos += usize;
			return *this;
		}
		
		template<class U>
		inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof( U ) == 8, out_byte_stream&>::type 
		put_arithmetic( U value, boost::endian::order buffer_order = boost::endian::order::big )
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof( U );
			using canonical_type = std::uint64_t;
			accommodate_put( sizeof( U ) );

			union
			{
				std::uint8_t bytes[ usize ];
				canonical_type canonical_value;
			} val;

			val.canonical_value = reinterpret_cast<canonical_type const&>( value );
			if ( reverse_order )
			{
				m_data[ m_pos ] = val.bytes[ 7 ];
				m_data[ m_pos+1 ] = val.bytes[ 6 ];
				m_data[ m_pos+2 ] = val.bytes[ 5 ];
				m_data[ m_pos+3 ] = val.bytes[ 4 ];
				m_data[ m_pos+4 ] = val.bytes[ 3 ];
				m_data[ m_pos+5 ] = val.bytes[ 2 ];
				m_data[ m_pos+6 ] = val.bytes[ 1 ];
				m_data[ m_pos+7 ] = val.bytes[ 0 ];
			}
			else
			{
				m_data[ m_pos ] = val.bytes[ 0 ];
				m_data[ m_pos+1 ] = val.bytes[ 1 ];
				m_data[ m_pos+2 ] = val.bytes[ 2 ];
				m_data[ m_pos+3 ] = val.bytes[ 3 ];
				m_data[ m_pos+4 ] = val.bytes[ 4 ];
				m_data[ m_pos+5 ] = val.bytes[ 5 ];
				m_data[ m_pos+6 ] = val.bytes[ 6 ];
				m_data[ m_pos+7 ] = val.bytes[ 7 ];
			}
			m_pos += usize;
			return *this;
		}
		
		// added to make this buffer directly usable by msgpack::packer

		inline void 
		write( const char* buf, size_t len )
		{
			put( buf, len );
		}

		inline std::size_t
		size() const noexcept
		{
			return m_pos;
		}

		inline const std::uint8_t* 
		data() const noexcept
		{
			return m_data;
		}

		inline std::size_t
		capacity() const noexcept
		{
			return m_capacity;
		}

		inline std::size_t
		position() const noexcept
		{
			return m_pos;
		}

		inline void
		position( std::size_t pos )
		{
			if ( pos < m_capacity )
			{
				m_pos = pos;
			}
			else
			{
				throw std::invalid_argument( "position out of range for buffer" );
			}
		}

		inline void dump( std::ostream& os, std::size_t offset, std::size_t nbytes ) const
		{
			utils::dump( os, &( m_data[ offset ] ), nbytes );
		}
		
		inline std::string strdump( std::size_t offset, std::size_t nbytes ) const
		{
			std::ostringstream oss;
			dump( oss, offset, nbytes );
			return oss.str();
		}

		inline void dump( std::ostream& os ) const
		{
			dump( os, 0, m_pos );
		}
		
		inline std::string strdump() const
		{
			std::ostringstream oss;
			dump( oss );
			return oss.str();
		}

	protected:

		inline void
		accommodate_put( std::size_t nbytes )
		{
			if ( ! has_remaining( nbytes ) )
			{
				increase_capacity( m_pos + nbytes );
				m_data = m_buf.mutable_data();
				m_capacity = m_buf.size();
			}
		}

		inline void
		increase_capacity( std::size_t required_capacity )
		{
			static constexpr float cushion_factor = 1.5f;
			m_buf.size( static_cast< std::size_t >( required_capacity * cushion_factor ) );
		}

		nodeoze::buffer		m_buf;
		std::uint8_t*		m_data;
		std::size_t 		m_capacity;
		std::size_t 		m_pos;
	};

} // namespace bstream
} // namespace nodeoze

#endif /* BSTREAM_OUT_BYTE_STREAM_H */

