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
 
#ifndef _nodeoze_string_h
#define _nodeoze_string_h

#include <nodeoze/nunicode.h>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#if defined( WIN32 )
#	include <tchar.h>
#endif

namespace std {

#if defined( WIN32 )

typedef TCHAR tchar_t;

#else

typedef char tchar_t;

#endif

inline void
memmove_s( void *s1, size_t size, const void *s2, size_t n )
{
#if defined( WIN32 )

	::memmove_s( s1, size, s2, n );

#else

	( void ) size;

	::memmove( s1, s2, n );

#endif
}

inline int
strncasecmp( const char *buf1, const char *buf2, size_t len )
{
#if defined( WIN32 )

	return _strnicmp( buf1, buf2, len );

#else

	return ::strncasecmp( buf1, buf2, len );
	
#endif
}

inline int
sprintf_s( char *buffer, size_t buflen, size_t count, const char *format, ... )
{
	va_list argList;
	
	va_start( argList, format );
	
#if defined( WIN32 )

	int ret = _vsnprintf_s( buffer, buflen, count, format, argList );

#else

	( void )( buflen );
	( void )( count );

	int ret = vsprintf( buffer, format, argList );
	
#endif

	va_end( argList );
	
	return ret;
}

}

namespace nodeoze {

namespace string {

inline bool
case_insensitive_compare( const std::string &a, const std::string &b )
{
	return ( a.size() == b.size() ) && std::equal( a.begin(), a.end(), b.begin(), []( char cA, char cB )
	{
		return tolower(cA) == tolower(cB);
	} );
}

struct case_insensitive_less : std::binary_function< std::string, std::string, bool >
{
	struct nocase_compare : public std::binary_function<unsigned char,unsigned char,bool>
	{
		bool operator() (const unsigned char &a, const unsigned char &b ) const
		{
			return tolower( a ) < tolower( b );
		}
	};
	
	bool
	operator()( const std::string & s1, const std::string & s2) const
	{
		return std::lexicographical_compare( s1.begin (), s1.end(), s2.begin(), s2.end(), nocase_compare() );
	}
};


inline void
find_and_replace_in_place( std::string &subject, const std::string &search, const std::string &replace )
{
    size_t pos = 0;
	
    while ( ( pos = subject.find( search, pos ) ) != std::string::npos )
	{
         subject.replace( pos, search.length(), replace );
         pos += replace.length();
    }
}


inline std::string
find_and_replace( const std::string& subject, const std::string& search, const std::string& replace )
{
	std::string tmp( subject );
	find_and_replace_in_place( tmp, search, replace );
	return tmp;
}


template< class T = std::vector< std::string > >
inline T
split( const std::string_view &s, const char delimiter = '/' )
{
	std::string		tmp( s );
	std::size_t		pos = 0;
	T				ret;

	while ( ( pos = tmp.find( delimiter ) ) != std::string::npos )
	{
		auto token = tmp.substr( 0, pos );

		if ( token.size() > 0 )
		{
			ret.emplace_back( std::move( token ) );
		}

		tmp.erase( 0, pos + 1 );
	}
		
	if ( tmp.size() > 0 )
	{
		ret.emplace_back( std::move( tmp ) );
	}

	return ret;
}

}

}

#endif
