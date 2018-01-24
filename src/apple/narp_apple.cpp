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

#include "narp_apple.h"
#include <nodeoze/nsocket.h>
#include <nodeoze/nmacros.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

using namespace nodeoze;

NODEOZE_DEFINE_SINGLETON( arp )

arp*
arp::create()
{
	return new arp_apple;
}


arp::arp()
{
}


arp::~arp()
{
}


arp_apple::arp_apple()
{
}


arp_apple::~arp_apple()
{
}


promise< mac::address >
arp_apple::resolve( const ip::address &ip_address )
{
	auto sock	= std::make_shared< ip::udp::socket >();
	auto ret	= promise< mac::address >();
	auto err	= sock->bind( ip::endpoint( ip::address::v4_any(), 0 ) );
	
	if ( !err )
	{
		mlog( nodeoze::marker::arp, log::level_t::info, "sending 1 byte to udp socket for arp" );
		
		buffer buf;
		
		buf.push_back( 1 );
		
		sock->send( std::move( buf ), ip::endpoint( ip_address, 80 ) )
		.then( [=]() mutable
		{
			return lookup( ip_address );
		} )
		.then( [=]( mac::address addr ) mutable
		{
			ret.resolve( std::move( addr ) );
		},
		[=]( std::error_code err ) mutable
		{
			ret.reject( err );
		} );
	}
	else
	{
		ret.reject( err );
	}
	
	return ret;
}


promise< mac::address >
arp_apple::lookup( const ip::address &ip_address )
{
	buffer			buf;
	int				mib[6];
	std::size_t		needed;
	std::uint8_t	*lim;
	std::uint8_t	*next;
	rt_msghdr		*rtm;
	sockaddr_inarp	*sin;
	sockaddr_dl		*sdl;
	bool			found = false;
	int				res;
	auto			ret = promise< mac::address >();

	mib[ 0 ] = CTL_NET;
	mib[ 1 ] = PF_ROUTE;
	mib[ 2 ] = 0;
	mib[ 3 ] = AF_INET;
	mib[ 4 ] = NET_RT_FLAGS;
	mib[ 5 ] = RTF_LLINFO;
	
	res = sysctl(mib, 6, NULL, &needed, NULL, 0 );
	ncheck_error( res >= 0, exit, "sysctl failed (%)", errno );
	buf.capacity( needed );
    res = sysctl(mib, 6, buf.data(), &needed, NULL, 0 );
	ncheck_error( res >= 0, exit, "sysctl failed (%)", errno );
	lim = buf.data() + needed;
	
	for ( next = buf.data(); next < lim; next += rtm->rtm_msglen )
	{
		rtm = reinterpret_cast< rt_msghdr *>( next );
		sin = reinterpret_cast< sockaddr_inarp* >( rtm + 1 );
		sdl = reinterpret_cast< sockaddr_dl* >( sin + 1 );
		
		if ( ip_address == sin->sin_addr.s_addr )
		{
			ret.resolve( mac::address( reinterpret_cast< std::uint8_t* >( LLADDR( sdl ) ), 6 ) );
			found = true;
			break;
        }
    }
	
exit:

	if ( !found )
	{
		ret.reject( make_error_code( err_t::not_exist ) );
	}

	return ret;
}
