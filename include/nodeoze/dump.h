/*
 * The MIT License
 *
 * Copyright 2017 David Curtis.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* 
 * File:   dump.h
 * Author: David Curtis
 *
 * Created on April 4, 2017, 4:16 PM
 */

#ifndef NODEOZE_DUMP_H
#define NODEOZE_DUMP_H

#include <iostream>
#include <cstdint>
#include <vector>
#include <iomanip>

namespace nodeoze
{

template< class CharT, class Traits = std::char_traits< CharT > > 
class save_format
{
public:
	using char_type = CharT;
	using traits_type = Traits;
	using stream_type = std::basic_ios< char_type, traits_type >;

	explicit
	save_format( stream_type& strm )
	:
	m_stream{ strm },
	m_flags{ strm.flags() },
	m_precision{ strm.precision() },
	m_width{ strm.width() },
	m_fill{ strm.fill() }
	{}

	~save_format()
	{
		m_stream.flags( m_flags );
		m_stream.precision( m_precision );
		m_stream.width( m_width );
		m_stream.fill( m_fill );
	}

private:
	stream_type&				m_stream;
	std::ios_base::fmtflags		m_flags;
	std::streamsize				m_precision;
	std::streamsize				m_width;
	char_type					m_fill;
};


class dumpster
{
public:

	dumpster( std::size_t line_length = 32, std::size_t limit = std::numeric_limits< std::size_t >::max() )
	:
	m_line_length{ line_length },
	m_dump_limit{ limit }
	{}

	// using get_next_byte_func = std::function< bool ( std::uint8_t& byte ) >;

	template< class Functor, class CharT, class Traits >
	void 
	dump( std::basic_ostream< CharT, Traits >& os, std::size_t length, Functor&& get_next_byte )
	{
		save_format<CharT, Traits> sfmt{ os };
		length = std::min( length, m_dump_limit );
		std::vector< std::uint8_t > linebuf( m_line_length, 0 );
		os << std::endl;
		std::size_t remaining = length;
		std::size_t line_index = 0ul;
		while ( remaining > 0 )
		{
			std::size_t line_size = std::min( m_line_length, remaining );
			std::size_t bytes_in_line = 0;
			while ( bytes_in_line < line_size )
			{
				if ( ! std::forward< Functor >( get_next_byte )( linebuf[ bytes_in_line ] ) )
				{
					break;
				}
				++bytes_in_line;				
			}

			if ( bytes_in_line > 0 )
			{
				dump_line( os, line_index, linebuf, bytes_in_line );
			}
			if ( bytes_in_line < line_size )
			{
				break;
			}

			line_index += line_size;
			remaining -= line_size;
		}
	}

	template< class Functor, class CharT, class Traits >
	void 
	dump( std::basic_ostream< CharT, Traits>& os, std::streambuf& src, std::size_t length )
	{
		dump( os, length, [&] ( std::uint8_t& byte )
		{
			bool result = false;
			auto ch = src.sbumpc();
			if ( ch != std::streambuf::traits_type::eof() )
			{
				byte = static_cast< std::uint8_t >( ch );
				result = true;
			}
			return result;
		} );
	}
	
	template< class CharT, class Traits >
	void 
	dump( std::basic_ostream< CharT, Traits >& os, const void * src, std::size_t length )
	{
		std::size_t index = 0;
	    auto base = reinterpret_cast< const std::uint8_t* >( src );
		dump( os, length, [ length, base, &index ] ( std::uint8_t& byte )
		{
			bool result = true;
			if ( index < length )
			{
				byte = base[ index++ ];
			}
			else
			{
				result = false;
			}
			return result;
		} );
	}

private:

	template< class CharT, class Traits >
	void 
	dump_line( std::basic_ostream< CharT, Traits >& os, std::size_t line_index, std::vector< uint8_t >& linebuf, std::size_t line_size )
	{

		os << std::hex << std::setfill( '0' );
		os << std::setw(8) << line_index << ": ";

		for ( auto i = 0ul; i < line_size; ++i )
		{
			auto byte = linebuf[ i ];
			os << std::setw(2) << (unsigned) byte << ' ';
		}

		if ( line_size < m_line_length )
		{
			auto fill = m_line_length - line_size;
			for ( auto i = 0u; i < fill; ++i )
			{
				os << "   ";
			}
		}

		os << "    ";
		for ( auto i = 0ul; i < line_size; ++i )
		{
			auto byte = linebuf[ i ];
			if ( isprint( byte ) )
			{
				os << (char)byte;
			}
			else
			{
				os << '.';
			}
		}
		os << std::endl;
	}

	std::size_t		m_line_length;
	std::size_t		m_dump_limit;
};

} // nodeoze

#endif // NODEOZE_DUMP_H
