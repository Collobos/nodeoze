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

#ifndef _nodeoze_address_h
#define _nodeoze_address_h

#include <nodeoze/nany.h>
#include <nodeoze/npromise.h>
#include <functional>
#include <iostream>
#include <deque>

namespace nodeoze {

namespace ip {

class address
{
public:

	enum class type_t
	{
		unknown = -1,
		v4		= 0,
		v6		= 1
	};
	
public:

	static promise< std::vector< ip::address > >
	resolve( std::string host );
	
	static const address&
	v4_loopback();
	
	static const address&
	v6_loopback();
	
	static const address&
	v4_any();
	
	static const address&
	v6_any();
	
	address( type_t type = type_t::v4 )
	:
		m_type( type ),
		m_addr( { { 0 } } )
	{
	}
	
	address( const char *val );
	
	address( const std::string &val );
	
	address( const nodeoze::any &val );
	
	inline
	address( const address &rhs )
	:
		m_type( rhs.m_type )
	{
		std::memcpy( &m_addr, &rhs.m_addr, sizeof( m_addr ) );
	}
	
	template < class T >
	address( const T &t )
	:
		m_type( type_t::unknown ),
		m_addr( { { 0 } } )
	{
		assign( t );
	}
	
	inline type_t
	type() const
	{
		return m_type;
	}
	
	inline bool
	is_v4() const
	{
		return ( m_type == type_t::v4 );
	}
	
	inline bool
	is_v6() const
	{
		return ( m_type == type_t::v6 );
	}
	
	inline bool
	is_loopback_v4() const
	{
		return is_v4() && ( m_addr.m_b[ 0 ] == 127 ) && ( m_addr.m_b[ 1 ] == 0 ) && ( m_addr.m_b[ 2 ] == 0 ) && ( m_addr.m_b[ 3 ] == 1 );
	}
	
	inline bool
	is_loopback_v6() const
	{
		return is_v6() && ( m_addr.m_l[ 0 ] == 0 ) && ( m_addr.m_l[ 1 ] == 0 ) && ( m_addr.m_l[ 2 ] == 0 ) && ( m_addr.m_b[ 12 ] == 0 ) && ( m_addr.m_b[ 13 ] == 0 ) && ( m_addr.m_b[ 14 ] == 0 ) && ( m_addr.m_b[ 15 ] == 1 );
	}
	
	inline bool
	is_loopback() const
	{
		return ( is_loopback_v4() || is_loopback_v6() );
	}

	inline bool
	is_link_local_v4() const
	{
		return is_v4() && ( m_addr.m_b[ 0 ] == 169 ) && ( m_addr.m_b[ 1 ] == 254 );
	}
	
	inline bool
	is_link_local_v6() const
	{
		return is_v6() && ( m_addr.m_b[ 0 ] == 0xfe ) && ( ( m_addr.m_b[ 1 ] & 0xc0 ) == 0x80 );
	}
	
	inline bool
	is_link_local() const
	{
		return ( is_link_local_v4() || is_link_local_v6() );
	}
	
	inline bool
	is_multicast_v4() const
	{
		return is_v4() && ( ( m_addr.m_b[ 0 ] >= 224 ) && ( m_addr.m_b[ 0 ] < 239 ) );
	}
	
	inline bool
	is_multicast_v6() const
	{
		return is_v6() && ( m_addr.m_b[ 0 ] == 0xff );
	}
	
	inline bool
	is_multicast() const
	{
		return ( is_multicast_v4() || is_multicast_v6() );
	}
	
	inline bool
	is_v4_any() const
	{
		return is_v4() && ( m_addr.m_l[ 0 ] == 0 );
	}
	
	inline bool
	is_v6_any() const
	{
		return is_v6() && ( m_addr.m_l[ 0 ] == 0 ) && ( m_addr.m_l[ 1 ] == 0 ) && ( m_addr.m_l[ 2 ] == 0 ) && ( m_addr.m_l[ 3 ] == 0 );
	}
	
	inline bool
	is_any() const
	{
		return ( is_v4_any() || is_v6_any() );
	}
	
	std::string
	to_string() const;
	
	std::string
	to_reverse_string() const;
	
	nodeoze::any
	to_any() const;
	
	inline address&
	operator=( const address &rhs )
	{
		m_type = rhs.m_type;
		std::memcpy( &m_addr, &rhs.m_addr, sizeof( m_addr ) );
		return *this;
	}
	
	template < class T >
	inline address&
	operator=( const T &t )
	{
		assign( t );
		return *this;
	}
	
	template < class T >
	inline address&
    operator<<( const T &t )
    {
        assign( t );
        return *this;
    }

	template < class T >
	inline const address&
	operator>>( T &t ) const
	{
		static_assert( ( sizeof( T ) == 4 ) || ( sizeof( T ) == 16 ), "can only construct type with 32 or 128 bit values" );
		memcpy( &t, &m_addr, sizeof( t ) );
		return *this;
	}
	
	inline address
	operator&( const address &rhs ) const
	{
		address a;
		
		if ( m_type == rhs.m_type )
		{
			a.m_type			= m_type;
			a.m_addr.m_l[ 0 ]	= m_addr.m_l[ 0 ] & rhs.m_addr.m_l[ 0 ];
			a.m_addr.m_l[ 1 ]	= m_addr.m_l[ 1 ] & rhs.m_addr.m_l[ 1 ];
			a.m_addr.m_l[ 2 ]	= m_addr.m_l[ 2 ] & rhs.m_addr.m_l[ 2 ];
			a.m_addr.m_l[ 3 ]	= m_addr.m_l[ 3 ] & rhs.m_addr.m_l[ 3 ];
		}
		else
		{
			nlog( log::level_t::warning, "address type mismatch: % -> %", to_string(), rhs.to_string() );
		}
		
		return a;
	}

	inline bool
	operator==( const address &rhs ) const
	{
		return ( m_type == rhs.m_type ) && ( memcmp( &m_addr, &rhs.m_addr, sizeof( m_addr ) ) == 0 );
	}
	
	inline bool
	operator!=( const address &rhs ) const
	{
		return ( m_type != rhs.m_type ) || ( memcmp( &m_addr, &rhs.m_addr, sizeof( m_addr ) ) != 0 );
	}
	
	inline bool
	operator<( const address &rhs ) const
	{
		if ( m_type == rhs.m_type )
		{
			for ( auto i = 0; i < 4; i++ )
			{
				if ( m_addr.m_l[ i ] != rhs.m_addr.m_l[ i ] )
				{
					return ( m_addr.m_l[ i ] < rhs.m_addr.m_l[ i ] );
				}
			}
			
			return false;
		}
		else
		{
			return ( m_type < rhs.m_type );
		}
	}
	
	inline std::uint8_t
	to_uint8( std::size_t index ) const
	{
		if ( index < 16 )
		{
			return m_addr.m_b[ index ];
		}
		else
		{
			nlog( log::level_t::warning, "index (%) out of bounds", index );
			return 0;
		}
	}
	
	inline std::uint16_t
	to_uint16( std::size_t index ) const
	{
		if ( index < 8 )
		{
			return m_addr.m_s[ index ];
		}
		else
		{
			nlog( log::level_t::warning, "index (%) out of bounds", index );
			return 0;
		}
	}
	
	inline std::uint32_t
	to_uint32( std::size_t index ) const
	{
		if ( index < 4 )
		{
			return m_addr.m_l[ index ];
		}
		else
		{
			nlog( log::level_t::warning, "index (%) out of bounds", index );
			return 0;
		}
	}

	inline static bool
	integrity_check( const any& root )
	{
		bool result = true;
		if ( ! root.is_array() )
		{
			nlog( log::level_t::error, "ip address is not an array" );
			result = false;
		}
		else if ( ! ( root.size() == 1 || root.size() == 4 || root.size() == 16 ) )
		{
			nlog( log::level_t::error, "ip address array size is %, should be 1, 4 or 16", root.size() );
			result = false;
		}
		else
		{
			for ( auto i = 0u; i < root.size(); i++ )
			{
				if ( !root[ i ].is_integer() )
				{
					nlog( log::level_t::error, "ip address array element [%] is type %, should be an integer", i, root[0].type() );
					result = false;
				}
			}
		}
		return result;
	}
	
	inline explicit operator bool () const
	{
		return ( m_addr.m_l[ 0 ] || m_addr.m_l[ 1 ] || m_addr.m_l[ 2 ] || m_addr.m_l[ 3 ] );
	}
	
protected:

	void
	from_string( const std::string &s );

	void
	from_v4( const std::string &s );
	
	void
	from_v6( const std::string &s );
	
	void
	to_v4( std::ostream &os ) const;
	
	void
	to_reverse_v4( std::ostream &os ) const;
	
	void
	to_v6( std::ostream &os ) const;

	void
	to_reverse_v6( std::ostream &os ) const;
	
	template < class T >
    inline void
    assign( const T &t )
    {
        static_assert( ( sizeof( T ) == 4 ) || ( sizeof( T ) == 16 ), "can only construct ip::address with 32 or 128 bit values" );
		m_type = ( sizeof( t ) == 4 ) ? type_t::v4 : type_t::v6;
		std::memcpy( &m_addr, &t, sizeof( t ) );
    }

	type_t m_type;
	
	union addr_t
	{
		std::uint8_t	m_b[ 16 ];
		std::uint16_t	m_s[ 8 ];
		std::uint32_t	m_l[ 4 ];
	} m_addr;
	
	static_assert( sizeof( addr_t ) == 16, "m_addr not 128 bits" );
};

inline std::ostream&
operator<<( std::ostream &os, const ip::address::type_t type )
{
	switch ( type )
	{
		case ip::address::type_t::v4:
		{
			os << "ipv4";
		}
		break;
		
		case ip::address::type_t::v6:
		{
			os << "ipv6";
		}
		break;
		
		default:
		{
			os << "unknown";
		}
		break;
	}
	
	return os;
}

inline std::ostream&
operator<<( std::ostream &os, const ip::address &addr )
{
	return os << addr.to_string();
}

inline std::ostream&
operator<<( std::ostream &os, const std::vector< ip::address > &addrs )
{
	for ( auto &address : addrs )
	{
		os << address << " ";
	}
	
	return os;
}

}

}

namespace std {

template <>
struct hash< nodeoze::ip::address >
{
	typedef nodeoze::ip::address	argument_type;
	typedef std::size_t				result_type;
 
	result_type operator()( const argument_type &v ) const
	{
		result_type val	= std::hash< std::uint32_t >()( v.to_uint32( 0 ) );
		result_type res	= val;
		
		if ( v.is_v6() )
		{
			val = ( std::hash< std::uint32_t >()( v.to_uint32( 1 ) ) );
			res = ( res << 1 ) + res + val;
			val = ( std::hash< std::uint32_t >()( v.to_uint32( 2 ) ) );
			res = ( res << 1 ) + res + val;
			val = ( std::hash< std::uint32_t >()( v.to_uint32( 3 ) ) );
			res = ( res << 1 ) + res + val;
		}
		
		return res;
    }
};

}

#endif
