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

buffer
bstream::ibstream::get_msgpack_obj_buf()
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
bstream::ibstream::ingest( bufwriter& os )
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
				os.put( get() ); // val[0]
				os.put( get() ); // val[1]
				return;
			}

			case typecode::fixext_4:
			{
				os.put( get() ); // type
				os.put( get() ); // val[0]
				os.put( get() ); // val[1]
				os.put( get() ); // val[2]
				os.put( get() ); // val[3]
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
