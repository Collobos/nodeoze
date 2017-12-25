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

#ifndef _nodeoze_machine_android_h
#define _nodeoze_machine_android_h

#include <nodeoze/nmachine.h>
#include <netinet/in.h>
#include <android-ifaddrs.h>
#include <net/if.h>

namespace nodeoze {

class machine_android : public machine
{
public:

	class nif_android : public nif
	{
	public:
	
		inline
		nif_android( ifaddrs *addr )
		{
			m_name			= addr->ifa_name;
			m_display_name	= addr->ifa_name;
			
			if ( addr->ifa_flags & IFF_LOOPBACK )
			{
				m_flags |= nif::flags_t::loopback;
			}
					
			if ( addr->ifa_flags & IFF_MULTICAST )
			{
				m_flags |= nif::flags_t::multicast;
			}
					
			if ( addr->ifa_flags & IFF_POINTOPOINT )
			{
				m_flags |= nif::flags_t::point_to_point;
			}
					
			if ( addr->ifa_flags & IFF_BROADCAST )
			{
				m_flags |= nif::flags_t::broadcast;
			}
					
			if ( addr->ifa_flags & IFF_UP )
			{
				m_flags |= nif::flags_t::up;
			}
					
			if ( addr->ifa_addr->sa_family == AF_INET )
			{
				m_address = ip::address( reinterpret_cast< sockaddr_in* >( addr->ifa_addr )->sin_addr );
				m_netmask = ip::address( reinterpret_cast< sockaddr_in* >( addr->ifa_netmask )->sin_addr );
			}
			else if ( addr->ifa_addr->sa_family == AF_INET6 )
			{
				m_address = ip::address( reinterpret_cast< sockaddr_in6* >( addr->ifa_addr )->sin6_addr );
				m_netmask = ip::address( reinterpret_cast< sockaddr_in6* >( addr->ifa_netmask )->sin6_addr );
			}
		}
	};

	machine_android();
	
	virtual ~machine_android();
	
	virtual void
	refresh();
	
protected:

	std::string
	get_hostname();
	
	std::string
	get_description();

	std::string
	get_uuid();
};

}

#endif
