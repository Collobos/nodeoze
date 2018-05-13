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

#ifndef nodeoze_nstream_h
#define nodeoze_nstream_h

#include <nodeoze/buffer.h>
#include <nodeoze/strtk.hpp>
#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>

namespace nodeoze {

class ostream
{
public:

	ostream( std::streambuf *buf )
	:
		m_buf( buf )
	{
	}
	
	ostream( const ostream& ) = delete;
	
	ostream( ostream&& ) = delete;
	
	ostream&
	operator=( const ostream& ) = delete;
	
	ostream&
	operator=( ostream&& ) = delete;

	inline ostream&
	operator<<( bool val )
	{
		m_buf->sputc( static_cast< char >( val ) );
		return *this;
	}
	
	inline ostream&
	operator<<( char val )
	{
		m_buf->sputc( val );
		return *this;
	}
	
	inline ostream&
	operator<<( std::int8_t val)
	{
		return put_signed_integer( val );
	}
	
	inline ostream&
	operator<<( std::uint8_t val )
	{
		return put_unsigned_integer( val );
	}
	
	inline ostream&
	operator<<( std::int16_t val )
	{
		return put_signed_integer( val );
	}
	
	inline ostream&
	operator<<( std::uint16_t val )
	{
		return put_unsigned_integer( val );
	}
	
	inline ostream&
	operator<<( std::int32_t val )
	{
		return put_signed_integer( val );
	}
	
	inline ostream&
	operator<<( std::uint32_t val )
	{
		return put_unsigned_integer( val );
	}
	
	inline ostream&
	operator<<( std::int64_t val )
	{
		return put_signed_integer( val );
	}
	
	inline ostream&
	operator<<( std::uint64_t val )
	{
		return put_unsigned_integer( val );
	}
	
	inline ostream&
	operator<<( float val )
	{
		char buf[ 256 ];
#if defined( WIN32 )
		auto num = sprintf_s( buf, "%f", val );
#else
		auto num = sprintf( buf, "%f", val );
#endif
		m_buf->sputn( buf, num );
		
		return *this;
	}
	
	inline ostream&
	operator<<( double val )
	{
		char buf[ 256 ];
#if defined( WIN32 )
		auto num = sprintf_s( buf, "%f", val );
#else
		auto num = sprintf( buf, "%f", val );
#endif

		m_buf->sputn( buf, num );
		
		return *this;
	}
	
	inline ostream&
	operator<<( const char *val )
	{
		m_buf->sputn( val, strlen( val ) );
		return *this;
	}

	inline ostream&
	operator<<( const std::string_view &val )
	{
		m_buf->sputn( val.data(), val.size() );
		return *this;
	}
	
	inline ostream&
	operator<<( const std::string &val )
	{
		m_buf->sputn( val.data(), val.size() );
		return *this;
	}
	
	inline ostream&
	operator<<( void *val )
	{
		char buf[ 256 ];
#if defined( WIN32 )
		auto num = sprintf_s( buf, "%p", val );
#else
		auto num = sprintf( buf, "%p", val );
#endif
		m_buf->sputn( buf, num );
		
		return *this;
	}
	
	inline ostream&
	operator<<( ostream& (*pf)( ostream& ) )
	{
		return pf( *this );
	}
	
protected:

	inline ostream&
	put_signed_integer( std::int64_t val )
	{
		auto abs_val	= static_cast< std::uint64_t >( val );
		bool neg		= ( val < 0 );
		
		if ( neg )
		{
			m_buf->sputc( '-' );
			abs_val = 0 - abs_val;
		}
		
		return put_unsigned_integer( abs_val );
	}
	
	inline ostream&
	put_unsigned_integer( std::uint64_t val )
	{
		strtk::type_to_string( val, [&]( const char *buf, std::size_t count ) mutable
		{
			m_buf->sputn( buf, count );
		} );
		
		return *this;
	}

	static const char	m_digits[];
	std::streambuf		*m_buf;
};

class ostringstream : public ostream
{
public:

	ostringstream()
	:
		ostream( &m_buf )
	{
	}
	
	inline std::string
	str() const
	{
		return m_buf.str();
	}
	
	inline void
	str( const std::string &val )
	{
		m_buf.str( val );
	}
	
protected:

	std::stringbuf m_buf;
};

namespace stream {

enum class state_t
{
	disconnected,
	handshaking,
	connected,
	error
};

class filter
{
public:

	virtual ~filter()
	{
	}
	
	virtual const std::string&
	name() const = 0;
	
	virtual state_t
	state() const = 0;
	
	virtual state_t
	send( buffer &in_buf, buffer &out_buf ) = 0;
		
	virtual state_t
	recv( std::vector< buffer > &in_recv_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs ) = 0;
	
	virtual void
	reset() = 0;
};

inline std::ostream&
operator<<( std::ostream &os, stream::state_t state )
{
	switch ( state )
	{
		case state_t::disconnected:
		{
			os << "disconnected";
		}
		break;
		
		case state_t::handshaking:
		{
			os << "handshaking";
		}
		break;
		
		case state_t::connected:
		{
			os << "connected";
		}
		break;
		
		case state_t::error:
		{
			os << "error";
		}
		break;
	}
	
	return os;
}

}

}

#endif
