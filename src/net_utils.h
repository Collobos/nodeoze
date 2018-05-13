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

#ifndef _nodeoze_endpoint_libuv_h
#define _nodeoze_endpoint_libuv_h

#include <nodeoze/endpoint.h>
#include "uv.h"

namespace nodeoze {

namespace ip {

inline void
endpoint_to_sockaddr( const ip::endpoint &endpoint, sockaddr_storage &sockaddr )
{
	memset( &sockaddr, 0, sizeof( sockaddr ) );

	if ( endpoint.is_v4() )
	{
		auto addr4 = reinterpret_cast< struct sockaddr_in* >( &sockaddr );
		
		addr4->sin_family	= AF_INET;
		endpoint.addr() >> addr4->sin_addr;
		addr4->sin_port		= htons( endpoint.port() );
#if defined( __APPLE__ )
		addr4->sin_len		= sizeof( sockaddr_in );
#endif
	}
	else if ( endpoint.is_v6() )
	{
		auto addr6 = reinterpret_cast< sockaddr_in6* >( &sockaddr );
		
		addr6->sin6_family	= AF_INET6;
		endpoint.addr() >> addr6->sin6_addr;
		addr6->sin6_port	= htons( endpoint.port() );
#if defined( __APPLE__ )
		addr6->sin6_len		= sizeof( sockaddr_in6 );
#endif
	}
}


inline void
sockaddr_to_endpoint( const sockaddr_storage &sockaddr, ip::endpoint &endpoint )
{
	if ( sockaddr.ss_family == AF_INET )
	{
		auto addr4 = reinterpret_cast< const sockaddr_in* >( &sockaddr );
		
		endpoint.set_addr( addr4->sin_addr );
		endpoint.set_port( ntohs( addr4->sin_port ) );
	}
	else if ( sockaddr.ss_family == AF_INET6 )
	{
		auto addr6 = reinterpret_cast< const sockaddr_in6* >( &sockaddr );
		endpoint.set_addr( addr6->sin6_addr );
		endpoint.set_port( ntohs( addr6->sin6_port ) );
	}
}

}

}

#endif
