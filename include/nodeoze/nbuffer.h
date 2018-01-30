/*
 * Copyright (c) 2013-2017, Collobos Software Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* Inspired by
 *
 * Boris Kolpackov - http://www.codesynthesis.com
 *
 */

#ifndef _nodeoze_buffer_h
#define _nodeoze_buffer_h

#include <nodeoze/nlog.h>
#include <nodeoze/nmarkers.h>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <string>
#include <iomanip>

namespace nodeoze {

class buffer
{
public:

	typedef std::uint8_t				elemtype;
	typedef std::size_t					sizetype;
	typedef std::shared_ptr< buffer >	ptr;
	
	static const sizetype npos = static_cast< sizetype >( -1 );
	
	typedef std::function< void ( elemtype *data ) > deleter_f;
	
	static deleter_f do_not_delete_data;

	inline
	explicit buffer( sizetype capacity = 0 )
	:
		m_size( 0 ),
		m_capacity( capacity )
	{
		m_data = ( m_capacity > 0 ? new elemtype[ m_capacity ] : nullptr );
	}

	inline
	buffer( const char *data )
	:
		buffer( data, data ? strlen( data ) : 0 )
	{
	}

	inline
	buffer( const std::string &data )
	:
		buffer( data.c_str(), data.size() )
	{
	}

	inline
	buffer( const void *data, sizetype size )
	:
		m_size( size ),
		m_capacity( size )
	{
		if ( m_size > 0 )
		{
			m_data = new elemtype[ m_size ];
			std::memcpy( m_data, data, m_size );
		}
		else
		{
			m_data = nullptr;
		}
	}

	inline
	buffer( const void* data, sizetype size, sizetype capacity )
	:
		m_size( 0 ),
		m_capacity( 0 )
	{
		if ( size <= capacity )
		{
			if ( capacity != 0 )
			{
				m_data = new elemtype[ capacity ];

				if ( size != 0 )
				{
					std::memcpy( m_data, data, size );
				}
			}
			else
			{
				m_data = nullptr;
			}

			m_size		= size;
			m_capacity	= capacity;
		}
	}

	inline
	buffer( void* data, sizetype size, sizetype capacity, deleter_f deleter )
    :
		m_data( static_cast< elemtype* >( data) ),
		m_size( size ),
		m_capacity( capacity ),
		m_deleter( deleter )
	{
	}
	
	buffer( const buffer &rhs ) = delete;

	inline
	buffer( buffer &&rhs )
	{
		move( rhs );
	}

	inline ~buffer()
	{
		free();
	}

	buffer&
	operator=( const buffer &rhs ) = delete;

	inline buffer&
	operator=( buffer &&rhs )
	{
		if ( this != &rhs )
		{
			move( rhs );
		}

		return *this;
	}
	
	inline bool
	operator==( const buffer &rhs ) const
	{
		return ( ( m_size == rhs.m_size ) && ( std::memcmp( m_data, rhs.m_data, m_size ) == 0 ) ) ? true : false;
	}
	
	inline bool
	operator!=( const buffer &rhs ) const
	{
		return !( *this == rhs );
	}

	inline void
	swap( buffer &rhs )
	{
		elemtype	*data		= rhs.m_data;
		sizetype	size		= rhs.m_size;
		sizetype	capacity	= rhs.m_capacity;
		deleter_f	deleter		= rhs.m_deleter;

		rhs.m_data		= m_data;
		rhs.m_size		= m_size;
		rhs.m_capacity	= m_capacity;
		rhs.m_deleter	= m_deleter;

		m_data			= data;
		m_size			= size;
		m_capacity		= capacity;
		m_deleter		= deleter;
	}

	inline elemtype*
	detach()
	{
  		auto data = m_data;

		m_data		= 0;
		m_size		= 0;
		m_capacity	= 0;

		return data;
	}

	inline void
	assign( const char *data )
	{
		assign( data, data ? strlen( data ) : 0 );
	}

	inline void
	assign( const std::string &data )
	{
		assign( data.c_str(), data.size() );
	}

	inline void
	assign( const void *data, sizetype size )
	{
		if ( size > m_capacity )
		{
			free();

    		m_data		= new elemtype[ size ];
    		m_capacity	= size;
  		}

		if ( size != 0 )
		{
			std::memcpy( m_data, data, size );
		}

		m_size = size;
	}

	inline void
	assign( void* data, sizetype size, sizetype capacity, deleter_f deleter )
	{
		free();

		m_data		= static_cast< elemtype* >( data );
		m_size		= size;
		m_capacity	= capacity;
		m_deleter	= deleter;
	}
	
	inline void
	push_back( elemtype val )
	{
		append( &val, 1 );
	}
	
	inline void
	assign( const void* data, sizetype pos, sizetype size )
	{
		if ( size > 0 )
		{
			sizetype new_size = pos + size;
                                                              
			if ( m_capacity < new_size )
			{
				capacity( new_size );
			}
                                                              
			std::memcpy( m_data + pos, data, size );
                                                              
			if ( m_size < new_size )
			{
				m_size = new_size;
			}
		}
	}

	inline void
	append( const buffer& rhs )
	{
		append( rhs.data(), rhs.size() );
	}

	inline void
	append( const void* data, sizetype size )
	{
		if ( size > 0 )
		{
			sizetype ns = m_size + size;

			if ( m_capacity < ns )
			{
				auto next_capacity = (sizetype)(m_capacity * 1.5);
				if ( next_capacity >= ns )
				{
					capacity(next_capacity);
				}
				else
				{
					capacity( ns );
				}
			}

			std::memcpy( m_data + m_size, data, size );
			m_size = ns;
		}
	}
	
	
	// added to make this buffer directly usable by msgpack::packer
    void write(const char* buf, size_t len)
	{
		append( buf, len );
	}
	
	
	inline buffer
	copy() const
	{
		return buffer( m_data, m_size, m_capacity );
	}

	inline void
	fill( elemtype value = 0 )
	{
		if ( m_size > 0 )
		{
			std::memset( m_data, value, m_size );
		}
	}
	
	inline sizetype
	space() const
	{
		return ( m_capacity - m_size );
	}

	inline sizetype
	size() const
	{
		return m_size;
	}

	inline bool
	size( sizetype size )
	{
		bool ok = false;

		if ( m_capacity < size )
		{
			ok = capacity( size );
		}

		m_size = size;
		return ok;
	}

	inline sizetype
	capacity() const
	{
		return m_capacity;
	}

	inline bool
	capacity( sizetype cap )
	{
		if ( m_capacity < cap )
		{
			elemtype *data = new elemtype[ cap ];
			std::memset( data, 0, cap );

			if ( m_size > 0 )
			{
				std::memcpy( data, m_data, m_size );
			}

			free();

			m_data		= data;
			m_capacity	= cap;
		}
	
		return true;
	}

	inline bool
	empty() const
	{
		return ( m_size == 0 );
	}

	inline void
	clear()
	{
		m_size = 0;
	}

	inline elemtype*
	data()
	{
		return m_data;
	}

	inline const elemtype*
	data() const
	{
		return m_data;
	}

	inline elemtype&
	operator[]( sizetype index )
	{
		return m_data[ index ];
	}

	inline elemtype
	operator[]( sizetype index ) const
	{
		return m_data[ index ];
	}

	inline elemtype&
	at( sizetype i )
	{
		if (i >= m_size)
		{
			throw std::out_of_range ("index out of range");
		}
		else
		{
			return m_data[i];
		}
	}

	inline elemtype
	at( sizetype index ) const
	{
		if ( index >= m_size )
		{
			throw std::out_of_range ("index out of range");
		}
		else
		{
			return m_data[ index ];
		}
	}
	
	inline std::error_code
	rotate( sizetype to, sizetype from, sizetype end )
	{
		auto err = std::error_code();

		if ( ( to < m_size ) && ( from < m_size ) && ( end <= m_size ) )
		{
			std::memmove( m_data + to, m_data + from, end - from );
		}
		else
		{
			err = make_error_code( std::errc::invalid_argument );
		}

		return err;
	}

	inline sizetype
	find( elemtype c, sizetype pos = 0 ) const
	{
		if ( m_size == 0 || pos >= m_size )
		{
			return npos;
		}

		elemtype* p (static_cast<elemtype*>( std::memchr( m_data + pos, c, m_size - pos ) ) );
		return p != 0 ? static_cast<sizetype>( p - m_data ) : npos;
	}
	
	inline sizetype
	find( const char *s, sizetype len, sizetype pos = 0 ) const
	{
		sizetype ret = npos;
		
		if ( ( pos + len ) < m_size )
		{
			auto p		= std::search( m_data + pos, m_data + m_size - pos, s, s + len );
			auto index	= static_cast< sizetype >( p - m_data );
			
			if ( index < m_size )
			{
				ret = index;
			}
		}
	
		return ret;
	}

	inline sizetype
	rfind( elemtype c, sizetype pos = npos ) const
	{
		if ( m_size != 0 )
		{
			sizetype n = m_size;

			if ( --n > pos )
			{
				n = pos;
			}

			for ( ++n; n-- != 0; )
			{
				if ( m_data[ n ] == c )
				{
					return n;
  				}
			}
		}

		return npos;
	}
	
	inline std::string
	to_string() const
	{
		return std::string( reinterpret_cast< const char* >( m_data ), m_size );
	}
	
	void
	dump( std::ostream& os ) const;

private:

	inline void
	move( buffer &rhs )
	{
		free();
	
		m_capacity		= rhs.m_capacity;
		rhs.m_capacity	= 0;
		m_size			= rhs.m_size;
		rhs.m_size		= 0;
		m_data			= rhs.m_data;
		rhs.m_data		= nullptr;
		m_deleter		= rhs.m_deleter;
	}
	
	inline void
	free()
	{
		if ( m_data )
		{
			if ( m_deleter )
			{
				m_deleter( m_data );
			}
			else
			{
				delete [] m_data;
			}

			m_data		= nullptr;
			m_deleter	= nullptr;
		}
	}

	elemtype	*m_data		= nullptr;
	sizetype	m_size		= 0;
	sizetype	m_capacity	= 0;
	deleter_f	m_deleter;
};

}




#endif
