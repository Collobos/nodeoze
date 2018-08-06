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
 * File:   obstream.h
 * Author: David Curtis
 *
 * Created on June 29, 2017, 9:58 PM
 */

#ifndef NODEOZE_BSTREAM_OBSTREAM_H
#define NODEOZE_BSTREAM_OBSTREAM_H

#include <nodeoze/bstream/obstream_traits.h>
#include <nodeoze/bstream/context.h>
#include <nodeoze/bstream/numstream.h>
#include <nodeoze/bstream/typecode.h>
#include <vector>
#include <unordered_map>


namespace nodeoze
{
namespace bstream
{

class obstream : public onumstream
{
protected:

//	obstream() {} // TODO: decide whether to delete certain ctors
		
public:	

    template< class U, class E > friend struct serializer;

    using saved_ptr_info = std::pair< poly_tag_type , std::size_t >;	// ( type_tag, saved_serial_num )

	class ptr_deduper
	{
	public:

		bool
		is_saved( std::shared_ptr< void > ptr, saved_ptr_info& info ) const
		{
			auto it = m_saved_ptrs.find( ptr );
			if ( it == m_saved_ptrs.end() )
			{
				return false;
			}
			else
			{
				info = it->second;
				return true;
			}
		}

		void save_ptr( std::shared_ptr< void > ptr, poly_tag_type tag )
		{
			m_saved_ptrs.emplace( ptr, std::make_pair( tag, m_next_serial_num ) );
			++m_next_serial_num;
		}

		void
		clear()
		{
			m_saved_ptrs.clear();
			m_next_serial_num = 0;
		}

	private:
		std::size_t 														m_next_serial_num = 0;
		std::unordered_map< std::shared_ptr< void >, saved_ptr_info >		m_saved_ptrs; 
	};

	obstream( std::unique_ptr< bstream::obstreambuf > strmbuf, context_base const& cntxt = get_default_context() )
	:
	onumstream{ std::move( strmbuf ), cntxt.get_context_impl()->byte_order() },
    m_context{ cntxt.get_context_impl() },
    m_ptr_deduper{ m_context->dedup_shared_ptrs() ? std::make_unique< ptr_deduper >() : nullptr }
	{}

	obstream&
	write_map_header( std::uint32_t size );

	obstream&
	write_map_header( std::uint32_t size, std::error_code& err );

	obstream&
	write_array_header( std::uint32_t size );     

	obstream&
	write_array_header( std::uint32_t size, std::error_code& err );

	obstream&
	write_blob_header( std::uint32_t size );

	obstream&
	write_blob_header( std::uint32_t size, std::error_code& err );

	obstream&
	write_blob_body( const void* p, std::size_t size )
	{
		putn( reinterpret_cast< const byte_type * >( p ), size );
		return *this;
	}

	obstream&
	write_blob_body( const void* p, std::size_t size, std::error_code& err )
	{
		putn( reinterpret_cast< const byte_type * >( p ), size, err );
		return *this;
	}

	obstream&
	write_blob_body( nodeoze::buffer const& blob )
	{
		putn( blob.data(), blob.size() );
		return *this;
	}

	obstream&
	write_blob_body( nodeoze::buffer const& blob, std::error_code& err )
	{
		putn( blob.data(), blob.size(), err );
		return *this;
	}

	obstream&
	write_blob( const void* p, std::size_t size )
	{
		write_blob_header( size );
		write_blob_body( p, size );
		return *this;
	}

	obstream&
	write_blob( const void* p, std::size_t size, std::error_code& err )
	{
		write_blob_header( size, err );
		if ( err ) goto exit;

		write_blob_body( p, size, err );

	exit:
		return *this;
	}

	obstream&
	write_blob( nodeoze::buffer const& buf )
	{
		write_blob_header( buf.size() );
		write_blob_body( buf );
		return *this;
	}

	obstream&
	write_blob( nodeoze::buffer const& buf, std::error_code& err )
	{
		write_blob_header( buf.size(), err );
		if ( err ) goto exit;

		write_blob_body( buf, err );

	exit:
		return *this;
	}

	obstream&
	write_object_header( std::uint32_t size )
	{
		return write_array_header( size );
	}

	obstream&
	write_object_header( std::uint32_t size, std::error_code& err )
	{
		return write_array_header( size, err );
	}

	template< class T >
	typename std::enable_if_t< std::is_arithmetic< T >::value, obstream& >	
	write_ext( std::uint8_t ext_type, T value )
	{
		put_num( detail::fixext_typecode< sizeof( T ) >::value );
		put_num( ext_type );
		put_num( value );
		return *this;
	}

	template< class T >
	typename std::enable_if_t< std::is_arithmetic< T >::value, obstream& >	
	write_ext( std::uint8_t ext_type, T value, std::error_code& err )
	{
		put_num( detail::fixext_typecode< sizeof( T ) >::value, err );
		if ( err ) goto exit;

		put_num( ext_type, err );
		if ( err ) goto exit;

		put_num( value, err );

	exit:
		return *this;
	}

	obstream&
	write_ext( std::uint8_t ext_type, buffer const& buf );

	obstream&
	write_ext( std::uint8_t ext_type, buffer const& buf, std::error_code& err );

	obstream&
	write_ext( std::uint8_t ext_type, std::vector< std::uint8_t > const& vec );

	obstream&
	write_ext( std::uint8_t ext_type, std::vector< std::uint8_t > const& vec, std::error_code& err );

	obstream&
	write_ext_header( std::uint8_t ext_type, std::uint32_t size );

	obstream&
	write_ext_header( std::uint8_t ext_type, std::uint32_t size, std::error_code& err  );

	obstream&
	write_ext( std::uint8_t ext_type, void* data, std::uint32_t size );

	obstream&
	write_ext( std::uint8_t ext_type, void* data, std::uint32_t size, std::error_code& err );

	obstream&
	write_null_ptr();

	obstream&
	write_null_ptr( std::error_code& err );

	obstream&
	write_nil()
	{
		put( typecode::nil );
		return *this;
	}

	obstream&
	write_nil( std::error_code& err )
	{
		put( typecode::nil, err );
		return *this;
	}

protected:

	template< class T >
	obstream&
	write_shared_ptr( std::shared_ptr< T > ptr );

	template< class T >
	obstream&
	write_shared_ptr( std::shared_ptr< T > ptr, std::error_code& err )
	{
		clear_error( err );
		try
		{
			write_shared_ptr( ptr );
		}
		catch ( std::system_error const& e )
		{
			err = e.code();
		}
		return *this;
	}

	template< class T >
	obstream&
	write_as_unique_pointer( T *ptr );

	template< class T >
	obstream&
	write_as_unique_pointer( T *ptr, std::error_code& err )
	{
		clear_error( err );
		try
		{
			write_as_unique_pointer( ptr );
		}
		catch ( std::system_error const& e )
		{
			err = e.code();
		}
		return *this;
	}

	obstream&
	write_error_code( std::error_code const& ecode );

	obstream&
	write_error_code( std::error_code const& ecode, std::error_code& err );
		
private:
		
	static std::size_t 
	map_header_size( std::size_t map_size );
		
	static std::size_t 
	array_header_size( std::size_t array_size );
		
	static std::size_t 
	blob_header_size( std::size_t blob_size );

    std::shared_ptr< const context_impl_base >      m_context;
    std::unique_ptr< ptr_deduper >                  m_ptr_deduper;
};

template< class T >
inline 
typename std::enable_if_t < has_serialize_method< T >::value, obstream& >
operator<< ( obstream& os, const T& value )
{
	return value.serialize( os );
}

template< class T >
inline 
typename std::enable_if_t < ! has_serialize_method< T >::value && has_serializer< T >::value, obstream& >
operator<< ( obstream& os, const T& value )
{
	return serializer< T >::put( os, value );
}

template< class T >
struct serializer< T, 
		typename std::enable_if_t<
			std::is_integral< T >::value && 
			!std::numeric_limits< T >::is_signed > >
{
	static obstream& put( obstream& os, T value )
	{
		if ( value <= typecode::positive_fixint_max )
		{
			std::uint8_t val = static_cast< std::uint8_t >( value );
			os.put( val );
		}
		else if ( value <= std::numeric_limits< std::uint8_t >::max() )
		{
			os.put( typecode::uint_8 );
			os.put( static_cast< std::uint8_t >( value ) );
		}
		else if ( value <= std::numeric_limits< std::uint16_t >::max() )
		{
			os.put( typecode::uint_16 );
			os.put_num( static_cast< std::uint16_t >( value ) );
		}
		else if ( value <= std::numeric_limits< std::uint32_t >::max() )
		{
			os.put( typecode::uint_32 );
			os.put_num( static_cast< std::uint32_t >( value ) );
		}
		else
		{
			os.put( typecode::uint_64 );
			os.put_num( static_cast< std::uint64_t >( value ) );
		}
		return os;
	}
};

template< class T >
struct serializer< T, 
		typename std::enable_if_t<
			std::is_integral< T >::value && 
			std::numeric_limits< T >::is_signed > >
{
	static obstream& put( obstream& os, T value )
	{
		if ( value >= 0 )
		{
			std::uint64_t uvalue = static_cast< std::uint64_t >( value );

			if ( uvalue <= typecode::positive_fixint_max )
			{
				std::uint8_t val = static_cast< std::uint8_t >( uvalue );
				os.put( val );
			}
			else if ( uvalue <= std::numeric_limits< std::uint8_t >::max() )
			{
				os.put( typecode::uint_8 );
				os.put( static_cast< std::uint8_t >( uvalue ) );
			}
			else if ( uvalue <= std::numeric_limits< std::uint16_t >::max() )
			{
				os.put( typecode::uint_16 );
				os.put_num( static_cast< std::uint16_t >( uvalue ) );
			}
			else if ( uvalue <= std::numeric_limits< std::uint32_t >::max() )
			{
				os.put( typecode::uint_32 );
				os.put_num( static_cast< std::uint32_t >( uvalue ) );
			}
			else
			{
				os.put( typecode::uint_64 );
				os.put_num( uvalue );
			}
		}
		else
		{
			if ( value >= -32 )
			{
				os.put( static_cast< std::uint8_t >( value ) );
			}
			else if ( value >= std::numeric_limits< std::int8_t >::min() )
			{
				os.put( typecode::int_8 );
				os.put_num( static_cast< std::int8_t >( value ) );
			}
			else if ( value >= std::numeric_limits< std::int16_t >::min() )
			{
				os.put( typecode::int_16 );
				os.put_num( static_cast< std::int16_t >( value ) );
			}
			else if ( value >= std::numeric_limits< std::int32_t >::min() )
			{
				os.put( typecode::int_32 );
				os.put_num( static_cast< std::int32_t >( value ) );
			}
			else
			{
				os.put( typecode::int_64 );
				os.put_num( static_cast< std::int64_t >( value ) );
			}
		}
		return os;
	}
};

template<>
struct serializer< float >
{
	static obstream& put( obstream& os, float value )
	{
		os.put( typecode::float_32 );
		os.put_num( value );
		return os;
	}
};

template<>
struct serializer< double >
{
	static obstream& put( obstream& os, double value )
	{
		os.put( typecode::float_64 );
		os.put_num( value );
		return os;
	}
};

template<>
struct serializer< bool >
{
	static obstream& put( obstream& os, bool value )
	{
		if ( value )
		{
			os.put( typecode::bool_true );
		}
		else
		{
			os.put( typecode::bool_false );
		}
		return os;
	}
};

template<>
struct serializer< std::string_view >
{
	static obstream& put( obstream& os, std::string_view const& value )
	{
		if ( value.size() <= 31 )
		{
			std::uint8_t code = 0xa0 | static_cast< std::uint8_t >( value.size() );
			os.put( code );
			os.putn( value.data(), value.size() );
		}
		else if ( value.size() <= std::numeric_limits< std::uint8_t >::max() )
		{
			os.put( typecode::str_8 );
			os.put_num( static_cast< std::uint8_t >( value.size() ) );
			os.putn( value.data(), value.size() );
		}
		else if ( value.size() <= std::numeric_limits< std::uint16_t >::max() )
		{
			os.put( typecode::str_16 );
			os.put_num( static_cast< std::uint16_t >( value.size() ) );
			os.putn( value.data(), value.size() );
		}
		else if ( value.size() <= std::numeric_limits< std::uint32_t >::max() )
		{
			os.put( typecode::str_32 );
			os.put_num( static_cast< std::uint32_t >( value.size() ) );
			os.putn( value.data(), value.size() );
		}
		else
		{
			throw type_error( "string length exceeds limit" );
		}
		return os;
	}
};

template<>
struct serializer< nodeoze::string_alias >
{
	static obstream& put( obstream& os, nodeoze::string_alias const& value )
	{
		return serializer< std::string_view >::put( os, value.view() );
	}
};

template<>
struct serializer< std::string >
{
	static obstream& put( obstream& os, std::string const& value )
	{
		if ( value.size() <= 31 )
		{
			std::uint8_t code = 0xa0 | static_cast< std::uint8_t >( value.size() );
			os.put( code );
			os.putn( value.data(), value.size() );
		}
		else if ( value.size() <= std::numeric_limits< std::uint8_t >::max() )
		{
			os.put( typecode::str_8 );
			os.put_num( static_cast< std::uint8_t >( value.size() ) );
			os.putn( value.data(), value.size() );
		}
		else if ( value.size() <= std::numeric_limits< std::uint16_t >::max() )
		{
			os.put( typecode::str_16 );
			os.put_num( static_cast< std::uint16_t >( value.size() ) );
			os.putn( value.data(), value.size() );
		}
		else if ( value.size() <= std::numeric_limits< std::uint32_t >::max() )
		{
			os.put( typecode::str_32 );
			os.put_num( static_cast< std::uint32_t >( value.size() ) );
			os.putn( value.data(), value.size() );
		}
		else
		{
			throw type_error( "string length exceeds limit" );
		}
		return os;
	}
};

template< class T >
struct serializer< std::shared_ptr< T > >
{
	static obstream& put( obstream& os, std::shared_ptr< T > const& ptr )
	{
		os.write_shared_ptr( ptr );
		return os;
	}
};

template< class T >
struct serializer< std::unique_ptr< T > >
{
	static obstream& put( obstream& os, std::unique_ptr< T > const& ptr )
	{
		os.write_as_unique_pointer( ptr.get() );
		return os;
	}
};
	
template< class T >
struct serializer< T, typename std::enable_if_t< std::is_enum< T >::value > >
{
	static obstream& put( obstream& os, T val )
	{
		os << static_cast< typename std::underlying_type< T >::type >( val );
		return os;
	}
};

template<>
struct serializer< nodeoze::buffer >
{
	static obstream& put( obstream& os, nodeoze::buffer const& val )
	{
		os.write_blob( val );
		return os;
	}
};

template<>
struct serializer< std::error_code >
{
	static obstream& put( obstream& os, std::error_code const& value )
	{
		os.write_error_code( value );
		return os;
	}
};

template< class Derived, class Base >
struct base_serializer< Derived, Base, typename std::enable_if_t<
	has_serialize_impl_method< Base >::value > >
{
	static obstream& put( obstream& os, Derived const& v )
	{
		return v.Base::serialize_impl( os );
	}
};

template< class Derived, class Base >
struct base_serializer< Derived, Base, typename std::enable_if_t<
	! has_serialize_impl_method< Base >::value && has_serialize_method< Base >::value > >
{
	static obstream& put( obstream& os, Derived const& v )
	{
		return v.Base::serialize( os );
	}
};

template< class Derived, class Base >
struct base_serializer< Derived, Base, typename std::enable_if_t<
	! has_serialize_impl_method< Base >::value && ! has_serialize_method< Base >::value && has_serializer< Base >::value > >
{
	static obstream& put( obstream& os, Derived const& v )
	{
		return serializer< Base >::put( os, static_cast< const Base& >( v ) );
	}
};

template< class T >
inline obstream&
obstream::write_shared_ptr( std::shared_ptr< T > ptr )
{
	if ( !ptr )
	{
		write_null_ptr();
	}
	else
	{
		if ( m_ptr_deduper )
		{
			saved_ptr_info info;
			if ( m_ptr_deduper->is_saved( ptr, info ) )
			{
				write_array_header( 2 );
				*this << info.first;	// type tag
				*this << info.second;	// saved index
			}
			else
			{
				auto tag = m_context->get_type_tag( typeid( *ptr ) );
				write_array_header( 2 );
				*this << tag;
				*this << *ptr;
				m_ptr_deduper->save_ptr( ptr, tag );
			}
		}
		else
		{
			auto tag = m_context->get_type_tag( typeid( *ptr ) );
			write_array_header( 2 );
			*this << tag;
			*this << *ptr;
		}
	}
	return *this;
}

template< class T >
inline obstream&
obstream::write_as_unique_pointer( T *ptr )
{
	if ( !ptr )
	{
		write_null_ptr();
	}
	else
	{
		auto tag = m_context->get_type_tag( typeid( *ptr ) );
		write_array_header( 2 );
		*this << tag;
		*this << *ptr;
	}
	return *this;
}

} // namespace bstream
} // namespace nodeoze

#endif /* NODEOZE_BSTREAM_OBSTREAM_H */

