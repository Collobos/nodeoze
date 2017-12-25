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

#ifndef _nodeoze_endian_h
#define _nodeoze_endian_h

namespace nodeoze {

namespace codec {

namespace big_endian {

// All these routines are unsafe in the sense that they assume that the buffer
// passed in has sufficient space

inline void
put( std::uint8_t *buf, std::int16_t val )
{
	buf[ 0 ] = static_cast< std::uint8_t >( ( val >> 8 ) & 0xFF );
	buf[ 1 ] = static_cast< std::uint8_t >( val & 0xFF );
}


inline void
put( std::uint8_t *buf, std::uint16_t val )
{
	buf[ 0 ] = static_cast< std::uint8_t >( ( val >> 8 ) & 0xFF );
	buf[ 1 ] = static_cast< std::uint8_t >( val & 0xFF );
}


inline void
put( std::uint8_t *buf, std::int32_t val )
{
	buf[ 0 ] = static_cast< std::uint8_t >( ( val >> 24 ) & 0xFF );
	buf[ 1 ] = static_cast< std::uint8_t >( ( val >> 16 ) & 0xFF );
	buf[ 2 ] = static_cast< std::uint8_t >( ( val >> 8 ) & 0xFF );
	buf[ 3 ] = static_cast< std::uint8_t >( val & 0xFF );
}


inline void
put( std::uint8_t *buf, std::uint32_t val )
{
	buf[ 0 ] = static_cast< std::uint8_t >( ( val >> 24 ) & 0xFF );
	buf[ 1 ] = static_cast< std::uint8_t >( ( val >> 16 ) & 0xFF );
	buf[ 2 ] = static_cast< std::uint8_t >( ( val >> 8 ) & 0xFF );
	buf[ 3 ] = static_cast< std::uint8_t >( val & 0xFF );
}


inline void
put( std::uint8_t *buf, std::int64_t val )
{
	buf[ 0 ] = static_cast< std::uint8_t >( ( val >> 56 ) & 0xFF );
	buf[ 1 ] = static_cast< std::uint8_t >( ( val >> 48 ) & 0xFF );
	buf[ 2 ] = static_cast< std::uint8_t >( ( val >> 40 ) & 0xFF );
	buf[ 3 ] = static_cast< std::uint8_t >( ( val >> 32 ) & 0xFF );
	buf[ 4 ] = static_cast< std::uint8_t >( ( val >> 24 ) & 0xFF );
	buf[ 5 ] = static_cast< std::uint8_t >( ( val >> 16 ) & 0xFF );
	buf[ 6 ] = static_cast< std::uint8_t >( ( val >> 8 ) & 0xFF );
	buf[ 7 ] = static_cast< std::uint8_t >( val & 0xFF );
}


inline void
put( std::uint8_t *buf, std::uint64_t val )
{
	buf[ 0 ] = static_cast< std::uint8_t >( ( val >> 56 ) & 0xFF );
	buf[ 1 ] = static_cast< std::uint8_t >( ( val >> 48 ) & 0xFF );
	buf[ 2 ] = static_cast< std::uint8_t >( ( val >> 40 ) & 0xFF );
	buf[ 3 ] = static_cast< std::uint8_t >( ( val >> 32 ) & 0xFF );
	buf[ 4 ] = static_cast< std::uint8_t >( ( val >> 24 ) & 0xFF );
	buf[ 5 ] = static_cast< std::uint8_t >( ( val >> 16 ) & 0xFF );
	buf[ 6 ] = static_cast< std::uint8_t >( ( val >> 8 ) & 0xFF );
	buf[ 7 ] = static_cast< std::uint8_t >( val & 0xFF );
}


inline void
put( std::uint8_t *buf, float val )
{
	union
	{
		float			f;
		std::uint32_t	u;
	} space;
	
	static_assert( sizeof( space ) == 4, "float union must be 32 bits" );
	
	space.f = val;
	put( buf, space.u );
}


inline void
put( std::uint8_t *buf, double val )
{
	union
	{
		double			d;
		std::uint64_t	u;
	} space;
	
	static_assert( sizeof( space ) == 8, "double union must be 64 bits" );
	
	space.d = val;
	put( buf, space.u );
}


inline void
get( const std::uint8_t *buf, std::uint16_t &val )
{
	val =  ( static_cast< std::uint16_t >( buf[ 0 ] ) << 8 );
	val |= ( static_cast< std::uint16_t >( buf[ 1 ] ) );
}


inline void
get( const std::uint8_t *buf, std::int16_t &val )
{
	val =  ( static_cast< std::uint16_t >( buf[ 0 ] ) << 8 );
	val |= ( static_cast< std::uint16_t >( buf[ 1 ] ) );
}


inline void
get( const std::uint8_t *buf, std::uint32_t &val )
{
	val =  ( static_cast< std::uint32_t >( buf[ 0 ] ) << 24 );
	val |= ( static_cast< std::uint32_t >( buf[ 1 ] ) << 16 );
	val |= ( static_cast< std::uint32_t >( buf[ 2 ] ) << 8 );
	val |= ( static_cast< std::uint32_t >( buf[ 3 ] ) );
}


inline void
get( const std::uint8_t *buf, std::int32_t &val )
{
	val =  ( static_cast< std::uint32_t >( buf[ 0 ] ) << 24 );
	val |= ( static_cast< std::uint32_t >( buf[ 1 ] ) << 16 );
	val |= ( static_cast< std::uint32_t >( buf[ 2 ] ) << 8 );
	val |= ( static_cast< std::uint32_t >( buf[ 3 ] ) );
}


inline void
get( const std::uint8_t *buf, std::uint64_t &val )
{
	val =  ( static_cast< std::uint64_t >( buf[ 0 ] ) << 56 );
	val |= ( static_cast< std::uint64_t >( buf[ 1 ]) << 48 );
	val |= ( static_cast< std::uint64_t >( buf[ 2 ] ) << 40 );
	val |= ( static_cast< std::uint64_t >( buf[ 3 ] ) << 32 );
	val |= ( static_cast< std::uint64_t >( buf[ 4 ] ) << 24 );
	val |= ( static_cast< std::uint64_t >( buf[ 5 ] ) << 16 );
	val |= ( static_cast< std::uint64_t >( buf[ 6 ] ) << 8 );
	val |= ( static_cast< std::uint64_t >( buf[ 7 ] ) );
}


inline void
get( const std::uint8_t *buf, std::int64_t &val )
{
	val =  ( static_cast< std::uint64_t >( buf[ 0 ] ) << 56 );
	val |= ( static_cast< std::uint64_t >( buf[ 1 ]) << 48 );
	val |= ( static_cast< std::uint64_t >( buf[ 2 ] ) << 40 );
	val |= ( static_cast< std::uint64_t >( buf[ 3 ] ) << 32 );
	val |= ( static_cast< std::uint64_t >( buf[ 4 ] ) << 24 );
	val |= ( static_cast< std::uint64_t >( buf[ 5 ] ) << 16 );
	val |= ( static_cast< std::uint64_t >( buf[ 6 ] ) << 8 );
	val |= ( static_cast< std::uint64_t >( buf[ 7 ] ) );
}


inline void
get( const std::uint8_t *buf, float &val )
{
	static_assert( sizeof( float ) == sizeof( std::uint32_t ), "float must be 32 bits" );
	get( buf, reinterpret_cast< std::uint32_t& >( val ) );
}


inline void
get( const std::uint8_t *buf, double &val )
{
	static_assert( sizeof( double ) == sizeof( std::uint64_t ), "double must be 64 bits" );
	get( buf, reinterpret_cast< std::uint64_t& >( val ) );
}

}

}

}

#endif
