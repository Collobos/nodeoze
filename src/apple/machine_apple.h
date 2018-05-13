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

#ifndef _nodeoze_machine_apple_h
#define _nodeoze_machine_apple_h

#include <nodeoze/machine.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <CoreFoundation/CoreFoundation.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/if.h>

namespace nodeoze {

class machine_apple : public machine
{
public:

	class nif_apple : public nif
	{
	public:
	
		inline
		nif_apple( ifaddrs *addr )
		{
			m_name			= addr->ifa_name;
			m_display_name	= addr->ifa_name;
			m_index			= if_nametoindex( addr->ifa_name );
			
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
			
			switch ( addr->ifa_addr->sa_family )
			{
				case AF_INET:
				{
					m_address = ip::address( reinterpret_cast< sockaddr_in* >( addr->ifa_addr )->sin_addr );
					m_netmask = ip::address( reinterpret_cast< sockaddr_in* >( addr->ifa_netmask )->sin_addr );
					
				}
				break;
					
				case AF_INET6:
				{
					m_address = ip::address( reinterpret_cast< sockaddr_in6* >( addr->ifa_addr )->sin6_addr );
					m_netmask = ip::address( reinterpret_cast< sockaddr_in6* >( addr->ifa_netmask )->sin6_addr );
				}
				break;
					
				default:
				{
				}
				break;
			}
			
			m_mac_address = get_mac_address( m_name );
		}
		
	protected:
	
		mac::address
		get_mac_address( const std::string &name );
	};

	machine_apple();
	
	virtual ~machine_apple();
	
	virtual void
	refresh();
	
protected:

	static void
	system_configuration_callback( SCDynamicStoreRef store, CFArrayRef changed_keys, void *info );
	
	static void
	printer_change_notification( CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef user_info );
	
	std::string
	get_hostname();
	
	std::string
	get_display_name();
	
	std::string
	get_description();

	std::string
	serial_number();
};

}

#endif
