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
 
#ifndef _nodeoze_endpoint_h
#define _nodeoze_endpoint_h

#include <nodeoze/nuri.h>
#include <nodeoze/naddress.h>
#include <nodeoze/nany.h>
#include <deque>

namespace nodeoze {

namespace ip {

class endpoint
{
public:

	endpoint();
	
	endpoint( const nodeoze::uri &uri );
	
	endpoint( const address &host, std::uint16_t port );
	
	endpoint( const nodeoze::any &val );

	endpoint( const nodeoze::any &val, const endpoint &rhs );
	
	endpoint( const endpoint &rhs )
	:
		m_addr( rhs.m_addr ),
		m_port( rhs.m_port )
	{
	}

	~endpoint();
	
	inline endpoint&
	operator=( const endpoint &rhs )
	{
		m_addr = rhs.m_addr;
		m_port = rhs.m_port;
		
		return *this;
	}
	
	inline bool
	operator==( const endpoint &rhs ) const
	{
		return ( m_addr == rhs.m_addr ) && ( m_port == rhs.m_port );
	}
	
	inline bool
	operator!=( const endpoint &rhs ) const
	{
		return ( m_addr != rhs.m_addr ) || ( m_port != rhs.m_port );
	}
	
	std::string
	to_string() const;
	
	any
	to_any() const;
	
	inline bool
	is_v4() const
	{
		return m_addr.is_v4();
	}
	
	inline bool
	is_v6() const
	{
		return m_addr.is_v6();
	}
	
	inline const address&
	addr() const
	{
		return m_addr;
	}
	
	inline void
	set_addr( const ip::address &addr )
	{
		m_addr = addr;
	}
	
	inline std::uint16_t
	port() const
	{
		return m_port;
	}
	
	inline void
	set_port( std::uint16_t val )
	{
		m_port = val;
	}

	explicit operator bool () const
	{
		return ( m_addr && m_port );
	}
	
protected:

	address			m_addr;
	std::uint16_t	m_port;
};

inline std::ostream&
operator<<( std::ostream &os, const ip::endpoint &endpoint )
{
	return os << endpoint.to_string();
}

}

}

namespace std
{

template <>
struct hash< nodeoze::ip::endpoint >
{
	std::size_t
	operator()( const nodeoze::ip::endpoint& val ) const
	{
		std::string s( val.to_string() );
		std::transform( s.begin(), s.end(), s.begin(), ::tolower );
		return hash< std::string >()( s );
	}
};

}

#endif
