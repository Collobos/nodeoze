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
#include <sstream>
#include <assert.h>
#include <nodeoze/buffer.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/bstreambuf.h>
#include <boost/endian/conversion.hpp>

namespace bend = boost::endian; 

namespace nodeoze
{
namespace bstream
{
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
	onumstream( std::unique_ptr< bstream::obstreambuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_reverse_order{ order != bend::order::native }
	{}

	virtual ~onumstream() {}

	inline bstream::obstreambuf&
	get_streambuf()
	{
		return *m_strmbuf.get();
	}

	inline bstream::obstreambuf const&
	get_streambuf() const
	{
		return *m_strmbuf.get();
	}

	inline std::unique_ptr< bstream::obstreambuf >
	release_streambuf()
	{
		return std::move( m_strmbuf );
		m_strmbuf = nullptr;
	}

	inline onumstream& 
	put( std::uint8_t byte )
	{
		m_strmbuf->put( byte );
		return *this;                
	}

	inline onumstream& 
	put( std::uint8_t byte, std::error_code& err )
	{
		m_strmbuf->put( byte, err );
		return *this;                
	}

	inline onumstream& 
	putn( buffer const& buf )
	{
		m_strmbuf->putn( buf.data(), buf.size() );
		return *this;
	}

	inline onumstream& 
	putn( buffer const& buf, std::error_code& err )
	{
		m_strmbuf->putn( buf.data(), buf.size(), err );
		return *this;
	}

	inline onumstream& 
	putn( const void* src, size_type nbytes )
	{
		m_strmbuf->putn( reinterpret_cast< const byte_type * >( src ), nbytes );
		return *this;
	}

	inline onumstream& 
	putn( const void* src, size_type nbytes, std::error_code& err )
	{
		m_strmbuf->putn( reinterpret_cast< const byte_type * >( src ), nbytes, err );
		return *this;                
	}

	template<class U>
	inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof( U ) == 1, onumstream&>::type 
	put_num( U value )
	{
		return put( static_cast< std::uint8_t >( value ) );
	}

	template<class U>
	inline typename std::enable_if<std::is_arithmetic<U>::value && sizeof( U ) == 1, onumstream&>::type 
	put_num( U value, std::error_code& err )
	{
		return put( static_cast< std::uint8_t >( value ), err );
	}

	template<class U>
	inline typename std::enable_if< std::is_arithmetic<U>::value && ( sizeof( U ) > 1 ), onumstream& >::type
	put_num( U value )
	{
		constexpr std::size_t usize = sizeof( U );
		using ctype = typename detail::canonical_type< usize >::type;
		ctype cval = m_reverse_order ? bend::endian_reverse( reinterpret_cast< ctype& >( value ) ) : reinterpret_cast< ctype& >( value );
		m_strmbuf->putn( reinterpret_cast< byte_type* >( &cval ), usize );
		return *this;
	}

	template<class U>
	inline typename std::enable_if< std::is_arithmetic<U>::value && ( sizeof( U ) > 1 ), onumstream& >::type
	put_num( U value, std::error_code& err )
	{
		constexpr std::size_t usize = sizeof( U );
		using ctype = typename detail::canonical_type< usize >::type;
		ctype cval = m_reverse_order ? bend::endian_reverse( reinterpret_cast< ctype& >( value ) ) : reinterpret_cast< ctype& >( value );
		m_strmbuf->putn( reinterpret_cast< byte_type* >( &cval ), usize, err );
		return *this;
	}

 	inline size_type 
	size()
	{
		return static_cast< size_type >( m_strmbuf->tell( seek_anchor::end ) );
	}

	inline size_type 
	size( std::error_code& err )
	{
		return static_cast< size_type >( m_strmbuf->tell( seek_anchor::end, err ) );
	}

	inline position_type
	position() 
	{
		return static_cast< position_type >( m_strmbuf->tell() );		
	}

	inline position_type
	position( std::error_code& err ) 
	{
		return static_cast< position_type >( m_strmbuf->tell( err ) );
	}

	inline position_type
	position( position_type pos )
	{
		return static_cast< position_type >( m_strmbuf->seek( pos ) );		
	}

	inline position_type
	position( position_type pos, std::error_code& err )
	{
		return static_cast< position_type >( m_strmbuf->seek( pos, err ) );
	}

	inline position_type
	position( offset_type offset, seek_anchor where )
	{
		return static_cast< position_type >( m_strmbuf->seek( where, offset ) );
	}

	inline position_type
	position( offset_type offset, seek_anchor where, std::error_code& err )
	{
		return static_cast< position_type >( m_strmbuf->seek( where, offset, err ) );
	}

	inline position_type
	tell( seek_anchor where ) 
	{
		return m_strmbuf->tell( where );
	}

	inline position_type
	tell( seek_anchor where, std::error_code& err ) 
	{
		return m_strmbuf->tell( where, err );
	}

	inline position_type
	seek( position_type pos, std::error_code& err )
	{
		return seek( seek_anchor::begin, pos, err );
	}

	inline position_type
	seek( position_type pos )
	{
		return seek( seek_anchor::begin, pos );
	}

	inline position_type
	seek( seek_anchor where, offset_type offset )
	{
		return m_strmbuf->seek( where, offset );
	}

	inline position_type
	seek( seek_anchor where, offset_type offset, std::error_code& err )
	{
		return m_strmbuf->seek( where, offset, err );
	}

	inline void 
	write(const char* src, std::size_t len)
	{
		putn( reinterpret_cast< const byte_type* >( src ), len );
	}

protected:

	inline void
	use( std::unique_ptr< bstream::obstreambuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
	}

	template< class T, class... Args >
	inline typename std::enable_if_t< std::is_base_of< bstream::obstreambuf, T >::value >
	use( Args&&... args )
	{
		m_strmbuf = std::make_unique< T >( std::forward< Args >( args )... );
	}

	std::unique_ptr< bstream::obstreambuf >		m_strmbuf;
	const bool 									m_reverse_order;
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
	inumstream( std::unique_ptr< bstream::ibstreambuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_reverse_order{ order != bend::order::native }
	{}

	virtual ~inumstream() {}
		
	inline bstream::ibstreambuf&
	get_streambuf()
	{
		return *m_strmbuf.get();
	}

	inline bstream::ibstreambuf const&
	get_streambuf() const
	{
		return *m_strmbuf.get();
	}

	inline std::unique_ptr< bstream::ibstreambuf >
	release_streambuf()
	{
		return std::move( m_strmbuf ); // hope this is set to null
	}

 	inline size_type 
	size()
	{
		return static_cast< size_type >( m_strmbuf->tell( seek_anchor::end ) );
	}

	inline size_type 
	size( std::error_code& err )
	{
		return static_cast< size_type >( m_strmbuf->tell( seek_anchor::end, err ) );
	}

	inline position_type
	position() 
	{
		return static_cast< position_type >( m_strmbuf->tell() );		
	}

	inline position_type
	position( std::error_code& err ) 
	{
		return static_cast< position_type >( m_strmbuf->tell( err ) );
	}

	inline position_type
	position( position_type pos )
	{
		return static_cast< position_type >( m_strmbuf->seek( pos ) );		
	}

	inline position_type
	position( position_type pos, std::error_code& err )
	{
		return static_cast< position_type >( m_strmbuf->seek( pos, err ) );
	}

	inline position_type
	position( offset_type offset, seek_anchor where )
	{
		return static_cast< position_type >( m_strmbuf->seek( where, offset ) );
	}

	inline position_type
	position( offset_type offset, seek_anchor where, std::error_code& err )
	{
		return static_cast< position_type >( m_strmbuf->seek( where, offset, err ) );
	}

	inline position_type
	tell( seek_anchor where )
	{
		return m_strmbuf->tell( where );
	}

	inline position_type
	tell( seek_anchor where, std::error_code& err )
	{
		return m_strmbuf->tell( where, err );
	}

	inline position_type 
	seek( seek_anchor where, offset_type offset )
	{
		return m_strmbuf->seek( where, offset );
	}

	inline position_type 
	seek( seek_anchor where, offset_type offset, std::error_code& err )
	{
		return m_strmbuf->seek( where, offset, err );
	}

	inline position_type
	seek( position_type pos )
	{
		return seek( seek_anchor::begin, pos );
	}

	inline position_type
	seek( position_type pos, std::error_code& err )
	{
		return seek( seek_anchor::begin, pos, err );
	}

	inline void 
	rewind()
	{
		position( 0 );
	}

	inline void 
	rewind( std::error_code& err )
	{
		position( 0, err );
	}

	inline byte_type
	get()
	{
		return m_strmbuf->get();
	}

	inline byte_type
	get( std::error_code& err )
	{
		return m_strmbuf->get( err );
	}

	inline byte_type 
	peek()
	{
		return m_strmbuf->peek();
	}

	inline byte_type 
	peek( std::error_code& err )
	{
		return m_strmbuf->peek( err );
	}

	template< class U >
	inline typename std::enable_if< std::is_arithmetic< U >::value && sizeof( U ) == 1, U >::type 
	get_num()
	{
		return static_cast< U >( get() );
	}

	template< class U >
	inline typename std::enable_if< std::is_arithmetic< U >::value && sizeof( U ) == 1, U >::type 
	get_num( std::error_code& err )
	{
		return static_cast< U >( get( err ) );
	}

	template< class U >
	inline typename std::enable_if< std::is_arithmetic< U >::value && ( sizeof( U ) > 1 ), U >::type 
	get_num()
	{
		constexpr std::size_t usize = sizeof( U );
		using ctype = typename detail::canonical_type< usize >::type;

		ctype cval;
		m_strmbuf->getn( reinterpret_cast< byte_type* >( &cval ), usize );
		cval = m_reverse_order ? bend::endian_reverse( cval ) : cval;
		return reinterpret_cast< U& >( cval );
	}

	template< class U >
	inline typename std::enable_if< std::is_arithmetic< U >::value && ( sizeof( U ) > 1 ), U >::type 
	get_num( std::error_code& err )
	{
		constexpr std::size_t usize = sizeof( U );
		using ctype = typename detail::canonical_type< usize >::type;

		ctype cval = 0;
		m_strmbuf->getn( reinterpret_cast< byte_type* >( &cval ), usize, err );
		if ( err ) goto exit;

		cval = m_reverse_order ? bend::endian_reverse( cval ) : cval;

	exit:
		return reinterpret_cast< U& >( cval );
	}

	virtual buffer
	getn( size_type nbytes, bool throw_on_incomplete = true )
	{
		buffer buf = m_strmbuf->getn( nbytes );
		if ( throw_on_incomplete && buf.size() < nbytes )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		return buf;
	}

	virtual buffer
	getn( size_type nbytes, std::error_code& err, bool err_on_incomplete = true )
	{
		clear_error( err );
		buffer buf = m_strmbuf->getn( nbytes, err );
		if ( err ) goto exit;

		if ( err_on_incomplete && buf.size() < nbytes )
		{
			err = make_error_code( bstream::errc::read_past_end_of_stream );
			goto exit;
		}
	exit:
		return buf;
	}

	size_type
	getn( byte_type* dst, size_type nbytes, bool throw_on_incomplete = true  )
	{
		auto result = m_strmbuf->getn( dst, nbytes );
		if ( throw_on_incomplete && result < nbytes )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		return result;
	}

	size_type
	getn( byte_type* dst, size_type nbytes, std::error_code& err, bool err_on_incomplete = true  )
	{
		clear_error( err );
		auto result = m_strmbuf->getn( dst, nbytes, err );
		if ( err ) goto exit;

		if ( err_on_incomplete && result < nbytes )
		{
			err = make_error_code( bstream::errc::read_past_end_of_stream );
			goto exit;
		}
	exit:
		return result;
	}

	inline bend::order
	byte_order() const
	{
		return m_reverse_order ? ( (bend::order::native == bend::order::little ) ? bend::order::big : bend::order::little ) : ( bend::order::native );
	}

protected:

	inline void
	use( std::unique_ptr< bstream::ibstreambuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
	}

	template< class T, class... Args >
	inline typename std::enable_if_t< std::is_base_of< bstream::ibstreambuf, T >::value >
	use( Args&&... args )
	{
		m_strmbuf = std::make_unique< T >( std::forward< Args >( args )... );
	}

	std::unique_ptr< bstream::ibstreambuf >		m_strmbuf;
	const bool								m_reverse_order;
};

} // namespace bstream
} // namespace nodeoze

#endif /* NODEOZE_BSTREAM_NUMSTREAM_H */

