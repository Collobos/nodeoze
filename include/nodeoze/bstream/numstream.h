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
#include <nodeoze/membuf.h>
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

inline position_type
get_streambuf_end( std::streambuf& strmbuf, std::ios_base::openmode which )
{
	auto savepos = strmbuf.pubseekoff( 0, std::ios_base::cur, which );
	if ( savepos < 0 )
	{
		throw std::system_error( make_error_code( std::errc::invalid_seek ) );
	}
	
	auto endpos = strmbuf.pubseekoff( 0, std::ios_base::end, which );
	if ( endpos < 0 )
	{
		throw std::system_error( make_error_code( std::errc::invalid_seek ) );
	}

	if ( savepos != endpos )
	{
		strmbuf.pubseekpos( savepos, which );
	}

	return static_cast< position_type >( endpos );
}


inline position_type
seek_streambuf_end( std::streambuf& strmbuf, std::ios_base::openmode which )
{

	auto endpos = strmbuf.pubseekoff( 0, std::ios_base::end, which );
	if ( endpos < 0 )
	{
		throw std::system_error( make_error_code( std::errc::invalid_seek ) );
	}
	return static_cast< position_type >( endpos );
}

inline position_type
get_streambuf_pos( std::streambuf& strmbuf, std::ios_base::openmode which )
{
	auto pos = strmbuf.pubseekoff( 0, std::ios_base::cur, which );
	if ( pos < 0 )
	{
		throw std::system_error( make_error_code( std::errc::invalid_seek ) );
	}
	return static_cast< position_type>( pos );
}

inline position_type
seek_streambuf_pos( std::streambuf& strmbuf, position_type pos, std::ios_base::openmode which )
{

	auto seek_result = strmbuf.pubseekpos( pos, which );
	if ( seek_result < 0 )
	{
		throw std::system_error( make_error_code( std::errc::invalid_seek ) );
	}
	return static_cast< position_type >( seek_result );
}

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

class inumstream;

class onumstream
{
public:

	friend class inumstream;

	inline 
	onumstream(bend::order order = bend::order::big ) 
	: 
	m_strmbuf{ nullptr },
	m_is_buffer{ false },
	m_reverse_order{ order != bend::order::native }
	{}

	inline
	onumstream( std::unique_ptr< std::streambuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_is_buffer{ false },
	m_reverse_order{ order != bend::order::native }
	{}

	inline
	onumstream( std::unique_ptr< omembuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_is_buffer{ false },
	m_reverse_order{ order != bend::order::native }
	{}

	inline
	onumstream( buffer const& buf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::make_unique< omembuf >( buf ) },
	m_is_buffer{ true },
	m_reverse_order{ order != bend::order::native }
	{}

	inline
	onumstream( buffer&& buf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::make_unique< omembuf >( std::move( buf ) ) },
	m_is_buffer{ true },
	m_reverse_order{ order != bend::order::native }
	{}

	inline onumstream( size_type capacity, buffer::policy pol = buffer::policy::copy_on_write, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::make_unique< omembuf >( capacity, pol ) },
	m_is_buffer{ true },
	m_reverse_order{ order != bend::order::native }
	{}

	virtual ~onumstream() {}

	inline void
	use( std::unique_ptr< std::streambuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
		m_is_buffer = false;
	}

	inline void
	use( std::unique_ptr< omembuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
		m_is_buffer = true;
	}

	inline void
	use( buffer const& buf )
	{
		m_strmbuf = std::make_unique< omembuf >( buf );
		m_is_buffer = true;
	}

	inline void
	use( buffer&& buf )
	{
		m_strmbuf = std::make_unique< omembuf >( std::move( buf ) );
		m_is_buffer = true;
	}

	template< class T, class... Args >
	inline typename std::enable_if_t< std::is_base_of< std::streambuf, T >::value >
	use( Args&&... args )
	{
		m_strmbuf = std::make_unique< T >( std::forward< Args >( args )... );
		m_is_buffer = std::is_same< T, omembuf >::value;
	}

	inline std::streambuf&
	get_streambuf()
	{
		return *m_strmbuf.get();
	}

	inline std::unique_ptr< std::streambuf >
	release_streambuf()
	{
		return std::move( m_strmbuf );
		m_strmbuf = nullptr;
	}

	inline buffer
	get_buffer( bool force_copy = false )
	{
		if ( ! m_is_buffer )
		{
			throw std::system_error{ std::make_error_code( std::errc::operation_not_supported ) };
		}
		auto sbptr = reinterpret_cast< omembuf* >( m_strmbuf.get() );
		auto size = sbptr->get_buffer().size();
		return sbptr->get_buffer().slice( 0 , size, force_copy );
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
		return static_cast< size_type >( detail::get_streambuf_end( *m_strmbuf, std::ios_base::out ) );
	}

	inline position_type
	position() 
	{
		return static_cast< size_type >( detail::get_streambuf_pos( *m_strmbuf, std::ios_base::out ) );
	}

	inline position_type
	position( position_type pos )
	{
		return detail::seek_streambuf_pos( *m_strmbuf, pos, std::ios_base::out );
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

	inline onumstream&
	clear()
	{
		if ( ! m_is_buffer )
		{
			throw std::system_error{ std::make_error_code( std::errc::operation_not_supported ) };
		}
		auto sbptr = reinterpret_cast< omembuf* >( m_strmbuf.get() );
		sbptr->clear();
		return *this;
	}

	inline void 
	write(const char* src, std::size_t len)
	{
		putn( src, len );
	}

	inline void 
	dump( std::ostream& os, position_type pos, size_type length );
		
	inline std::string 
	strdump( position_type pos, size_type length );

protected:

	std::unique_ptr< std::streambuf >		m_strmbuf;
	bool									m_is_buffer;
//	bend::order								m_byte_order;
	const bool 								m_reverse_order;
};

class inumstream
{
public:

	inline 
	inumstream( bend::order order = bend::order::big )
	: 
	m_strmbuf{ nullptr }, 
	m_size{ 0 },
	m_pos{ 0 },
	m_is_buffer{ false },
	m_reverse_order{ order != bend::order::native }
	{}

	inline 
	inumstream( std::unique_ptr< std::streambuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_size{ detail::get_streambuf_end( *m_strmbuf, std::ios_base::in ) },
	m_pos{ detail::get_streambuf_pos( *m_strmbuf, std::ios_base::in ) },
	m_is_buffer{ false },
	m_reverse_order{ order != bend::order::native }
	{}

	inline 
	inumstream( std::unique_ptr< imembuf > strmbuf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::move( strmbuf ) },
	m_size{ detail::get_streambuf_end( *m_strmbuf, std::ios_base::in ) },
	m_pos{ detail::get_streambuf_pos( *m_strmbuf, std::ios_base::in ) },
	m_is_buffer{ true },
	m_reverse_order{ order != bend::order::native }
	{}

	inline
	inumstream( buffer const& buf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::make_unique< imembuf >( buf ) },
	m_size{ detail::get_streambuf_end( *m_strmbuf, std::ios_base::in ) },
	m_pos{ 0 },
	m_is_buffer{ true },
	m_reverse_order{ order != bend::order::native }
	{}

	inline
	inumstream( buffer&& buf, bend::order order = bend::order::big )
	:
	m_strmbuf{ std::make_unique< imembuf >( std::move( buf ) ) },
	m_size{ detail::get_streambuf_end( *m_strmbuf, std::ios_base::in ) },
	m_pos{ 0 },
	m_is_buffer{ true },
	m_reverse_order{ order != bend::order::native }
	{}

	virtual ~inumstream() {}
		
	inline void
	use( std::unique_ptr< std::streambuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
		m_size = detail::get_streambuf_end( *m_strmbuf, std::ios_base::in );
		m_pos = detail::get_streambuf_pos( *m_strmbuf, std::ios_base::in );
		m_is_buffer = false;
	}

	inline void
	use( std::unique_ptr< imembuf > strmbuf )
	{
		m_strmbuf = std::move( strmbuf );
		m_size = detail::get_streambuf_end( *m_strmbuf, std::ios_base::in );
		m_pos = detail::get_streambuf_pos( *m_strmbuf, std::ios_base::in );
		m_is_buffer = true;
	}

	inline void
	use( buffer const& buf )
	{
		m_size = buf.size();
		m_strmbuf = std::make_unique< imembuf >( buf );
		m_pos = 0;
		m_is_buffer = true;
	}

	inline void
	use( buffer&& buf )
	{
		m_size = buf.size();
		m_strmbuf = std::make_unique< imembuf >( std::move( buf ) );
		m_pos = 0;
		m_is_buffer = true;
	}

	template< class T, class... Args >
	inline typename std::enable_if_t< std::is_base_of< std::streambuf, T >::value >
	use( Args&&... args )
	{
		m_strmbuf = std::make_unique< T >( std::forward< Args >( args )... );
		m_size = detail::get_streambuf_end( *m_strmbuf, std::ios_base::in );
		m_pos = detail::get_streambuf_pos( *m_strmbuf, std::ios_base::in );
		m_is_buffer = std::is_same< T, imembuf >::value;
	}

	inline std::streambuf&
	get_streambuf()
	{
		return *m_strmbuf.get();
	}

	inline std::unique_ptr< std::streambuf >
	release_streambuf()
	{
		m_size = 0;
		m_pos = 0;
		m_is_buffer = false;
		return std::move( m_strmbuf ); // hope this is set to null
	}

	inline buffer
	get_buffer()
	{
		if ( m_is_buffer )
		{
			auto bptr = reinterpret_cast< imembuf* >( m_strmbuf.get() );
			return bptr->get_buffer();
		}
		else
		{
			throw std::system_error{ std::make_error_code( std::errc::operation_not_supported ) };
		}
	}

	inline size_type
	remaining() const noexcept
	{
		return m_size - m_pos;
	}

	inline position_type
	position()
	{
		assert( check_pos_sync() );
		return m_pos;
	}

	inline position_type 
	position( position_type pos )
	{
		assert( check_pos_sync() );

		if ( pos > m_size )
		{
			throw std::system_error( make_error_code( std::errc::invalid_argument ) );
		}
		auto seek_result = m_strmbuf->pubseekpos( pos, std::ios_base::in );
		if ( seek_result < 0)
		{
			throw std::system_error{ make_error_code( std::errc::invalid_seek ) };
		}
		m_pos = static_cast< std::size_t >( seek_result );
		return m_pos;
	}

	inline position_type
	position( offset_type offset, std::ios_base::seekdir way )
	{
		assert( check_pos_sync() );

		offset_type new_pos = 0;

		switch ( way )
		{
			case std::ios_base::beg:
			{
				new_pos = offset;
			}
			break;

			case std::ios_base::cur:
			{
				new_pos = static_cast< offset_type >( m_pos ) + offset;
			}
			break;

			case std::ios_base::end:
			{
				new_pos = static_cast< offset_type >( m_size ) + offset;
			}
		}
		if ( new_pos < 0 || static_cast< size_type >( new_pos ) > m_size )
		{
			throw std::system_error( make_error_code( std::errc::invalid_argument ) );
		}
		return position( static_cast< position_type >( new_pos ) );
	}

	inline size_type 
	size() const noexcept
	{
		return m_size;
	}

	inline void 
	rewind()
	{
		position( 0 );
	}
		
	inline byte_type
	get()
	{
		if ( remaining() < 1 )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		++m_pos;
		return static_cast< byte_type >( m_strmbuf->sbumpc() );
	}

	inline byte_type 
	peek()
	{
		if ( remaining() < 1 )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}
		return static_cast< byte_type >( m_strmbuf->sgetc() );
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

		if ( remaining() < usize )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}

		ctype cval;
		m_strmbuf->sgetn( reinterpret_cast< std::streambuf::char_type* >( &cval ), usize );
		m_pos += usize;
		cval = m_reverse_order ? bend::endian_reverse( cval ) : cval;
		return reinterpret_cast< U& >( cval );
	}

	buffer
	getn( size_type nbytes )
	{
		if ( remaining() < nbytes )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}

		if ( m_is_buffer )
		{
			auto bptr = reinterpret_cast< imembuf* >( m_strmbuf.get() );
			buffer internal_buf = bptr->get_buffer();
			auto prev_pos = m_pos;
			auto new_pos = m_strmbuf->pubseekoff( nbytes, std::ios_base::cur, std::ios_base::in );
			m_pos += nbytes;
			assert( m_pos == static_cast< position_type >( new_pos ) );
			return bptr->get_buffer().slice( prev_pos, nbytes );
		}
		else
		{
			buffer buf{ nbytes };
			auto n = m_strmbuf->sgetn( reinterpret_cast< std::streambuf::char_type* >( buf.mutable_data() ), nbytes );
			assert( static_cast< size_type >( n ) == nbytes );
			m_pos += n;
			return buf;
		}
	}

	void
	getn( byte_type* dst, size_type nbytes )
	{
		if ( remaining() < nbytes )
		{
			throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
		}

		auto n = m_strmbuf->sgetn( reinterpret_cast< std::streambuf::char_type* >( dst ), nbytes );
		assert( static_cast< size_type >( n ) == nbytes );
		m_pos += nbytes;
	}

	inline bend::order
	byte_order() const
	{
		return m_reverse_order ? ( (bend::order::native == bend::order::little ) ? bend::order::big : bend::order::little ) : ( bend::order::native );
	}

	void 
	dump( std::ostream& os, position_type pos, size_type length );

	std::string 
	strdump( position_type pos, size_type length );
/*
	void 
	dump_state( std::ostream& os );
*/

protected:

	void
	sync_position()
	{
		auto seek_result = m_strmbuf->pubseekoff( 0, std::ios_base::cur, std::ios_base::in );
		if ( seek_result == std::streambuf::traits_type::eof() )
		{
			throw std::system_error( make_error_code( std::errc::invalid_seek ) );
		}
		m_pos = seek_result;
	}
		
	inline bool
	check_pos_sync()
	{
		auto seek_result = m_strmbuf->pubseekoff( 0, std::ios_base::cur, std::ios_base::in );
		if ( seek_result == std::streambuf::traits_type::eof() )
		{
			std::cout << "in check_pos_sync(), seek_result is " << seek_result << std::endl; std::cout.flush();
			return false;
		}
		return ( seek_result != std::streambuf::traits_type::eof() ) && ( m_pos == static_cast< position_type >( seek_result ) );
	}

	std::unique_ptr< std::streambuf >		m_strmbuf;
	size_type								m_size;
	position_type 							m_pos;
	bool									m_is_buffer;
//	bend::order								m_byte_order;
	const bool								m_reverse_order;
};

} // namespace bstream
} // namespace nodeoze

#endif /* NODEOZE_BSTREAM_NUMSTREAM_H */

