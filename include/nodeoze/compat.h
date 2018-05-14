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

#ifndef _nodeoze_compat_h
#define _nodeoze_compat_h

#if defined( WIN32 )

#	include <winsock2.h>
#	include <windows.h>

#else

#	include <sys/socket.h>
#	include <sys/types.h>
#	include <stdlib.h>
#	include <unistd.h>
#	include <errno.h>

#	if defined( ANDROID ) || defined( __ANDROID__ )

#		include <time.h>
#		include <sstream>
#		include <memory>
#		include <type_traits>
#		include <utility>


inline std::int32_t
is_leap( std::int32_t year )
{
	if ( year % 400 == 0 )
	{
		return 1;
	}

	if (year % 100 == 0 )
	{
		return 0;
	}

	if ( year % 4 == 0 )
	{
		return 1;
	}

	return 0;
}


inline std::int32_t
days_from_0( std::int32_t year )
{
	year--;
	return 365 * year + (year / 400) - (year/100) + (year / 4);
}


inline std::int32_t
days_from_1970( std::int32_t year )
{
	static const int days_from_0_to_1970 = days_from_0(1970);
	return days_from_0(year) - days_from_0_to_1970;
}


inline std::int32_t
days_from_1jan( std::int32_t year, std::int32_t month, std::int32_t day )
{
	static const std::int32_t days[2][12] =
	{
		{ 0,31,59,90,120,151,181,212,243,273,304,334 },
		{ 0,31,60,91,121,152,182,213,244,274,305,335 }
	};

	return days[ is_leap( year ) ][ month - 1 ] + day - 1;
}


inline time_t
timegm( struct tm * const t )
{
	int year	= t->tm_year + 1900;
	int month	= t->tm_mon;

	if ( month > 11 )
	{
		year += month / 12;
		month %= 12;
	}
	else if ( month < 0 )
	{
		int years_diff  = ( -month + 11 ) / 12;
		year			-= years_diff;
		month			+= 12 * years_diff;
	}

	month++;

	int day					= t->tm_mday;
	int day_of_year			= days_from_1jan(year,month,day);
	int days_since_epoch	= days_from_1970(year) + day_of_year;

	time_t seconds_in_day	= 3600 * 24;
	time_t result			= seconds_in_day * days_since_epoch + 3600 * t->tm_hour + 60 * t->tm_min + t->tm_sec;

	return result;
}

namespace std {

template <typename T, typename... Args>
inline unique_ptr<T>
make_unique_helper(std::false_type, Args&&... args)
{
	return unique_ptr<T>(new T( forward<Args>(args)...));
}

template <typename T, typename... Args>
inline unique_ptr<T>
make_unique_helper( true_type, Args&&... args)
{
	static_assert( extent<T>::value == 0,
		"make_unique<T[N]>() is forbidden, please use make_unique<T[]>().");

	typedef typename remove_extent<T>::type U;
	return unique_ptr<T>(new U[sizeof...(Args)]{ forward<Args>(args)... });
}

template <typename T, typename... Args>
inline unique_ptr<T>
make_unique(Args&&... args)
{
	return make_unique_helper<T>( is_array<T>(), forward<Args>(args)...);
}

}

#	endif
#endif

#endif
