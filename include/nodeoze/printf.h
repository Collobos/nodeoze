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

#ifndef _nodeoze_printf_h
#define _nodeoze_printf_h

#include <nodeoze/time.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace nodeoze {

template<typename char_type, typename traits = std::char_traits<char_type>>
void
printf( std::basic_ostream<char_type, traits> &output, const char_type *format )
{
	while ( *format )
	{
		if ( *format == '%' )
		{
			if ( *( format + 1 ) == '%' )
			{
                ++format;
            }
            else
			{
                throw std::runtime_error( "invalid format string: missing arguments" );
            }
        }

        output << *format++;
    }
}


template<typename char_type, typename traits = std::char_traits<char_type>, typename Head, typename ...Tail>
void
printf( std::basic_ostream<char_type, traits> &output, const char_type *format, const Head &head, const Tail &... tail)
{
	while ( *format )
	{
		if ( *format == '%' )
		{
			if ( *( format + 1 ) == '%' )
			{
				++format;
			}
			else
			{
				output << head;
				nodeoze::printf( output, format + 1, tail... );
				return;
			}
		}

		output << *format++;
	}

	throw std::logic_error( "extra arguments provided to printf" );
}

}

#endif
