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

#include "nmachine_android.h"
#include <nodeoze/nmachine.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nnotification.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nlog.h>
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <net/if.h>
#include <netinet/in.h>
#include <sys/system_properties.h>

#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <netdb.h>

#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
    
#define MAX_DNS_PROPERTIES		8
#define DNS_PROP_NAME_PREFIX	"net.dns"
#define DNS_SEARCH_PROP_NAME	"net.dns.search"

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark machine_android implementation
#endif

machine&
machine::self()
{
	static machine_android obj;
	
	return obj;
}


machine_android::machine_android()
{
	refresh();
}


machine_android::~machine_android()
{
}


void
machine_android::refresh()
{
	char	prop_name[ PROP_NAME_MAX ];
	char	prop_value[ PROP_VALUE_MAX ] = "";
	ifaddrs *addrs						= nullptr;
	
	m_name_servers.clear();
	m_domains.clear();
	m_nifs.clear();
	
	m_hostname		= get_hostname();
	m_mdnsname		= m_hostname + ".local";
	m_description	= get_description();

	mlog( marker::machine, log::level_t::info, "hostname = %, mdnsname = %, description = %", m_hostname, m_mdnsname, m_description );

	for ( auto i = 1; i <= MAX_DNS_PROPERTIES; i++)
	{
		snprintf( prop_name, sizeof( prop_name ), "%s%u", DNS_PROP_NAME_PREFIX, i );

		if ( __system_property_get( prop_name, prop_value ) < 1 )
		{
			break;
		}

		ip::endpoint endpoint( ip::address( reinterpret_cast< const char* >( prop_value ) ), 53 );
		
		if ( std::find( m_name_servers.begin(), m_name_servers.end(), endpoint ) == m_name_servers.end() )
		{
			m_name_servers.emplace_back( endpoint );
		}
	}
	
	for ( auto &it : m_name_servers )
	{
		mlog( marker::machine, log::level_t::info, "name server: %", it.to_string() );
	}

	if ( __system_property_get( DNS_SEARCH_PROP_NAME, prop_value ) >= 1 )
	{
		mlog( marker::machine, log::level_t::info, "domain: %", prop_value );
		m_domains.push_back( prop_value );
	}
	
	getifaddrs( &addrs );
	
	for ( auto addr = addrs; addr; addr = addr->ifa_next)
	{
		if ( addr->ifa_addr )
		{
			if ( ( addr->ifa_addr->sa_family == AF_INET ) || ( addr->ifa_addr->sa_family == AF_INET6 ) )
			{
				m_nifs.emplace_back( nif_android( addr ) );
			}
		}
	}
	
	for ( auto &it : m_nifs )
	{
		mlog( marker::machine, log::level_t::info, "interface address: %", it.address().to_string() );
		mlog( marker::machine, log::level_t::info, "interface netmask: %", it.netmask().to_string() );
		mlog( marker::machine, log::level_t::info, "interface subnet: %", ( it.address() & it.netmask() ).to_string() );
	}
	
exit:

	if ( addrs )
	{
		freeifaddrs( addrs );
	}
}


std::string
machine_android::get_hostname()
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
machine_android::get_description()
{
	return "android";
}


std::string
machine_android::get_uuid()
{
	std::string id;
	
	if ( id.size() == 0 )
	{
	}

	return id;
}
