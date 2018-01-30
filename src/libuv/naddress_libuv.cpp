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
 
#include <nodeoze/naddress.h>
#include <nodeoze/nrunloop.h>
#include "uv.h"
#include <thread>

using namespace nodeoze;

inline bool
operator==( sockaddr_storage s1, sockaddr_storage s2 )
{
    return memcmp( &s1, &s2, sizeof( sockaddr_storage ) ) == 0;
}

promise< std::vector< ip::address > >
ip::address::resolve( std::string host )
{
	promise< std::vector< ip::address > > ret;

	assert( host.size() > 0 );
	
	std::thread t( [=]()
	{
		std::vector< address >	addrs;
		struct addrinfo			*result;
		struct addrinfo			*res;
		std::ostringstream		os;
		int						err;
		
		err = getaddrinfo( host.c_str(), "0", NULL, &result );
    
		if ( err == 0 )
		{
			std::deque< struct sockaddr_storage > natives;

			for ( res = result; res != NULL; res = res->ai_next)
			{
				std::deque< struct sockaddr_storage >::iterator it;
				struct sockaddr_storage							storage;

				memset( &storage, 0, sizeof( storage ) );
				memcpy( &storage, res->ai_addr, res->ai_addrlen );
				
				it = std::find( natives.begin(), natives.end(), storage );
				
				if ( it == natives.end() )
				{
					if ( res->ai_family == AF_INET )
					{
						addrs.emplace_back( ( ( struct sockaddr_in* ) res->ai_addr )->sin_addr );
					}
					else if ( res->ai_family == AF_INET6 )
					{
						addrs.emplace_back( ( ( struct sockaddr_in6* ) res->ai_addr )->sin6_addr );
					}
					
					natives.push_back( storage );
				}
			}
 
			freeaddrinfo( result );
		}

		runloop::shared().dispatch( [=]() mutable
		{
			switch ( err )
			{
				case 0:
				{
					ret.resolve( std::move( addrs ) );
				}
				break;

#if !defined( WIN32 )

				case EAI_ADDRFAMILY:
				{
					ret.reject( make_error_code( std::errc::address_family_not_supported ), reject_context );
				}
				break;

#endif

				case EAI_AGAIN:
				{
					ret.reject( make_error_code( std::errc::resource_unavailable_try_again ), reject_context );
				}
				break;

				case EAI_BADFLAGS:
				{
					ret.reject( make_error_code( std::errc::invalid_argument ), reject_context );
				}
				break;

				case EAI_FAIL:
				{
					ret.reject( make_error_code( std::errc::bad_address ), reject_context );
				}
				break;

				case EAI_FAMILY:
				{
					ret.reject( make_error_code( std::errc::address_family_not_supported ), reject_context );
				}
				break;

				case EAI_MEMORY:
				{
					ret.reject( make_error_code( std::errc::not_enough_memory ), reject_context );
				}
				break;

				case EAI_NODATA:
				{
					ret.reject( make_error_code( std::errc::no_message ), reject_context );
				}
				break;

#if !defined( WIN32 )

				case EAI_NONAME:
				{
					ret.reject( make_error_code( std::errc::no_message ), reject_context );
				}
				break;

#endif

				case EAI_SERVICE:
				{
					ret.reject( make_error_code( std::errc::wrong_protocol_type ), reject_context );
				}
				break;

				case EAI_SOCKTYPE:
				{
					ret.reject( make_error_code( std::errc::wrong_protocol_type ), reject_context );
				}
				break;

#if !defined( WIN32 )

				case EAI_SYSTEM:
				{
					ret.reject( std::error_code( errno, std::generic_category() ), reject_context );
				}
				break;

#endif
				
				default:
				{
					ret.reject( make_error_code( std::errc::host_unreachable ), reject_context );
				}
			}
		} );
	} );
	
	t.detach();

	return ret;
}
