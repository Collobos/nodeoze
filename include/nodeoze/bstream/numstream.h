/*
 * The MIT License
 *
 * Copyright 2018 David Curtis.
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
 * File:   numstream.h
 * Author: David Curtis
 *
 * Created on January 7, 2018, 11:22 AM
 */

#ifndef NODEOZE_BSTREAM_NUMSTREAM_H
#define NODEOZE_BSTREAM_NUMSTREAM_H

#include <type_traits>
#include <nodeoze/nbuffer.h>
// #include <nodeoze/membuf.h>
#include <nodeoze/bstream/error.h>
#include <sstream>
#include <boost/endian/conversion.hpp>
#include <assert.h>

namespace bend = boost::endian; 

namespace nodeoze
{
namespace bstream
{

using byte_type = std::uint8_t;
using offset_type = std::int64_t;
using position_type = std::uint64_t;
using size_type = std::uint64_t;

namespace detail
{

template< int N >
struct canonical_type;

template<>
struct canonical_type< 1 >
{
	using type = std::uint8_t;
};

template<>
struct canonical_type< 2 >
{
	using type = std::uint16_t;
};

template<>
struct canonical_type< 4 >
{
	using type = std::uint32_t;
};

template<>
struct canonical_type< 8 >
{
	using type = std::uint64_t;
};

} // namespace detail


class onumstream
{
public:

	inline 
	onumstream(bend::order order = bend::order::big ) 
	: 
	m_strmbuf{ nullptr },
	m_reverse_order{ order != bend::order::native }
	{}

	inline
	onumstream( std::unique_ptr< std::streambuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_reverse_order{ order != bend::order::native }
	{}

	virtual ~onumstream() {}

	inline void
	use( std::unique_ptr< std::streambuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
	}

	template< class T, class... Args >
	inline typename std::enable_if_t< std::is_base_of< std::streambuf, T >::value >
	use( Args&&... args )
	{
		m_strmbuf = std::make_unique< T >( std::forward< Args >( args )... );
	}

	inline std::streambuf&
	get_streambuf()
	{
		return *m_strmbuf.get();
	}

	inline std::streambuf const&
	get_streambuf() const
	{
		return *m_strmbuf.get();
	}

	inline std::unique_ptr< std::streambuf >
	release_streambuf()
	{
		return std::move( m_strmbuf );
		m_strmbuf = nullptr;
	}

	inline onumstream& 
	put( std::uint8_t byte )
	{
		m_strmbuf->sputc( static_cast< std::streambuf::char_type >( byte ) );
		return *this;                
	}

	inline onumstream& 
	putn( buffer const& buf )
	{
		m_strmbuf->sputn( reinterpret_cast< const std::streambuf::char_type* >( buf.const_data() ), buf.size() );
		return *this;
	}

	inline onumstream& 
	putn( const void* src, size_type nbytes )
	{
		m_strmbuf->sputn( reinterpret_cast< const std::streambuf::char_type* >( src ), nbytes );
		return *this;                
	}

	template<class U>
	inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof( U ) == 1, onumstream&>::type 
	put_num( U value )
	{
		m_strmbuf->sputc( static_cast< std::streambuf::char_type >( value ) );
		return *this;                
	}

	template<class U>
	inline typename std::enable_if< std::is_arithmetic<U>::value && ( sizeof( U ) > 1 ), onumstream& >::type
	put_num( U value )
	{
		constexpr std::size_t usize = sizeof( U );
		using ctype = typename detail::canonical_type< usize >::type;

		ctype cval = m_reverse_order ? bend::endian_reverse( reinterpret_cast< ctype& >( value ) ) : reinterpret_cast< ctype& >( value );
		m_strmbuf->sputn( reinterpret_cast< std::streambuf::char_type* >( &cval ), usize );
		return *this;
	}

	inline size_type 
	size()
	{
		auto pos = position();
		auto end_pos = position( 0, std::ios_base::end );
		position( pos );
		return static_cast< size_type >( end_pos );
	}

	inline position_type
	position() 
	{
		auto seek_result = m_strmbuf->pubseekoff( 0, std::ios_base::cur, std::ios_base::out );
		if ( seek_result < 0 )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_seek ) };
		}
		return static_cast< position_type >( seek_result );		
	}

	inline position_type
	position( position_type pos )
	{
		auto seek_result = m_strmbuf->pubseekpos( pos, std::ios_base::out );
		if ( seek_result < 0)
		{
			throw std::system_error{ make_error_code( std::errc::invalid_seek ) };
		}
		return static_cast< position_type >( seek_result );		
	}

	inline position_type
	position( offset_type offset, std::ios_base::seekdir way )
	{
		auto pos = m_strmbuf->pubseekoff( offset, way, std::ios_base::out );
		if ( pos < 0 )
		{
			throw std::system_error( make_error_code( std::errc::invalid_seek ) );
		}
		return static_cast< position_type >( pos );
	}

	inline void 
	write(const char* src, std::size_t len)
	{
		putn( src, len );
	}

protected:

	std::unique_ptr< std::streambuf >		m_strmbuf;
	const bool 								m_reverse_order;
};

class inumstream
{
public:

	inline 
	inumstream( bend::order order = bend::order::big )
	: 
	m_strmbuf{ nullptr }, 
	m_reverse_order{ order != bend::order::native }
	{}

	inline 
	inumstream( std::unique_ptr< std::streambuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_reverse_order{ order != bend::order::native }
	{}

	virtual ~inumstream() {}
		
	inline void
	use( std::unique_ptr< std::streambuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
	}

	template< class T, class... Args >
	inline typename std::enable_if_t< std::is_base_of< std::streambuf, T >::value >
	use( Args&&... args )
	{
		m_strmbuf = std::make_unique< T >( std::forward< Args >( args )... );
	}

	inline std::streambuf&
	get_streambuf()
	{
		return *m_strmbuf.get();
	}

	inline std::streambuf const&
	get_streambuf() const
	{
		return *m_strmbuf.get();
	}

	inline std::unique_ptr< std::streambuf >
	release_streambuf()
	{
		return std::move( m_strmbuf ); // hope this is set to null
	}

	inline position_type
	position()
	{
		auto seek_result = m_strmbuf->pubseekoff( 0, std::ios_base::cur, std::ios_base::in );
		if ( seek_result < 0 )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_seek ) };
		}
		return static_cast< position_type >( seek_result );		
	}

	inline position_type 
	position( position_type pos )
	{
		auto seek_result = m_strmbuf->pubseekpos( pos, std::ios_base::in );
		if ( seek_result < 0)
		{
			throw std::system_error{ make_error_code( std::errc::invalid_seek ) };
		}
		return static_cast< position_type >( seek_result );		
	}

	inline position_type
	position( offset_type offset, std::ios_base::seekdir way )
	{
		auto seek_result = m_strmbuf->pubseekoff( offset, way, std::ios_base::in );
		if ( seek_result < 0 )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_seek ) };
		}
		return static_cast< position_type >( seek_result );		
	}

	inline size_type 
	size()
	{
		auto pos = position();
		auto end_pos = position( 0, std::ios_base::end );
		position( pos );
		return static_cast< size_type >( end_pos );
	}

	inline void 
	rewind()
	{
		position( 0 );
	}
		
	inline byte_type
	get()
	{
		auto ch = m_strmbuf->sbumpc();
		if ( ch == std::streambuf::traits_type::eof() )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		return static_cast< byte_type >( ch );
	}

	inline byte_type 
	peek()
	{
		auto ch = m_strmbuf->sgetc();
		if ( ch == std::streambuf::traits_type::eof() )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		return static_cast< byte_type >( ch );

	}

	template< class U >
	inline typename std::enable_if< std::is_arithmetic< U >::value && sizeof( U ) == 1, U >::type 
	get_num()
	{
		return static_cast< U >( get() );
	}
		
	template< class U >
	inline typename std::enable_if< std::is_arithmetic< U >::value && ( sizeof( U ) > 1 ), U >::type 
	get_num()
	{
		constexpr std::size_t usize = sizeof( U );
		using ctype = typename detail::canonical_type< usize >::type;

		ctype cval;
		auto n = m_strmbuf->sgetn( reinterpret_cast< std::streambuf::char_type* >( &cval ), usize );
		if ( static_cast< size_type >( n ) < usize )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		cval = m_reverse_order ? bend::endian_reverse( cval ) : cval;
		return reinterpret_cast< U& >( cval );
	}

	virtual buffer
	getn( size_type nbytes, bool throw_on_eof = true )
	{
		buffer buf{ nbytes };
		auto n = m_strmbuf->sgetn( reinterpret_cast< std::streambuf::char_type* >( buf.mutable_data() ), nbytes );
		if ( throw_on_eof && static_cast< size_type >( n ) < nbytes )
		{
			if ( throw_on_eof )
			{
				throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
			}
			buf.size( static_cast< size_type >( n ) );
		}
		return buf;
	}

	size_type
	getn( byte_type* dst, size_type nbytes, bool throw_on_eof = true  )
	{
		auto n = m_strmbuf->sgetn( reinterpret_cast< std::streambuf::char_type* >( dst ), nbytes );
		if ( throw_on_eof && static_cast< size_type >( n ) < nbytes )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		return static_cast< size_type >( n );
	}

	inline bend::order
	byte_order() const
	{
		return m_reverse_order ? ( (bend::order::native == bend::order::little ) ? bend::order::big : bend::order::little ) : ( bend::order::native );
	}
/*
	void 
	dump( std::ostream& os, position_type pos, size_type length );

	std::string 
	strdump( position_type pos, size_type length );
*/
/*
	void 
	dump_state( std::ostream& os );
*/

protected:

	std::unique_ptr< std::streambuf >		m_strmbuf;
	const bool								m_reverse_order;
};

} // namespace bstream
} // namespace nodeoze

#endif /* NODEOZE_BSTREAM_NUMSTREAM_H */

