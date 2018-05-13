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
 
#include <nodeoze/unicode.h>
#include <nodeoze/macros.h>
#if defined( WIN32 )
#	include <WinSock2.h>
#	include <Windows.h>
#else
#endif

using namespace nodeoze;

std::string
nodeoze::narrow( const wchar_t *s )
{
	std::string ret;

#if defined( WIN32 )

	int n;

    n = WideCharToMultiByte( CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL );

    if ( n > 0 )
    {
        char *utf8;

        try
        {
            utf8 = new char[ n ];
        }
        catch ( ... )
        {
            utf8 = NULL;
        }

        if ( utf8 )
        {
            n = WideCharToMultiByte( CP_UTF8, 0, s, -1, utf8, n, NULL, NULL );

            if ( n > 0 )
            {
                ret = utf8;
            }

            delete [] utf8;
        }
    }

#else

	nunused( s );
	
#endif

    return ret;
}


std::string
nodeoze::narrow( const std::wstring &s )
{
	return narrow( s.c_str() );
}


std::wstring
nodeoze::widen( const char *s )
{
	std::wstring ret;

#if defined( WIN32 )

    int n;

    n = MultiByteToWideChar( CP_UTF8, 0, s, -1, NULL, 0 );

    if ( n > 0 )
    {
        wchar_t *utf16;

        try
        {
            utf16 = new wchar_t[ n ];
        }
        catch ( ... )
        {
            utf16 = NULL;
        }

        if ( utf16 )
        {
            n = MultiByteToWideChar( CP_UTF8, 0, s, -1, utf16, n );

            if ( n > 0 )
            {
                try
                {
                    ret = utf16;
                }
                catch( ... )
                {
                }
            }

            delete [] utf16;
        }
    }

#else

	nunused( s );

#endif

    return ret;
}


std::wstring
nodeoze::widen( const std::string &s )
{
	return widen( s.c_str() );
}
