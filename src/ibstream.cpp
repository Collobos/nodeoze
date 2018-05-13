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

#if 0

/*
 * Macros to verify buffer contents at 
 * representational boundaries for integer 
 * numeric types
 */

#define CHECK_0(ostrm)							\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0);							\
	ostrm.clear();								\
}												\
/**/

#define CHECK_neg_fixint_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_neg_fixint_min(ostrm)				\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0xe0);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_neg_fixint_min_minus_1(ostrm)		\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd0);						\
	byte = is.get();							\
	CHECK(byte == 0xdf);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_pos_fixint_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0x7f);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_pos_fixint_max_plus_1(ostrm)		\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcc);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_8_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd0);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_8_min_minus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd1);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0x7f);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_8_max(ostrm)					\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcc);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_8_max_plus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcd);						\
	byte = is.get();							\
	CHECK(byte == 0x01);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_16_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd1);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_16_min_minus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd2);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0x7f);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_16_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcd);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_16_max_plus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xce);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x01);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_32_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd2);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_32_min_minus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd3);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0x7f);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_32_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xce);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_32_max_plus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcf);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x01);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_64_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd3);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_64_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm);				\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcf);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

TEST_CASE("nodeoze/smoke/numeric_representation")
{
	bstream::obstream os{1024};
	
	os << (std::uint8_t)0;
	CHECK_0(os);
	os << (std::int8_t)0;
	CHECK_0(os);
	os << (std::uint16_t)0;
	CHECK_0(os);
	os << (std::int16_t)0;
	CHECK_0(os);
	os << (std::uint32_t)0;
	CHECK_0(os);
	os << (std::int32_t)0;
	CHECK_0(os);
	os << (std::uint64_t)0;
	CHECK_0(os);
	os << (std::int64_t)0;
	CHECK_0(os);
	
	os << (std::int8_t)-1;
	CHECK_neg_fixint_max(os);
	os << (std::int16_t)-1;
	CHECK_neg_fixint_max(os);
	os << (std::int32_t)-1;
	CHECK_neg_fixint_max(os);
	os << (std::int64_t)-1;
	CHECK_neg_fixint_max(os);
	
	os << (std::int8_t)-32;
	CHECK_neg_fixint_min(os);
	os << (std::int16_t)-32;
	CHECK_neg_fixint_min(os);
	os << (std::int32_t)-32;
	CHECK_neg_fixint_min(os);
	os << (std::int64_t)-32;
	CHECK_neg_fixint_min(os);
	
	os << (std::int8_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	os << (std::int16_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	os << (std::int32_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	os << (std::int64_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	
	os.clear();
	os << (std::uint8_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int8_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::uint16_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int16_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::uint32_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int32_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::uint64_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int64_t)127;
	CHECK_pos_fixint_max(os);

	os << (std::uint8_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::uint16_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::int16_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::uint32_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::int32_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::uint64_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::int64_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	
	os << (std::int8_t)-128;
	CHECK_int_8_min(os);
	os << (std::int16_t)-128;
	CHECK_int_8_min(os);
	os << (std::int32_t)-128;
	CHECK_int_8_min(os);
	os << (std::int64_t)-128;
	CHECK_int_8_min(os);
	
	os << (std::int16_t)-129;
	CHECK_int_8_min_minus_1(os);
	os << (std::int32_t)-129;
	CHECK_int_8_min_minus_1(os);
	os << (std::int64_t)-129;
	CHECK_int_8_min_minus_1(os);
	
	os << (std::uint8_t)255;
	CHECK_uint_8_max(os);	
	os << (std::uint16_t)255;
	CHECK_uint_8_max(os);	
	os << (std::int16_t)255;
	CHECK_uint_8_max(os);	
	os << (std::uint32_t)255;
	CHECK_uint_8_max(os);	
	os << (std::int32_t)255;
	CHECK_uint_8_max(os);	
	os << (std::uint64_t)255;
	CHECK_uint_8_max(os);	
	os << (std::int64_t)255;
	CHECK_uint_8_max(os);	
	
	os << (std::uint16_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::int16_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::uint32_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::int32_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::uint64_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::int64_t)256;
	CHECK_uint_8_max_plus_1(os);
	
	os << (std::int16_t)-32768;
	CHECK_int_16_min(os);
	os << (std::int32_t)-32768;
	CHECK_int_16_min(os);
	os << (std::int64_t)-32768;
	CHECK_int_16_min(os);
	
	os << (std::int32_t)-32769;
	CHECK_int_16_min_minus_1(os);
	os << (std::int64_t)-32769;
	CHECK_int_16_min_minus_1(os);
	
	os << (std::uint16_t)65535;
	CHECK_uint_16_max(os);
	os << (std::uint32_t)65535;
	CHECK_uint_16_max(os);
	os << (std::int32_t)65535;
	CHECK_uint_16_max(os);
	os << (std::uint64_t)65535;
	CHECK_uint_16_max(os);
	os << (std::int64_t)65535;
	CHECK_uint_16_max(os);
	
	os << (std::uint32_t)65536;
	CHECK_uint_16_max_plus_1(os);	
	os << (std::int32_t)65536;
	CHECK_uint_16_max_plus_1(os);	
	os << (std::uint64_t)65536;
	CHECK_uint_16_max_plus_1(os);	
	os << (std::int64_t)65536;
	CHECK_uint_16_max_plus_1(os);	

	os << (std::int32_t)-2147483648;
	CHECK_int_32_min(os);
	os << (std::int64_t)-2147483648;
	CHECK_int_32_min(os);
	
	os << (std::int64_t)-2147483649;
	CHECK_int_32_min_minus_1(os);
	
	os << (std::uint32_t)4294967295;
	CHECK_uint_32_max(os);
	os << (std::uint64_t)4294967295;
	CHECK_uint_32_max(os);
	os << (std::int64_t)4294967295;
	CHECK_uint_32_max(os);

	os << (std::uint64_t)4294967296;
	CHECK_uint_32_max_plus_1(os);
	os << (std::int64_t)4294967296;
	CHECK_uint_32_max_plus_1(os);

	os << (std::int64_t)std::numeric_limits<std::int64_t>::min();
	CHECK_int_64_min(os);

	os << (std::uint64_t)18446744073709551615ull;
	CHECK_uint_64_max(os);
}

#define WRITE_READ_TEST(type, value)			\
{												\
	bstream::obstream os{1024};					\
	type v0 = value;							\
	os << v0;									\
	bstream::ibstream is{os};					\
	type v1;									\
	is >> v1;									\
	CHECK(v1 == v0);							\
	is.rewind();								\
	auto v2 = is.read_as<type>();				\
	CHECK(v2 == v0);							\
}												\

#define READ_TYPE_ERROR_TEST(type, value)		\
{												\
	bstream::obstream os{1024};					\
	os << value;								\
	bstream::ibstream is{os};					\
	try {										\
		type v;									\
		is >> v;								\
		CHECK(false);							\
	} catch (bstream::type_error const& e) {	\
		CHECK(true);							\
	}											\
}												\
/**/

TEST_CASE("nodeoze/smoke/numeric_write_read")
{
	WRITE_READ_TEST(std::int8_t, 0);
	WRITE_READ_TEST(std::uint8_t, 0);
	WRITE_READ_TEST(std::int16_t, 0);
	WRITE_READ_TEST(std::uint16_t, 0);
	WRITE_READ_TEST(std::int32_t, 0);
	WRITE_READ_TEST(std::uint32_t, 0);
	WRITE_READ_TEST(std::int64_t, 0);
	WRITE_READ_TEST(std::uint64_t, 0);

	WRITE_READ_TEST(std::int8_t, 127);
	WRITE_READ_TEST(std::uint8_t, 127);
	WRITE_READ_TEST(std::int16_t, 127);
	WRITE_READ_TEST(std::uint16_t, 127);
	WRITE_READ_TEST(std::int32_t, 127);
	WRITE_READ_TEST(std::uint32_t, 127);
	WRITE_READ_TEST(std::int64_t, 127);
	WRITE_READ_TEST(std::uint64_t, 127);
	
	READ_TYPE_ERROR_TEST(std::int8_t, 128);
	
	WRITE_READ_TEST(std::uint8_t, 128);
	WRITE_READ_TEST(std::int16_t, 128);
	WRITE_READ_TEST(std::uint16_t, 128);
	WRITE_READ_TEST(std::int32_t, 128);
	WRITE_READ_TEST(std::uint32_t, 128);
	WRITE_READ_TEST(std::int64_t, 128);
	WRITE_READ_TEST(std::uint64_t, 128);

	WRITE_READ_TEST(std::uint8_t, 255);
	WRITE_READ_TEST(std::int16_t, 255);
	WRITE_READ_TEST(std::uint16_t, 255);
	WRITE_READ_TEST(std::int32_t, 255);
	WRITE_READ_TEST(std::uint32_t, 255);
	WRITE_READ_TEST(std::int64_t, 255);
	WRITE_READ_TEST(std::uint64_t, 255);

	READ_TYPE_ERROR_TEST(std::uint8_t, 256);
	
	WRITE_READ_TEST(std::int16_t, 256);
	WRITE_READ_TEST(std::uint16_t, 256);
	WRITE_READ_TEST(std::int32_t, 256);
	WRITE_READ_TEST(std::uint32_t, 256);
	WRITE_READ_TEST(std::int64_t, 256);
	WRITE_READ_TEST(std::uint64_t, 256);

	WRITE_READ_TEST(std::int16_t, 32767);
	WRITE_READ_TEST(std::uint16_t, 32767);
	WRITE_READ_TEST(std::int32_t, 32767);
	WRITE_READ_TEST(std::uint32_t, 32767);
	WRITE_READ_TEST(std::int64_t, 32767);
	WRITE_READ_TEST(std::uint64_t, 32767);

	READ_TYPE_ERROR_TEST(std::int16_t, 32768);
	
	WRITE_READ_TEST(std::uint16_t, 32768);
	WRITE_READ_TEST(std::int32_t, 32768);
	WRITE_READ_TEST(std::uint32_t, 32768);
	WRITE_READ_TEST(std::int64_t, 32768);
	WRITE_READ_TEST(std::uint64_t, 32768);

	WRITE_READ_TEST(std::uint16_t, 65535);
	WRITE_READ_TEST(std::int32_t, 65535);
	WRITE_READ_TEST(std::uint32_t, 65535);
	WRITE_READ_TEST(std::int64_t, 65535);
	WRITE_READ_TEST(std::uint64_t, 65535);

	READ_TYPE_ERROR_TEST(std::uint16_t, 65536);
	
	WRITE_READ_TEST(std::int32_t, 65536);
	WRITE_READ_TEST(std::uint32_t, 65536);
	WRITE_READ_TEST(std::int64_t, 65536);
	WRITE_READ_TEST(std::uint64_t, 65536);

	WRITE_READ_TEST(std::int32_t, 2147483647);
	WRITE_READ_TEST(std::uint32_t, 2147483647);
	WRITE_READ_TEST(std::int64_t, 2147483647);
	WRITE_READ_TEST(std::uint64_t, 2147483647);

	READ_TYPE_ERROR_TEST(std::int32_t, 2147483648);
	
	WRITE_READ_TEST(std::uint32_t, 2147483648);
	WRITE_READ_TEST(std::int64_t, 2147483648);
	WRITE_READ_TEST(std::uint64_t, 2147483648);

	WRITE_READ_TEST(std::uint32_t, 4294967295);
	WRITE_READ_TEST(std::int64_t, 4294967295);
	WRITE_READ_TEST(std::uint64_t, 4294967295);

	READ_TYPE_ERROR_TEST(std::uint32_t, 4294967296LL);
	
	WRITE_READ_TEST(std::int64_t, 4294967296LL);
	WRITE_READ_TEST(std::uint64_t, 4294967296ULL);

	WRITE_READ_TEST(std::int64_t, 9223372036854775807ULL);
	WRITE_READ_TEST(std::uint64_t, 9223372036854775807ULL);

	READ_TYPE_ERROR_TEST(std::int64_t, 9223372036854775808ULL);
	
	WRITE_READ_TEST(std::uint64_t, 9223372036854775808ULL);
	
	WRITE_READ_TEST(std::uint64_t, 18446744073709551615ULL);
	
	WRITE_READ_TEST(std::int8_t, -1);
	WRITE_READ_TEST(std::int16_t, -1);
	WRITE_READ_TEST(std::int32_t, -1);
	WRITE_READ_TEST(std::int64_t, -1);

	WRITE_READ_TEST(std::int8_t, -32);
	WRITE_READ_TEST(std::int16_t, -32);
	WRITE_READ_TEST(std::int32_t, -32);
	WRITE_READ_TEST(std::int64_t, -32);

	WRITE_READ_TEST(std::int8_t, -33);
	WRITE_READ_TEST(std::int16_t, -33);
	WRITE_READ_TEST(std::int32_t, -33);
	WRITE_READ_TEST(std::int64_t, -33);

	WRITE_READ_TEST(std::int8_t, -128);
	WRITE_READ_TEST(std::int16_t, -128);
	WRITE_READ_TEST(std::int32_t, -128);
	WRITE_READ_TEST(std::int64_t, -128);

	READ_TYPE_ERROR_TEST(std::int8_t, -129);
	
	WRITE_READ_TEST(std::int16_t, -129);
	WRITE_READ_TEST(std::int32_t, -129);
	WRITE_READ_TEST(std::int64_t, -129);

	WRITE_READ_TEST(std::int16_t, -32768);
	WRITE_READ_TEST(std::int32_t, -32768);
	WRITE_READ_TEST(std::int64_t, -32768);

	READ_TYPE_ERROR_TEST(std::int16_t, -32769);
	
	WRITE_READ_TEST(std::int32_t, -32769);
	WRITE_READ_TEST(std::int64_t, -32769);

	WRITE_READ_TEST(std::int32_t, -2147483648);
	WRITE_READ_TEST(std::int64_t, -2147483648);

	READ_TYPE_ERROR_TEST(std::int32_t, -2147483649LL);
	
	WRITE_READ_TEST(std::int64_t, -2147483649LL);

	WRITE_READ_TEST(std::int64_t, std::numeric_limits<std::int64_t>::min());
}

#endif

