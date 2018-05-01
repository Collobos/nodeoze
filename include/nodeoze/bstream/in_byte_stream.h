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

#ifndef BSTREAM_IN_BYTE_STREAM_H
#define BSTREAM_IN_BYTE_STREAM_H

#include <nodeoze/bstream/out_byte_stream.h>
#include <nodeoze/bstream/utils/dump.h>
#include <sstream>
#include <boost/endian/conversion.hpp>

namespace nodeoze
{
namespace bstream
{
    class in_byte_stream
    {
    public:

		void debug_state() const
		{
			m_buf.debug_state();
			std::cout << "in_byte_stream - m_data: " << (void*) m_data << ", m_size: " << m_size << ", m_pos: " << m_pos << std::endl;
			std::cout.flush();
		}

        using max_unsigned = std::uint64_t;
        using max_signed = std::int64_t;

        inline in_byte_stream()
        : m_buf{}, m_data{ nullptr }, m_size{ 0 }, m_pos{ 0ul }
		{}

		inline in_byte_stream( in_byte_stream const& rhs )
		: m_buf{ rhs.m_buf }, m_data{ m_buf.const_data() }, m_size{ m_buf.size() }, m_pos{ rhs.m_pos }
		{}
	
		in_byte_stream( nodeoze::buffer const& buf )
		: m_buf{ buf }, m_data{ m_buf.const_data() }, m_size{ m_buf.size() }, m_pos{ 0ul }
		{}	

		in_byte_stream( const void* data, std::size_t size )
		: m_buf{ data, size }, m_data{ m_buf.const_data() }, m_size{ m_buf.size() }, m_pos{ 0ul }
		{}

		virtual ~in_byte_stream() {}
		
		in_byte_stream( out_byte_stream const& os )
		: m_buf{ os.m_buf }, m_data{ m_buf.const_data() }, m_size{ os.size() }, m_pos{ 0ul }
		{}
		
/*
		in_byte_stream( out_byte_stream const& obuf )
		: m_buf{ std::make_unique< buffer_view_memory >( obuf.view() ) }, m_data{ m_buf.const_data() }, m_size{ m_buf.size() }, m_pos{ 0ul }
		{}

		inline void
		capture( nodeoze::buffer&& buf )
		{
			
			m_buf = std::make_unique< buffer_memory >( std::move( buf ) );
			m_data = m_buf.const_data();
			m_size = m_buf.size();
			m_pos = 0ul;
		}

		inline void 
		capture( memory_ptr&& memptr )
		{
			m_buf = std::move( memptr );
			m_data = m_buf.const_data();
			m_size = m_buf.size();
			m_pos = 0ul;
		}
*/

		inline void
		use( buffer const& buf )
		{
			m_buf = buf;
			m_data = m_buf.const_data();
			m_size = m_buf.size();
			m_pos = 0ul;
		}

		inline void 
		use( out_byte_stream const& os )
		{
			m_buf = os.m_buf;
			m_data = m_buf.const_data();
			m_size = os.size();
			m_pos = 0ul;
		}

		inline buffer const& 
		get_buffer() const
		{
			return m_buf;
		}	

        inline std::size_t 
		remaining() const noexcept
        {
            return m_size - m_pos;
        }

        inline bool has_remaining( std::size_t nbytes = 1 ) const noexcept
        {
            return remaining() >= nbytes;
        }
        
        inline std::size_t position() const noexcept
        {
            return m_pos;
        }
        
        inline void position( std::size_t pos )
        {
            if ( pos > m_size )
            {
				throw std::invalid_argument( "position out of range for buffer" );
            }
            m_pos = pos;
        }
        
        inline size_t size() const noexcept
        {
            return m_size;
        }

        inline const std::uint8_t* data() const noexcept
        {
            return m_data;
        }
        
        virtual void rewind() noexcept
        {
            m_pos = 0;
        }
		
        inline std::uint8_t 
        get()
        {
            if ( !has_remaining( 1 ) )
            {
                throw std::out_of_range( "get() past end of buffer" );
            }
            return m_data[ m_pos++ ];
        }

        inline std::uint8_t 
        peek()
        {
            if ( !has_remaining( 1 ) )
            {
                throw std::out_of_range( "peek() past end of buffer" );
            }
            return m_data[ m_pos ];
        }
		
		union bytes_2
		{
			std::uint8_t bytes[ 2 ];
			std::uint16_t u16;
		};
		
		union bytes_4
		{
			std::uint8_t bytes[ 4 ];
			std::uint32_t u32;
		};
		
		union bytes_8
		{
			std::uint8_t bytes[ 8 ];
			std::uint64_t u64;
		};
		
		template< class U >
		inline typename std::enable_if< std::is_arithmetic< U >::value && sizeof( U ) == 1, U >::type 
		get_arithmetic_as()
		{
			constexpr std::size_t usize = sizeof( U );
			
			if ( !has_remaining( usize ) )
			{
				throw std::out_of_range( "get_arithmetic_as() past end of buffer" );
			}
			
			return static_cast< U >( m_data[ m_pos++ ] );
		}
		
		template< class U >
		inline typename std::enable_if< std::is_arithmetic< U >::value && sizeof( U ) == 2, U >::type 
		get_arithmetic_as( boost::endian::order buffer_order = boost::endian::order::big )
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof( U );
			
			if ( !has_remaining( usize ) )
			{
				throw std::out_of_range( "get_arithmetic_as() past end of buffer" );
			}
			
			bytes_2 b;
			if ( reverse_order )
			{
				b.bytes[ 1 ] = m_data[ m_pos ];
				b.bytes[ 0 ] = m_data[ m_pos+1 ];
			}
			else
			{
				b.bytes[ 0 ] = m_data[ m_pos ];
				b.bytes[ 1 ] = m_data[ m_pos+2 ];
			}
			m_pos+= usize;
			return reinterpret_cast< const U& >( b.u16 );
		}
		
		template< class U >
		inline typename std::enable_if< std::is_arithmetic< U >::value && sizeof( U ) == 4, U >::type 
		get_arithmetic_as( boost::endian::order buffer_order = boost::endian::order::big )
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof( U );
			
			if ( !has_remaining( usize ) )
			{
				throw std::out_of_range( "get_arithmetic_as() past end of buffer" );
			}
			
			bytes_4 b;
			if ( reverse_order )
			{
				b.bytes[ 3 ] = m_data[ m_pos ];
				b.bytes[ 2 ] = m_data[ m_pos+1 ];
				b.bytes[ 1 ] = m_data[ m_pos+2 ];
				b.bytes[ 0 ] = m_data[ m_pos+3 ];
			}
			else
			{
				b.bytes[ 0 ] = m_data[ m_pos ];
				b.bytes[ 1 ] = m_data[ m_pos+1 ];
				b.bytes[ 2 ] = m_data[ m_pos+2 ];
				b.bytes[ 3 ] = m_data[ m_pos+3 ];
			}
			m_pos+= usize;
			return reinterpret_cast< const U& >( b.u32 );
		}
		
		template< class U >
		inline typename std::enable_if< std::is_arithmetic< U >::value && sizeof( U ) == 8, U >::type 
		get_arithmetic_as( boost::endian::order buffer_order = boost::endian::order::big )
		{
			static const bool reverse_order = boost::endian::order::native != buffer_order;
			constexpr std::size_t usize = sizeof( U );
			
			if ( !has_remaining( usize ) )
			{
				throw std::out_of_range( "get_arithmetic_as() past end of buffer" );
			}
			
			bytes_8 b;
			if ( reverse_order )
			{
				b.bytes[ 7 ] = m_data[ m_pos ];
				b.bytes[ 6 ] = m_data[ m_pos+1 ];
				b.bytes[ 5 ] = m_data[ m_pos+2 ];
				b.bytes[ 4 ] = m_data[ m_pos+3 ];
				b.bytes[ 3 ] = m_data[ m_pos+4 ];
				b.bytes[ 2 ] = m_data[ m_pos+5 ];
				b.bytes[ 1 ] = m_data[ m_pos+6 ];
				b.bytes[ 0 ] = m_data[ m_pos+7 ];
			}
			else
			{
				b.bytes[ 0 ] = m_data[ m_pos ];
				b.bytes[ 1 ] = m_data[ m_pos+1 ];
				b.bytes[ 2 ] = m_data[ m_pos+2 ];
				b.bytes[ 3 ] = m_data[ m_pos+3 ];
				b.bytes[ 4 ] = m_data[ m_pos+4 ];
				b.bytes[ 5 ] = m_data[ m_pos+5 ];
				b.bytes[ 6 ] = m_data[ m_pos+6 ];
				b.bytes[ 7 ] = m_data[ m_pos+7 ];
			}
			m_pos+= usize;
			return reinterpret_cast< const U& >( b.u64 );
		}
		
/*
        template< class U >
        inline U get_arithmetic_as()
        {
            static_assert( std::is_scalar< U >::value, "get_arithmetic_as template parameter must be an arithmetic type" );

            constexpr std::size_t usize = sizeof( U );

            if ( !has_remaining( usize ) )
            {
                throw std::out_of_range( "get_arithmetic_as() past end of buffer" );
            }

            switch( usize )
            {
                case 1:
                {
                    return reinterpret_cast< const U& >( m_data[ m_pos++ ] );
                }
                case 2:
                {
                    std::uint16_t value = 
                            ( static_cast< std::uint16_t >( m_data[ m_pos ] ) << 8 ) 
                            | ( static_cast< std::uint16_t >( m_data[ m_pos + 1 ] ) );
                    m_pos += usize;
                    return reinterpret_cast< U& >( value );
                }
                case 4:
                {
                    std::uint32_t value = 
                            ( static_cast< std::uint32_t >( m_data[ m_pos ] ) << 24 ) 
                            | ( static_cast< std::uint32_t >( m_data[ m_pos + 1 ] ) << 16 ) 
                            | ( static_cast< std::uint32_t >( m_data[ m_pos + 2 ] ) << 8 ) 
                            | ( static_cast< std::uint32_t >( m_data[ m_pos + 3 ] ) );
                    m_pos += usize;
                    return reinterpret_cast< U& >( value );
                }
                case 8:
                {
                    std::uint64_t value = 
                            ( static_cast< std::uint64_t >( m_data[ m_pos ] ) << 56 ) 
                            | ( static_cast< std::uint64_t >( m_data[ m_pos + 1 ] ) << 48 ) 
                            | ( static_cast< std::uint64_t >( m_data[ m_pos + 2 ] ) << 40 ) 
                            | ( static_cast< std::uint64_t >( m_data[ m_pos + 3 ] ) << 32 )
                            | ( static_cast< std::uint64_t >( m_data[ m_pos + 4 ] ) << 24 ) 
                            | ( static_cast< std::uint64_t >( m_data[ m_pos + 5 ] ) << 16 ) 
                            | ( static_cast< std::uint64_t >( m_data[ m_pos + 6 ] ) << 8 ) 
                            | ( static_cast< std::uint64_t >( m_data[ m_pos + 7 ] ) );
                    m_pos += usize;
                    return reinterpret_cast< U& >( value );
                }
                default:
                {
					std::string msg( "unexpected size for arithmetic type: " );
					msg.append( std::to_string( usize ) );
					throw std::logic_error( msg );
                }
            }
        }
*/
        inline const std::uint8_t* 
        get_bytes( std::size_t nbytes )
        {
            if ( !has_remaining( nbytes ) )
            {
                throw std::out_of_range( "get_bytes() past end of buffer" );
            }
            auto p = &( m_data[ m_pos ] );
            m_pos += nbytes;
            return p;
        }


        inline void dump( std::ostream& os, std::size_t offset, std::size_t nbytes ) const
        {
            utils::dump( os, &( m_data[ offset ] ), nbytes );
        }

        inline void dump( std::ostream& os ) const
        {
            dump( os, 0, m_size );
        }
		
		inline std::string strdump( std::size_t offset, std::size_t nbytes ) const
		{
			std::ostringstream oss;
			dump( oss, offset, nbytes );
			return oss.str(); 
		}
		
		inline std::string strdump() const
		{
			return strdump( 0, m_size );
		}
		
	protected:
        nodeoze::buffer			m_buf;
		const std::uint8_t*		m_data;
		std::size_t				m_size;
        std::size_t 			m_pos = 0UL;
    };

} // namespace bstream
} // namespace nodeoze

#endif /* BSTREAM_IN_BYTE_STREAM_H */

