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
 
#include <nodeoze/endpoint.h>
#include <nodeoze/any.h>
#include <nodeoze/string.h>
#include <nodeoze/unicode.h>
#include <sstream>

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark ip::endpoint implementation
#endif

ip::endpoint::endpoint()
:
	m_port( 0 )
{
}


ip::endpoint::endpoint( const ip::address &addr, uint16_t port )
:
	m_addr( addr ),
	m_port( port )
{
}

	
ip::endpoint::endpoint( const uri &uri )
:
	m_addr( uri.host() ),
	m_port( uri.port() )
{
	if ( m_port == 0 )
	{
	}
}


ip::endpoint::endpoint( const nodeoze::any &val )
:
	m_addr( val[ "address" ] ),
	m_port( val[ "port" ].to_uint16() )
{
}


ip::endpoint::endpoint( const nodeoze::any &val, const endpoint &rhs )
:
	m_addr( val[ "address" ] ),
	m_port( val[ "port" ].to_uint16() )
{
	if ( !m_addr || !m_port )
	{
		m_addr = rhs.m_addr;
		m_port = rhs.m_port;
	}
}


ip::endpoint::~endpoint()
{
}


std::string
ip::endpoint::to_string() const
{
	std::string ret;
	
	ret = m_addr.to_string();
	ret += ":" + std::to_string( m_port );

	return ret;
}


any
ip::endpoint::to_any() const
{
	any v;
	
	v[ "address" ]	= m_addr.to_any();
	v[ "port" ]		= m_port;
	
	return v;
}
