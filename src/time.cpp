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

#include <nodeoze/time.h>
#include <string.h>
#include <iostream>
#include <ctime>


std::string
std::to_string( const std::chrono::system_clock::time_point &val )
{
	char		*time_str = nullptr;
	std::time_t	t = std::chrono::system_clock::to_time_t( val );
			
#if defined( WIN32 )
	char time_buf[ 1024 ];
	memset( time_buf, 0, sizeof( time_buf ) );
	ctime_s( time_buf, sizeof( time_buf ), &t );
	time_str	= time_buf;
#else
	time_str = ctime( &t );
#endif
			
	if ( time_str )
	{
		for ( auto i = strlen( time_str ) - 1; i > 0; i-- )
		{
			if ( time_str[ i ] == '\n' )
			{
				time_str[ i ] = '\0';
				break;
			}
		}
	}
	
	return time_str;
}

/*
std::chrono::system_clock::time_point
nodeoze::chrono::now_with_benefits()
{
	static std::time_t last = 0;
	
	auto now = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
	
	if ( now <= last )
	{
		return std::chrono::system_clock::from_time_t( ++last );
	}
	else
	{
		last = now;
		return std::chrono::system_clock::from_time_t( now );
	}
}
*/
