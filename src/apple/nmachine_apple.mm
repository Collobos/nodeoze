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

#include "nmachine_apple.h"
#include <nodeoze/nmachine.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nnotification.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nlog.h>
#include <AssertMacros.h>
#include <uv.h>
#include <Foundation/Foundation.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <TargetConditionals.h>
#if ( TARGET_OS_IPHONE == 1 ) || ( TARGET_IPHONE_SIMULATOR == 1 )
#	include <UIKit/UIKit.h>
#endif
#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/stat.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <resolv.h>

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

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark machine_apple::nif_apple implementation
#endif

mac::address
machine_apple::nif_apple::get_mac_address( const std::string &name )
{
	std::string			id;
	int					mgmt_info_base[6];
	buffer				msg_buffer;
	std::size_t			length;
	struct if_msghdr	*interface_msg_struct;
	struct sockaddr_dl	*socket_struct;
	unsigned char       bytes[6];
	mac::address		mac;
	int					err;

	// Setup the management Information Base (mib)
	mgmt_info_base[0] = CTL_NET;        // Request network subsystem
	mgmt_info_base[1] = AF_ROUTE;       // Routing table info
	mgmt_info_base[2] = 0;
	mgmt_info_base[3] = AF_LINK;        // Request link layer information
	mgmt_info_base[4] = NET_RT_IFLIST;  // Request all configured interfaces

	// With all configured interfaces requested, get handle index
	
	mgmt_info_base[5] = if_nametoindex( name.c_str() );
	ncheck_error_quiet( mgmt_info_base[ 5 ] != 0, exit );
	
	// Get the size of the data available (store in len)
		
	err = sysctl( mgmt_info_base, 6, NULL, &length, NULL, 0 );
	ncheck_error_quiet( err >= 0, exit );
	
	// Alloc memory based on above call
			
	msg_buffer.size( length );
			
	// Get system information, store in buffer
				
	err = sysctl( mgmt_info_base, 6, msg_buffer.mutable_data(), &length, NULL, 0 );
	ncheck_error_quiet( err >= 0, exit );

	// Map msgbuffer to interface message structure
	
	interface_msg_struct = reinterpret_cast< if_msghdr* >( msg_buffer.mutable_data() );

	// Map to link-level socket structure
		
	socket_struct = reinterpret_cast< sockaddr_dl* >( interface_msg_struct + 1 );

	// Copy link layer address data in socket structure to an array
		
	memcpy( &bytes, socket_struct->sdl_data + socket_struct->sdl_nlen, 6 );
	
	mac = mac::address( bytes, sizeof( bytes ) );

exit:

	return mac;
}

#if defined( __APPLE__ )
#	pragma mark machine_apple implementation
#endif

machine&
machine::self()
{
	static machine_apple obj;
	
	return obj;
}


machine_apple::machine_apple()
{
#if ( TARGET_OS_IPHONE == 1 ) || ( TARGET_IPHONE_SIMULATOR == 1 )
#else
	SCDynamicStoreContext	context			= { 0, this, NULL, NULL, NULL };
	CFStringRef				keys[ 3 ]		= { CFSTR( "State:/Network/Interface/.*/IPv4" ) };
	CFArrayRef				watched_keys	= nullptr;
	CFRunLoopSourceRef		runloop_source	= nullptr;
	Boolean					ok;
	
	auto dynamic_store = SCDynamicStoreCreate( nullptr, ( CFStringRef ) [ [ NSProcessInfo processInfo ] processName ], system_configuration_callback, &context );
	ncheck_error_quiet( dynamic_store, exit );
	
	watched_keys = CFArrayCreate( kCFAllocatorDefault, reinterpret_cast< const void** >( keys ), 1, &kCFTypeArrayCallBacks );
	
	ok = SCDynamicStoreSetNotificationKeys( dynamic_store, NULL, watched_keys );
	ncheck_error_quiet( ok, exit );
	
	runloop_source = SCDynamicStoreCreateRunLoopSource( kCFAllocatorDefault, dynamic_store, 0 );
	ncheck_error_quiet( runloop_source, exit );
	
	CFRunLoopAddSource( CFRunLoopGetMain(), runloop_source, kCFRunLoopDefaultMode );
	CFRelease( runloop_source );
	
	CFNotificationCenterAddObserver( CFNotificationCenterGetDarwinNotifyCenter(), this, printer_change_notification, CFSTR( "com.apple.printerListChange" ), nullptr, CFNotificationSuspensionBehaviorDeliverImmediately );
	
exit:

	if ( watched_keys != nullptr )
	{
		CFRelease( watched_keys );
	}
	
#endif

	refresh();
}


machine_apple::~machine_apple()
{
	CFNotificationCenterRemoveObserver( CFNotificationCenterGetDarwinNotifyCenter(), this, CFSTR( "com.apple.printerListChange" ), nullptr );
}


void
machine_apple::refresh()
{
	ifaddrs *addrs = nullptr;
	
	m_name_servers.clear();
	m_domains.clear();
	m_nifs.clear();
	
	m_name				= get_hostname();
	m_display_name		= get_display_name();
	std::transform( m_name.begin(), m_name.end(), m_name.begin(), tolower );
	m_mdnsname			= m_name + ".local";
	m_description		= get_description();
	
	if ( ( _res.options & RES_INIT ) == 0 )
	{
		res_init();
	}
	
	for ( auto i = 0; i < _res.nscount; i++ )
	{
		auto address	= ip::address();
		auto saddr		= reinterpret_cast< sockaddr* >( &_res.nsaddr_list[ i ] );
		
		if ( saddr->sa_family == AF_INET )
		{
			address = ip::address( reinterpret_cast< sockaddr_in* >( &_res.nsaddr_list[ i ] )->sin_addr );
		}
		else if ( saddr->sa_family == AF_INET6 )
		{
			address = ip::address( reinterpret_cast< sockaddr_in6* >( &_res.nsaddr_list[ i ] )->sin6_addr );
		}
		
		if ( address )
		{
			m_name_servers.emplace_back( address, 53 );
		}
	}
	
	if ( strlen( _res.defdname ) > 0 )
	{
		m_domains.push_back( _res.defdname );
	}
	
	m_memory			= uv_get_total_memory();
	
	getifaddrs( &addrs );
	
	for ( auto addr = addrs; addr; addr = addr->ifa_next)
	{
		if ( addr->ifa_addr )
		{
			if ( addr->ifa_addr->sa_family == AF_INET )
			{
				m_nifs.emplace_back( nif_apple( addr ) );
			}
			else if ( addr->ifa_addr->sa_family == AF_INET6 )
			{
				m_nifs.emplace_back( nif_apple( addr ) );
			}
		}
	}
	
	mlog( marker::machine, log::level_t::info, "%", to_any() );

	if ( addrs )
	{
		freeifaddrs( addrs );
	}
}


std::string
machine_apple::get_hostname()
{
    char        buf[ 256 ];                    
    std::string name;                          
                                               
    memset( buf, 0, sizeof( 256 ) );           
                                               
    gethostname( buf, sizeof( buf ) );         
                                               
    for ( auto i = 0u; i < strlen( buf ); i++ )
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
machine_apple::get_display_name()
{
	return [ [ [ NSHost currentHost] localizedName ] UTF8String ];
}


std::string
machine_apple::get_description()
{
	std::string ret;
	
	@autoreleasepool
	{
		NSString		*systemVersionFile	= @"/System/Library/CoreServices/SystemVersion.plist";
		NSData			*data				= [ NSData dataWithContentsOfFile:systemVersionFile ];
		NSDictionary	*dict				= [ NSPropertyListSerialization propertyListWithData:data options:NSPropertyListImmutable format:nullptr error:nil ];
		NSString		*description		= [ NSString stringWithFormat:@"%@ %@ (%@)", [ dict objectForKey:@"ProductName" ], [ dict objectForKey:@"ProductVersion" ], [ dict objectForKey:@"ProductBuildVersion" ] ];
	
		ret = [ description UTF8String ];
	}
	
	return ret;
}


void
machine_apple::system_configuration_callback( SCDynamicStoreRef store, CFArrayRef changed_keys, void *info )
{
	nunused( store );
	nunused( changed_keys );
	
	auto self = reinterpret_cast< machine_apple* >( info );
	
	mlog( marker::machine, log::level_t::info, "received network change event" );
	
	runloop::shared().dispatch( [=]()
	{
		self->refresh();
		notification::shared().publish( notification::local, 0, make_oid( self ), network_change_event, self->to_any() );
	} );
}


void
machine_apple::printer_change_notification( CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef user_info )
{
	nunused( center );
	nunused( name );
	nunused( object );
	nunused( user_info );
	
	machine_apple *self = reinterpret_cast< machine_apple* >( observer );
	
	mlog( marker::machine, log::level_t::info, "printer change callback" );
	
	runloop::shared().dispatch( [=]()
	{
		notification::shared().publish( notification::local, 0, make_oid( self ), printer_change_event );
	} );
}
