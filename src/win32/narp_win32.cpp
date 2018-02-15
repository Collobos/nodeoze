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

#include "narp_win32.h"
#include <nodeoze/nmachine.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nmacros.h>
#include <WinSock2.h>
#include <IPHlpApi.h>

using namespace nodeoze;

NODEOZE_DEFINE_SINGLETON( arp )

static int nflag;


arp*
arp::create()
{
	return new arp_win32;
}


arp::arp()
{
}


arp::~arp()
{
}


arp_win32::arp_win32()
{
}


arp_win32::~arp_win32()
{
}


promise< mac::address >
arp_win32::resolve( const ip::address &ip_address )
{
	auto self	= false;
	auto ret	= promise< mac::address >();

	for ( auto nif : machine::self().nifs() )
	{
		if ( nif.address() == ip_address )
		{
			mac::address mac = nif.mac_address();

			runloop::shared().dispatch( [=]() mutable
			{
				ret.resolve( std::move( mac ) );
			} );

			self = true;
			break;
		}
	}

	if ( !self )
	{
		std::thread t( [=]() mutable
		{
			IPAddr			dest;
			ULONG			bytes[ 2 ];
			ULONG			len = sizeof( bytes );
			mac::address	mac;
			auto			err = std::error_code();

			ip_address >> dest;

			auto aerr = SendARP( dest, INADDR_ANY, &bytes, &len );
			ncheck_error_action( aerr == NO_ERROR, err = make_error_code( std::errc::no_such_device_or_address ), exit, "SendARP to % failed (%)", ip_address.to_string(), err );
			ncheck_error_action( len == 6, err = make_error_code( std::errc::no_such_device_or_address ), exit, "bad mac address size: %", len );
			mac = mac::address( reinterpret_cast< std::uint8_t* >( &bytes ), len );

			mlog( marker::arp, log::level_t::info, "% -> %", ip_address.to_string(), mac.to_string() );

		exit:

			runloop::shared().dispatch( [=]() mutable
			{
				if ( !err )
				{
					ret.resolve( std::move( mac ) );
				}
				else
				{
					ret.reject( err, reject_context );
				}
			} );
		} );

		t.detach();
	}

	return ret;
}
