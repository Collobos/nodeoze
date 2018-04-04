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
 
#ifndef _nodeoze_ethernet_h
#define _nodeoze_ethernet_h

#include <nodeoze/nany.h>
#include <iostream>
#include <string>
#include <cstdint>

namespace nodeoze {

namespace mac {

class address
{
public:

	address();
	
	address( const std::uint8_t *bytes, std::size_t size );

	address( const nodeoze::buffer &bytes );

	address( const nodeoze::buffer_view &bytes );
	
	address( const std::string &s );

	template < class T >
	address( const T &t )
	{
		memset( m_bytes, 0, sizeof( m_bytes ) );
		assign( t );
	}

	~address();

	inline address&
	operator=( const address &rhs )
	{
		memcpy( m_bytes, rhs.m_bytes, sizeof( m_bytes ) );
		return *this;
	}

	inline bool
	operator==( const address &rhs ) const
	{
		return ( memcmp( m_bytes, rhs.m_bytes, sizeof( m_bytes ) ) == 0 );
	}

	inline bool
	operator!=( const address &rhs ) const
	{
		return !operator==( rhs );
	}

	inline bool
	is_valid() const
	{
		static const std::uint8_t empty[ 6 ] = { 0 };
		return ( memcmp( m_bytes, empty, sizeof( m_bytes ) ) != 0 ) ? true : false;
	}
	
	std::string
	to_string( const char delimiter = '-' ) const;
	
	any
	to_any() const;

	inline explicit operator bool () const
	{
		return ( m_bytes [ 0 ] || m_bytes[ 1 ] || m_bytes[ 2 ] || m_bytes[ 3 ] || m_bytes[ 4 ] || m_bytes[ 5 ] );
	}
	
private:

	template < class T >
    inline void
    assign( const T &t )
    {
        static_assert( sizeof( T ) == 6, "can only construct mac::address with 48 bit values" );
		std::memcpy( &m_bytes, &t, sizeof( t ) );
    }

	std::uint8_t m_bytes[ 6 ];
};

inline std::ostream&
operator<<( std::ostream &os, const mac::address &address )
{
	return os << address.to_string();
}

}

}

#endif
