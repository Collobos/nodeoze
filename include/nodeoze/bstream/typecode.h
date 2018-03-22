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

#ifndef BSTREAM_TYPECODE_H
#define BSTREAM_TYPECODE_H

#include <cstdint>

namespace nodeoze
{
namespace bstream
{
	struct typecode
	{
		static constexpr int max_positive_fixint = 127;
		static constexpr int min_negative_fixint =  -32;
			
		static constexpr std::uint8_t positive_fixint_min = 0x00;
		static constexpr std::uint8_t positive_fixint_max = 0x7f;
		static constexpr std::uint8_t fixmap_min = 0x80;
		static constexpr std::uint8_t fixmap_max = 0x8f;
		static constexpr std::uint8_t fixarray_min = 0x90;
		static constexpr std::uint8_t fixarray_max = 0x9f;
		static constexpr std::uint8_t fixstr_min = 0xa0;
		static constexpr std::uint8_t fixstr_max = 0xbf;
		static constexpr std::uint8_t nil = 0xc0;
		static constexpr std::uint8_t unused = 0xc1;
		static constexpr std::uint8_t bool_false = 0xc2;
		static constexpr std::uint8_t bool_true = 0xc3;
		static constexpr std::uint8_t bin_8 = 0xc4;
		static constexpr std::uint8_t bin_16 = 0xc5;
		static constexpr std::uint8_t bin_32 = 0xc6;
		static constexpr std::uint8_t ext_8 = 0xc7;
		static constexpr std::uint8_t ext_16 = 0xc8;
		static constexpr std::uint8_t ext_32 = 0xc9;
		static constexpr std::uint8_t float_32 = 0xca;
		static constexpr std::uint8_t float_64 = 0xcb;
		static constexpr std::uint8_t uint_8 = 0xcc;
		static constexpr std::uint8_t uint_16 = 0xcd;
		static constexpr std::uint8_t uint_32 = 0xce;
		static constexpr std::uint8_t uint_64 = 0xcf;
		static constexpr std::uint8_t int_8 = 0xd0;
		static constexpr std::uint8_t int_16 = 0xd1;
		static constexpr std::uint8_t int_32 = 0xd2;
		static constexpr std::uint8_t int_64 = 0xd3;
		static constexpr std::uint8_t fixext_1 = 0xd4;
		static constexpr std::uint8_t fixext_2 = 0xd5;
		static constexpr std::uint8_t fixext_4 = 0xd6;
		static constexpr std::uint8_t fixext_8 = 0xd7;
		static constexpr std::uint8_t fixext_16 = 0xd8;
		static constexpr std::uint8_t str_8 = 0xd9;
		static constexpr std::uint8_t str_16 = 0xda;
		static constexpr std::uint8_t str_32 = 0xdb;
		static constexpr std::uint8_t array_16 = 0xdc;
		static constexpr std::uint8_t array_32 = 0xdd;
		static constexpr std::uint8_t map_16 = 0xde;
		static constexpr std::uint8_t map_32 = 0xdf;
		static constexpr std::uint8_t negative_fixint_min = 0xe0;
		static constexpr std::uint8_t negative_fixint_max = 0xff;
			
		static inline bool
		is_positive_int(std::uint8_t code)
		{
			return code <= positive_fixint_max || code == uint_8 || code == uint_16 || code == uint_64;
		}
			
		static inline bool
		is_int(std::uint8_t code)
		{
			return is_positive_int(code) || (code >= negative_fixint_min && code <= negative_fixint_max) || 
					code == int_8 || code == int_16 || code == int_64;
		}
			
		static inline bool
		is_array(std::uint8_t code)
		{
			return (code >= fixarray_min && code <= fixarray_max) || code == array_16 || code == array_32;
		}

		static inline bool
		is_map(std::uint8_t code)
		{
			return (code >= fixmap_min && code <= fixmap_max) || code == map_16 || code == map_32;
		}
	
		static inline bool
		is_string(std::uint8_t code)
		{
			return (code >= fixstr_min && code <= fixstr_max) || code == str_8 || code == str_16 || code == str_32;
		}

		static inline bool
		is_blob(std::uint8_t code)
		{
			return code == bin_8 || code == bin_16 || code == bin_32;
		}
	
		static inline bool
		is_bool(std::uint8_t code)
		{
			return code == bool_true || code == bool_false;
		}

	};
} // bstream
} // nodeoze

#endif /* BSTREAM_TYPECODES_H */

