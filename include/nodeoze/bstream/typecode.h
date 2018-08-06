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
 * File:   typecode.h
 * Author: David Curtis
 *
 * Created on June 23, 2017, 9:09 PM
 */

#ifndef NODEOZE_BSTREAM_TYPECODE_H
#define NODEOZE_BSTREAM_TYPECODE_H

#include <cstdint>

namespace nodeoze
{
namespace bstream
{
struct typecode
{
	using type = std::uint8_t;

	static constexpr int max_positive_fixint = 127;
	static constexpr int min_negative_fixint =  -32;
		
	static constexpr type positive_fixint_min = 0x00;
	static constexpr type positive_fixint_max = 0x7f;
	static constexpr type fixmap_min = 0x80;
	static constexpr type fixmap_max = 0x8f;
	static constexpr type fixarray_min = 0x90;
	static constexpr type fixarray_max = 0x9f;
	static constexpr type fixstr_min = 0xa0;
	static constexpr type fixstr_max = 0xbf;
	static constexpr type nil = 0xc0;
	static constexpr type unused = 0xc1;
	static constexpr type bool_false = 0xc2;
	static constexpr type bool_true = 0xc3;
	static constexpr type bin_8 = 0xc4;
	static constexpr type bin_16 = 0xc5;
	static constexpr type bin_32 = 0xc6;
	static constexpr type ext_8 = 0xc7;
	static constexpr type ext_16 = 0xc8;
	static constexpr type ext_32 = 0xc9;
	static constexpr type float_32 = 0xca;
	static constexpr type float_64 = 0xcb;
	static constexpr type uint_8 = 0xcc;
	static constexpr type uint_16 = 0xcd;
	static constexpr type uint_32 = 0xce;
	static constexpr type uint_64 = 0xcf;
	static constexpr type int_8 = 0xd0;
	static constexpr type int_16 = 0xd1;
	static constexpr type int_32 = 0xd2;
	static constexpr type int_64 = 0xd3;
	static constexpr type fixext_1 = 0xd4;
	static constexpr type fixext_2 = 0xd5;
	static constexpr type fixext_4 = 0xd6;
	static constexpr type fixext_8 = 0xd7;
	static constexpr type fixext_16 = 0xd8;
	static constexpr type str_8 = 0xd9;
	static constexpr type str_16 = 0xda;
	static constexpr type str_32 = 0xdb;
	static constexpr type array_16 = 0xdc;
	static constexpr type array_32 = 0xdd;
	static constexpr type map_16 = 0xde;
	static constexpr type map_32 = 0xdf;
	static constexpr type negative_fixint_min = 0xe0;
	static constexpr type negative_fixint_max = 0xff;
		
	static bool
	is_positive_int( type code )
	{
		return code <= positive_fixint_max || code == uint_8 || code == uint_16 || code == uint_64;
	}
		
	static bool
	is_int( type code )
	{
		return is_positive_int( code ) || ( code >= negative_fixint_min && code <= negative_fixint_max ) || 
				code == int_8 || code == int_16 || code == int_64;
	}

	static bool
	is_floating( type code )
	{
		return code == float_32 || code == float_64;
	}

	static bool
	is_nil( type code )
	{
		return code == nil;
	}
		
	static bool
	is_array( type code )
	{
		return ( code >= fixarray_min && code <= fixarray_max ) || code == array_16 || code == array_32;
	}

	static bool
	is_map( type code )
	{
		return ( code >= fixmap_min && code <= fixmap_max ) || code == map_16 || code == map_32;
	}

	static bool
	is_string( type code )
	{
		return ( code >= fixstr_min && code <= fixstr_max ) || code == str_8 || code == str_16 || code == str_32;
	}

	static bool
	is_blob( type code )
	{
		return code == bin_8 || code == bin_16 || code == bin_32;
	}

	static bool
	is_bool( type code )
	{
		return code == bool_true || code == bool_false;
	}

	static bool
	is_ext( type code )
	{
		return code == fixext_1 
			|| code == fixext_2
			|| code == fixext_4
			|| code == fixext_8
			|| code == fixext_16
			|| code == ext_8
			|| code == ext_16
			|| code == ext_32;
	}
};

namespace detail
{

	template< int N >
	struct fixext_typecode;

	template<>
	struct fixext_typecode< 1 >
	{
		static constexpr typecode::type value = typecode::fixext_1;
	};

	template<>
	struct fixext_typecode< 2 >
	{
		static constexpr typecode::type value = typecode::fixext_2;
	};

	template<>
	struct fixext_typecode< 4 >
	{
		static constexpr typecode::type value = typecode::fixext_4;
	};

	template<>
	struct fixext_typecode< 8 >
	{
		static constexpr typecode::type value = typecode::fixext_8;
	};

} // namespace detail
} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_TYPECODE_H

