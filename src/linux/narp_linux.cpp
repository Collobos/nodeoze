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

#include "narp_linux.h"
#include <nodeoze/nstring.h>
#include <nodeoze/nsocket.h>
#include <nodeoze/nmacros.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define xstr(s) str(s)
#define str(s) #s

#define ARP_CACHE       "/proc/net/arp"
#define ARP_STRING_LEN  1023
#define ARP_BUFFER_LEN  (ARP_STRING_LEN + 1)

#define ARP_LINE_FORMAT "%" xstr(ARP_STRING_LEN) "s %*s %*s " \
                        "%" xstr(ARP_STRING_LEN) "s %*s " \
                        "%" xstr(ARP_STRING_LEN) "s"


using namespace nodeoze;

static std::uint8_t
to_digit( char c )
{
	c = std::tolower( c );

	if ( ( c >= '0' ) || ( c <= '9' ) )
	{
		return ( c - '0' );
	}
	else
	{
		return ( c - 'a' ) + 10;
	}
}

static std::uint8_t
to_byte( const std::string &s )
{
	assert( s.size() == 2 );
	return ( to_digit( s[ 0 ] ) * 16 + to_digit( s[ 1 ] ) );
}

NODEOZE_DEFINE_SINGLETON( arp )

static int nflag;


arp*
arp::create()
{
	return new arp_linux;
}


arp::arp()
{
}


arp::~arp()
{
}


arp_linux::arp_linux()
{
}


arp_linux::~arp_linux()
{
}


promise< mac::address >
arp_linux::resolve( const ip::address &ip_address )
{
	auto sock	= std::make_shared< ip::udp::socket >();
	auto ret	= promise< mac::address >();
	
	auto err = sock->bind( ip::endpoint( ip::address::v4_any(), 0 ) );

	if ( !err )
	{
		buffer buf;
		
		buf.push_back( 1 );
		
		mlog( nodeoze::marker::arp, log::level_t::info, "sending 1 byte to udp socket for arp" );
		
		sock->send( std::move( buf ), ip::endpoint( ip_address, 80 ) )
		.then( [=]() mutable
		{
			bool found = lookup( ip_address, [&]( mac::address addr )
			{
				ret.resolve( std::move( addr ) );
			} );
				
			if ( !found )
			{
				ret.reject( std::make_error_code( err_t::not_exist ) );
			}
		},
		[=]( auto err ) mutable
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


template< class Visitor >
bool
arp_linux::lookup( const ip::address &ip_address, Visitor visitor )
{
	buffer			buf;
	bool			found = false;
	int				res;
    char	ipAddr[ARP_BUFFER_LEN];
	char	hwAddr[ARP_BUFFER_LEN];
	char	device[ARP_BUFFER_LEN];
	char	header[ARP_BUFFER_LEN];
    int		count = 0;
	int		num;
	char			*ret;

    FILE *cache = fopen(ARP_CACHE, "r");
	ncheck_error_action( cache, found = false, exit, "unable to open arp cache" );

	ret = fgets( header, sizeof( header ), cache );
	ncheck_error_action( ret, found = false, exit, "unable to read past header" );

    while ( fscanf( cache, ARP_LINE_FORMAT, ipAddr, hwAddr, device ) == 3 )
    {
		if ( ip_address.to_string() == ipAddr )
		{
			std::uint8_t	bytes[ 6 ];
			auto			parts = string::split( hwAddr, ':' );

			assert( parts.size() == 6 );

			bytes[ 0 ] = to_byte( parts[ 0 ] );
			bytes[ 1 ] = to_byte( parts[ 1 ] );
			bytes[ 2 ] = to_byte( parts[ 2 ] );
			bytes[ 3 ] = to_byte( parts[ 3 ] );
			bytes[ 4 ] = to_byte( parts[ 4 ] );
			bytes[ 5 ] = to_byte( parts[ 5 ] );

			mac::address addr( bytes );
			
			visitor( std::move( addr ) );
		
			found = true;
			break;
		}
    }

exit:

	if ( cache )
	{
    	fclose( cache );
	}

    return found;
}
