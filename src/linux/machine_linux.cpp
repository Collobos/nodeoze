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

#include "nmachine_linux.h"
#include <nodeoze/nmachine.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nnotification.h>
#include <nodeoze/nmacros.h>
#include <sys/utsname.h>
#include <nodeoze/nlog.h>
#include <netpacket/packet.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <net/if.h>
#include <netinet/in.h>
#include <resolv.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>

#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <stdio.h>

#define MAX_DNS_PROPERTIES		8
#define DNS_PROP_NAME_PREFIX	"net.dns"
#define DNS_SEARCH_PROP_NAME	"net.dns.search"

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark machine_linux implementation
#endif

machine&
machine::self()
{
	static machine_linux obj;

	return obj;
}


machine_linux::machine_linux()
{
	refresh();
}


machine_linux::~machine_linux()
{
}


void
machine_linux::refresh()
{
	ifaddrs *addrs = nullptr;

	m_name_servers.clear();
	m_domains.clear();
	m_nifs.clear();

	m_name			= get_hostname();
	m_display_name	= m_name;
	m_mdnsname		= m_name + ".local";
	m_description	= get_description();

	mlog( marker::machine, log::level_t::info, "hostname: %, mdnsname: %, description: %", m_name, m_mdnsname, m_description );

    if ( ( _res.options & RES_INIT ) == 0 )
    {
        res_init();
    }

    for ( auto i = 0; i < _res.nscount; i++ )
    {
        auto saddr = reinterpret_cast< sockaddr* >( &_res.nsaddr_list[ i ] );

        if ( saddr->sa_family == AF_INET )
        {
            m_name_servers.emplace_back( ip::address( reinterpret_cast< sockaddr_in* >( &_res.nsaddr_list[ i ] )->sin_addr ), 53 );
        }
        else if ( saddr->sa_family == AF_INET6 )
        {
            m_name_servers.emplace_back( ip::address( reinterpret_cast< sockaddr_in6* >( &_res.nsaddr_list[ i ] )->sin6_addr ), 53 );
        }
    }

    m_domains.push_back( _res.defdname );

	for ( auto &it : m_name_servers )
	{
		mlog( marker::machine, log::level_t::info, "name server: %", it.to_string() );
	}

	getifaddrs( &addrs );

	std::unordered_map< std::string, mac::address > mac_addresses;

	for ( auto addr = addrs; addr; addr = addr->ifa_next )
	{
		if ( addr->ifa_addr && ( addr->ifa_addr->sa_family == AF_PACKET ) )
		{
			auto hw = reinterpret_cast< sockaddr_ll* >( addr->ifa_addr );
			mac_addresses[ addr->ifa_name ] = mac::address( hw->sll_addr, hw->sll_halen );
		}
	}

	for ( auto addr = addrs; addr; addr = addr->ifa_next )
	{
		auto nif	= nif_linux( addr );
		auto it		= mac_addresses.find( addr->ifa_name );

		if ( it != mac_addresses.end() )
		{
			nif.set_mac_address( it->second );
		}

		m_nifs.emplace_back( nif );
	}

exit:

	if ( addrs )
	{
		freeifaddrs( addrs );
	}
}


std::string
machine_linux::get_hostname()
{
    char        buf[ 256 ];
    std::string name;

    memset( buf, 0, sizeof( 256 ) );

    gethostname( buf, sizeof( buf ) );

    for ( auto i = 0; i < strlen( buf ); i++ )
    {
        if ( buf[ i ] != '.' )
        {
            name.push_back( buf[ i ] );
        }
        else
        {
            break;
        }
    }

    if ( name.size() == 0 )
    {
        name = "localhost";
    }

    return name;
}


std::string
machine_linux::get_description()
{
	std::string		distro;
	std::string		kernel;
	utsname			buf;
	std::ifstream	ifs( "/etc/os-release" );
	std::string		ret;

	uname( &buf );

	kernel = std::string( buf.sysname ) + " " + std::string( buf.release ) + " " + std::string( buf.machine );

	if ( ifs.is_open() )
	{
		std::string line;

		while ( std::getline( ifs, line ) )
		{
			auto keyval = string::split( line, '=' );

			if ( keyval[ 0 ] == "PRETTY_NAME" )
			{
				distro = keyval[ 1 ];

				if ( distro.front() == '"' )
				{
					distro.erase( distro.begin() );
				}

				if ( distro.back() == '"' )
				{
					distro.pop_back();
				}

				break;
			}
		}
	}

	if ( distro.size() > 0 )
	{
		ret = distro + " (" + kernel + ")";
	}
	else
	{
		ret = kernel;
	}

	return ret;
}


std::string
machine_linux::get_uuid()
{
	static std::string	id;

	if ( id.size() == 0 )
	{
/*
		char tmp[ 1024 ];
		int  ret;

		auto fp = fopen( "/var/lib/dbus/machine-id", "r" );
		nrequire2( fp, exit, "fopen() failed: %", errno );
		ret = fscanf( fp, "%s", tmp );
		fclose( fp );
		nrequire2( ret > 0, exit, "fscanf() failed: %", errno );
		id = tmp;
*/

		id = "ff-3e-e1-c7-a6-d8";
	}

exit:

	return id;
}
