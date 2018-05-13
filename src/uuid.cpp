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

#include <nodeoze/uuid.h>
#include <nodeoze/buffer.h>
#include <nodeoze/base64.h>
#include <cstring>
#include <sstream>

using namespace nodeoze;

#if defined( __APPLE__ )

#	include <CoreFoundation/CoreFoundation.h>

uuid::uuid()
{
	CFUUIDRef	uuid_ref     = CFUUIDCreate( nullptr );
	CFUUIDBytes	uuid_bytes   = CFUUIDGetUUIDBytes( uuid_ref );
	
	m_data[ 0 ] = uuid_bytes.byte0;
	m_data[ 1 ] = uuid_bytes.byte1;
	m_data[ 2 ] = uuid_bytes.byte2;
	m_data[ 3 ] = uuid_bytes.byte3;
	m_data[ 4 ] = uuid_bytes.byte4;
	m_data[ 5 ] = uuid_bytes.byte5;
	m_data[ 6 ] = uuid_bytes.byte6;
	m_data[ 7 ] = uuid_bytes.byte7;
	m_data[ 8 ] = uuid_bytes.byte8;
	m_data[ 9 ] = uuid_bytes.byte9;
	m_data[ 10 ] = uuid_bytes.byte10;
	m_data[ 11 ] = uuid_bytes.byte11;
	m_data[ 12 ] = uuid_bytes.byte12;
	m_data[ 13 ] = uuid_bytes.byte13;
	m_data[ 14 ] = uuid_bytes.byte14;
	m_data[ 15 ] = uuid_bytes.byte15;

	CFRelease( uuid_ref );
}

#elif defined( WIN32 )

#include <WinSock2.h>
#include <WinCrypt.h>
#include "CRC32.h"

uuid::uuid()
{
	GUID guid;

	if ( CoCreateGuid( &guid ) == S_OK )
	{
		memcpy( m_data, &guid.Data1, sizeof( guid.Data1 ) );
		memcpy( m_data + 4, &guid.Data2, sizeof( guid.Data2 ) );
		memcpy( m_data + 6, &guid.Data3, sizeof( guid.Data3 ) );

		for ( auto i = 0; i < 8; i++ )
		{
			m_data[ i + 8 ] = guid.Data4[ i ];
		}
	}
	else
	{
		// fall back in case of failure - build a custom GUID
		LARGE_INTEGER cpuTime;

		QueryPerformanceCounter(&cpuTime);
 
		long guidValue = CRC32( ( const unsigned char* ) &cpuTime, sizeof( cpuTime ) );
 
		memcpy( m_data, &guidValue, 8 );
	}
}

#elif defined( ANDROID ) || defined( __ANDROID__ )

uuid::uuid()
{
}

#elif defined( __linux__ )

#	include <uuid/uuid.h>

uuid::uuid()
{
	uuid_generate( m_data );
}

#else

uuid::uuid()
{
}

#endif


std::string
uuid::to_string( const char *delim ) const
{
	std::ostringstream os;

	os << std::hex << std::uppercase;

	for ( auto i = 0u; i < sizeof( m_data ); i++ )
	{
		if ( i && ( ( i % 4 ) == 0 ) )
		{
			os << delim;
		}

		os << ( int ) m_data[ i ];
	}

	return os.str();
}


std::string
uuid::to_base64() const
{
	return codec::base64::encode( buffer( m_data, sizeof( m_data ) ) );
}
