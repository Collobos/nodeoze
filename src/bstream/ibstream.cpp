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

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <nodeoze/test.h>

using namespace nodeoze;
using namespace bstream;

buffer
ibstream::get_msgpack_obj_buf()
{
	// allocate bufwriter at most once
	if ( ! m_bufwriter )
	{
		m_bufwriter = std::make_unique< bufwriter >( 16 * 1024 ); // whatever...
	}

	m_bufwriter->reset();
	ingest( *m_bufwriter );
	return m_bufwriter->get_buffer();
}

void 
ibstream::ingest( bufwriter& os )
{
	auto tcode = base::get();
	os.put( tcode );

	if ( ( tcode <= typecode::positive_fixint_max ) ||
		( tcode >= typecode::negative_fixint_min && tcode <= typecode::negative_fixint_max ) )
	{
		return;
	}
	else if ( tcode <= typecode::fixmap_max )
	{
		std::size_t len = tcode & 0x0f;
		for ( auto i = 0u; i < len * 2; ++i )
		{
			ingest( os );
		}
		return;
	}
	else if ( tcode <= typecode::fixarray_max )
	{
		std::size_t len = tcode & 0x0f;
		for ( auto i = 0u; i < len; ++i )
		{
			ingest( os );
		}
		return;
	}
	else if ( tcode <= typecode::fixstr_max )
	{
		std::size_t len = tcode & 0x1f;
		getn( os.accommodate( len ), len );
		os.advance( len );
		return;
	}
	else
	{
		switch ( tcode )
		{
			case typecode::array_16:
			{
				std::uint16_t len = get_num< std::uint16_t >();
				std::uint16_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				for ( auto i = 0u; i < len; ++i )
				{
					ingest( os );
				}
				return;
			}
					
			case typecode::array_32:
			{
				std::uint32_t len = get_num< std::uint32_t >();
				std::uint32_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				for ( auto i = 0u; i < len; ++i )
				{
					ingest( os );
				}
				return;
			}
					
			case typecode::map_16:
			{
				std::uint16_t len = get_num< std::uint16_t >();
				std::uint16_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				for ( auto i = 0u; i < len * 2; ++i )
				{
					ingest( os );
				}
				return;
			}
					
			case typecode::map_32:
			{
				std::uint32_t len = get_num< std::uint32_t >();
				std::uint32_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				for ( auto i = 0u; i < len * 2; ++i )
				{
					ingest( os );
				}
				return;
			}
					
			case typecode::nil:
			case typecode::unused:
			case typecode::bool_false:
			case typecode::bool_true:
			{
				return;
			}

			case typecode::uint_8:
			case typecode::int_8:
			{
				os.put( get() );
				return;
			}

			case typecode::uint_16:
			case typecode::int_16:
			{
				os.put( get() );
				os.put( get() );
				return;
			}

			case typecode::uint_32:
			case typecode::int_32:
			case typecode::float_32:
			{
				os.put( get() );
				os.put( get() );
				os.put( get() );
				os.put( get() );
				return;
			}

			case typecode::uint_64:
			case typecode::int_64:
			case typecode::float_64:
			{
				getn( os.accommodate( 8 ), 8 );
				os.advance( 8 );
				return;
			}

			case typecode::str_8:
			case typecode::bin_8:
			{
				auto len = get_num< std::uint8_t >();
				os.put( len );
				getn( os.accommodate( len ), len );
				os.advance( len );
				return;
			}

			case typecode::str_16:
			case typecode::bin_16:
			{
				std::uint16_t len = get_num< std::uint16_t >();
				std::uint16_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				getn( os.accommodate( len ), len );
				os.advance( len );
				return;
			}

			case typecode::str_32:
			case typecode::bin_32:
			{
				std::uint32_t len = get_num< std::uint32_t >();
				std::uint32_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				getn( os.accommodate( len ), len );
				os.advance( len );
				return;
			}

			case typecode::fixext_1:
			{
				os.put( get() ); // type
				os.put( get() ); // val
				return;
			}

			case typecode::fixext_2:
			{
				os.put( get() ); // type
				os.put( get() ); // val[0 ]
				os.put( get() ); // val[1 ]
				return;
			}

			case typecode::fixext_4:
			{
				os.put( get() ); // type
				os.put( get() ); // val[0 ]
				os.put( get() ); // val[1 ]
				os.put( get() ); // val[2 ]
				os.put( get() ); // val[3 ]
				return;
			}

			case typecode::fixext_8:
			{
				getn( os.accommodate( 1 + 8 ), 1 + 8 );
				os.advance( 1 + 8 );
				return;
			}

			case typecode::fixext_16:
			{
				getn( os.accommodate( 1 + 16 ), 1 + 16 );
				os.advance( 1 + 16 );
				return;
			}

			case typecode::ext_8:
			{
				auto len = get_num< std::uint8_t >();
				os.put( len );
				os.put( get() ); // type
				getn( os.accommodate( len ), len );
				os.advance( len );
				return;
			}

			case typecode::ext_16:
			{
				std::uint16_t len = get_num< std::uint16_t >();
				std::uint16_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				os.put( get() ); // type
				getn( os.accommodate( len ), len );
				os.advance( len );
				return;
			}

			case typecode::ext_32:
			{
				std::uint32_t len = get_num< std::uint32_t >();
				std::uint32_t len_bigend = bend::endian_reverse( len );
				os.putn( &len_bigend, sizeof( len_bigend ) );
				os.put( get() ); // type
				getn( os.accommodate( len ), len );
				os.advance( len );
				return;
			}

		}
	}
}


std::size_t
ibstream::read_string_header()
{
	std::size_t length = 0ul;
	auto tcode = base::get();
	if (tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max )
	{
		std::uint8_t mask = 0x1f;
		length = tcode & mask;
	}
	else
	{
		switch(tcode )
		{
			case typecode::str_8:
			{
				length = base::get_num< std::uint8_t >();
			}
			break;
			case typecode::str_16:
			{
				length = base::get_num< std::uint16_t >();
			}
			break;
			case typecode::str_32:
			{
				length = base::get_num< std::uint32_t >();
			}
			break;
			default:
			{
				throw std::system_error{ make_error_code( bstream::errc::type_error ) };
			}
		}
	}
	return length;
}

std::size_t
ibstream::read_string_header( std::error_code& ec )
{
	clear_error( ec );
	std::size_t result = 0ul;
	try
	{
		result = read_string_header();
	}
	catch ( std::system_error const& e )
	{
		ec = e.code();
	}
	return result;
}

std::size_t 
ibstream::read_array_header()
{
	std::size_t length = 0;
	auto tcode = base::get();
	if (tcode >= typecode::fixarray_min && tcode <= typecode::fixarray_max )
	{
		std::uint8_t mask = 0x0f;
		length = tcode & mask;
	}
	else
	{
		switch (tcode )
		{
			case typecode::array_16:
			{
				length = base::get_num< std::uint16_t >();
			}
			break;
			case typecode::array_32:
			{
				length = base::get_num< std::uint32_t >();
			}
			break;
			default:
			{
				throw std::system_error{ make_error_code( bstream::errc::type_error ) };
			}
		}
	}
	return length;
}

std::size_t 
ibstream::read_array_header( std::error_code& ec )
{
	clear_error( ec );
	std::size_t result = 0;
	try
	{
		result = read_array_header();
	}
	catch ( std::system_error const& e )
	{
		ec = e.code();
	}
	return result;
}

std::size_t
ibstream::read_map_header()
{
	std::size_t length = 0;
	auto tcode = base::get();
	if (tcode >= typecode::fixmap_min && tcode <= typecode::fixarray_max )
	{
		std::uint8_t mask = 0x0f;
		length = tcode & mask;
	}
	else
	{
		switch (tcode )
		{
		case typecode::map_16:
			{
				length = base::get_num< std::uint16_t >();
			}
			break;
			case typecode::map_32:
			{
				length = base::get_num< std::uint32_t >();
			}
			break;
			default:
			{
				throw std::system_error{ make_error_code( bstream::errc::type_error ) };
			}
		}
	}
	return length;
}

std::size_t
ibstream::read_map_header( std::error_code& ec )
{
	clear_error( ec );
	std::size_t result = 0;
	try
	{
		result = read_map_header();
	}
	catch ( std::system_error const& e )
	{
		ec = e.code();
	}
	return result;
}

ibstream&
ibstream::check_map_key(std::string const& key )
{
	auto name = read_as< std::string >();
	if (name != key )
	{
		throw std::system_error{ make_error_code( bstream::errc::type_error ) };
	}
	return *this;
}

ibstream&
ibstream::check_map_key(std::string const& key, std::error_code& ec )
{
	clear_error( ec );
	auto name = read_as< std::string >();
	if (name != key )
	{
		ec = make_error_code( bstream::errc::type_error );
	}
	return *this;
}

ibstream&
ibstream::check_array_header(std::size_t expected )
{
	auto actual = read_array_header();
	if (actual != expected )
	{
		throw std::system_error{ make_error_code( bstream::errc::type_error ) };
	}
	return *this;
}

ibstream&
ibstream::check_array_header(std::size_t expected, std::error_code& ec )
{
	clear_error( ec );
	auto actual = read_array_header();
	if (actual != expected )
	{
		ec = make_error_code( bstream::errc::type_error );
	}
	return *this;
}

ibstream&
ibstream::check_map_header(std::size_t expected )
{
	auto actual = read_map_header();
	if (actual != expected )
	{
		throw std::system_error{ make_error_code( bstream::errc::type_error ) };
	}
	return *this;
}

ibstream&
ibstream::check_map_header(std::size_t expected, std::error_code& ec )
{
	clear_error( ec );
	auto actual = read_map_header();
	if (actual != expected )
	{
		ec = make_error_code( bstream::errc::type_error );
	}
	return *this;
}

std::size_t 
ibstream::read_blob_header()
{
	std::size_t length = 0;
	auto tcode = base::get();
	switch (tcode )
	{
		case typecode::bin_8:
		{
			length = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::bin_16:
		{
			length = base::get_num< std::uint16_t >();
		}
		break;
		case typecode::bin_32:
		{
			length = base::get_num< std::uint32_t >();
		}
		break;
		default:
		{
			throw std::system_error{ make_error_code( bstream::errc::type_error ) };
		}
	}
	return length;
}

std::size_t 
ibstream::read_blob_header( std::error_code& ec )
{
	clear_error( ec );
	std::size_t result = 0;
	try
	{
		result = read_blob_header();
	}
	catch ( std::system_error const& e )
	{
		ec = e.code();
	}
	return result;
}

nodeoze::buffer
ibstream::read_blob( std::error_code& ec )
{
	auto nbytes = read_blob_header( ec );
	if ( ec )
	{
		return buffer{};
	}
	else
	{
		return read_blob_body( nbytes, ec );
	}
}

std::size_t
ibstream::read_ext_header( std::uint8_t& ext_type )
{
	std::size_t length = 0;
	auto tcode = base::get();
	switch (tcode )
	{
		case typecode::fixext_1:
		{
			length = 1;
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::fixext_2:
		{
			length = 2;
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::fixext_4:
		{
			length = 4;
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::fixext_8:
		{
			length = 8;
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::fixext_16:
		{
			length = 16;
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::ext_8:
		{
			length = base::get_num< std::uint8_t >();
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::ext_16:
		{
			length = base::get_num< std::uint16_t >();
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		case typecode::ext_32:
		{
			length = base::get_num< std::uint32_t >();
			ext_type = base::get_num< std::uint8_t >();
		}
		break;
		default:
		{
			throw std::system_error{ make_error_code( bstream::errc::type_error ) };
		}
	}
	return length;
}

std::size_t
ibstream::read_ext_header( std::uint8_t& ext_type, std::error_code& ec )
{
	clear_error( ec );
	std::size_t result = 0;
	try
	{
		result = read_ext_header( ext_type );
	}
	catch ( std::system_error const& e )
	{
		ec = e.code();
	}
	return result;
}

std::error_code
ibstream::read_error_code()
{
	auto n = read_array_header();
	if ( n != 2 )
	{
		throw std::system_error{ make_error_code( bstream::errc::type_error ) };
	}
	auto category_index = read_as< error_category_context::index_type >();
	auto value = read_as< error_category_context::index_type >();
	return std::error_code{ value, m_context->category_from_index( category_index ) };
}

