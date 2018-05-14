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

#include <cstdint>
#include <vector>
#include <nodeoze/bstream/numstream.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/typecode.h>
#include <nodeoze/bstream/ibstream.h>
#include <iterator>
#include <memory>

namespace nodeoze
{
namespace bstream
{

class obstream;

class obs_context
{
public:
	using ptr = std::unique_ptr< obs_context >;

	virtual bool maybe_write_shared_ptr( obstream& os, void *ptr) = 0;

	virtual ~obs_context() {}
};

class obs_ptr_context : public obs_context
{
public:
	using ptr = std::unique_ptr< obs_ptr_context >;

	virtual bool maybe_write_shared_ptr( obstream& os, void *ptr) override;

private:
	std::unordered_map<void*,std::size_t> 	m_shared_pointers;
};

class obstream : public onumstream
{
protected:

//	inline obstream() {} // TODO: decide whether to delete certain ctors
		
public:	
/*
	inline 
	obstream(bend::order order = bend::order::big ) 
	:
	onumstream{ order }
	{}
*/
	inline
	obstream( std::unique_ptr< std::streambuf > strmbuf, obs_context::ptr context = nullptr )
	:
	onumstream{ std::move( strmbuf ) },
	m_context{ std::move( context ) }
	{}

	inline void
	set_context( obs_context::ptr context )
	{
		m_context = std::move( context );
	}

	inline void
	set_ptr_context()
	{
		m_context = std::make_unique< obs_ptr_context >();
	}
/*
	inline
	obstream( std::unique_ptr< omembuf > strmbuf, bend::order order = bend::order::big )
	:
	onumstream{ std::move( strmbuf ), order }
	{}

	inline
	obstream( buffer const& buf, bend::order order = bend::order::big )
	:
	onumstream{ buf, order }
	{}

	inline
	obstream( buffer&& buf, bend::order order = bend::order::big )
	:
	onumstream{ std::move( buf ), order }
	{}

	inline 
	obstream( size_type capacity, buffer::policy pol = buffer::policy::copy_on_write, bend::order order = bend::order::big )
	:
	onumstream{ capacity, pol, order }
	{}
*/
	inline obstream&
	write_map_header(std::uint32_t size)
	{
		if (size <= 15)
		{
			std::uint8_t code = 0x80 | static_cast<std::uint8_t>(size);
			put(code);
		}
		else if (size <= std::numeric_limits<std::uint16_t>::max())
		{
			put(typecode::map_16);
			put_num(static_cast<std::uint16_t>(size));
		}
		else
		{
			put(typecode::map_32);
			put_num(static_cast<std::uint32_t>(size));
		}
		return *this;
	}

	inline obstream&
	write_map_header(std::uint32_t size, std::error_code& ec )
	{
		clear_error( ec );
		try
		{
			write_map_header( size );
		}
		catch ( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}

	inline obstream&
	write_array_header(std::uint32_t size)
	{
		if (size <= 15)
		{
			std::uint8_t code = 0x90 | static_cast<std::uint8_t>(size);
			put(code);
		}
		else if (size <= std::numeric_limits<std::uint16_t>::max())
		{
			put(typecode::array_16);
			put_num(static_cast<std::uint16_t>(size));
		}
		else
		{
			put(typecode::array_32);
			put_num(static_cast<std::uint32_t>(size));
		}
		return *this;
	}      

	inline obstream&
	write_array_header(std::uint32_t size, std::error_code& ec )
	{
		clear_error( ec );
		try
		{
			write_array_header( size );
		}
		catch ( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}

	inline obstream&
	write_blob_header(std::uint32_t size)
	{
		if (size <= std::numeric_limits<std::uint8_t>::max())
		{
			put(typecode::bin_8);
			put_num(static_cast<std::uint8_t>(size));
		}
		else if (size <= std::numeric_limits<std::uint16_t>::max())
		{
			put(typecode::bin_16);
			put_num(static_cast<std::uint16_t>(size));
		}
		else
		{
			put(typecode::bin_32);
			put_num(static_cast<std::uint32_t>(size));
		}
		return *this;
	}

	inline obstream&
	write_blob_header(std::uint32_t size, std::error_code& ec )
	{
		clear_error( ec );
		try
		{
			write_blob_header( size );
		}
		catch ( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}

	inline obstream&
	write_blob_body(const void* p, std::size_t size)
	{
		putn(p, size);
		return *this;
	}

	inline obstream&
	write_blob_body(const void* p, std::size_t size, std::error_code& ec )
	{
		putn(p, size, ec );
		return *this;
	}

	inline obstream&
	write_blob_body( nodeoze::buffer const& blob )
	{
		putn( blob.const_data(), blob.size() );
		return *this;
	}

	inline obstream&
	write_blob_body( nodeoze::buffer const& blob, std::error_code& ec )
	{
		putn( blob.const_data(), blob.size(), ec );
		return *this;
	}

	inline obstream&
	write_blob(const void* p, std::size_t size)
	{
		write_blob_header(size);
		write_blob_body(p, size);
		return *this;
	}

	inline obstream&
	write_blob( const void* p, std::size_t size, std::error_code& ec )
	{
		write_blob_header( size, ec );
		if ( ec ) goto exit;

		write_blob_body( p, size, ec );

	exit:
		return *this;
	}

	inline obstream&
	write_blob( nodeoze::buffer const& buf )
	{
		write_blob_header( buf.size() );
		write_blob_body( buf );
		return *this;
	}

	inline obstream&
	write_blob( nodeoze::buffer const& buf, std::error_code& ec )
	{
		write_blob_header( buf.size(), ec );
		if ( ec ) goto exit;

		write_blob_body( buf, ec );

	exit:
		return *this;
	}

	inline obstream&
	write_object_header(std::uint32_t size)
	{
		return write_array_header(size);
	}

	inline obstream&
	write_object_header( std::uint32_t size, std::error_code& ec )
	{
		return write_array_header(size, ec );
	}

	template< class T >
	inline typename std::enable_if_t< std::is_arithmetic< T >::value, obstream& >	
	write_ext( std::uint8_t ext_type, T value )
	{
		put_num( detail::fixext_typecode< sizeof( T ) >::value );
		put_num( ext_type );
		put_num( value );
		return *this;
	}

	template< class T >
	inline typename std::enable_if_t< std::is_arithmetic< T >::value, obstream& >	
	write_ext( std::uint8_t ext_type, T value, std::error_code& ec )
	{
		put_num( detail::fixext_typecode< sizeof( T ) >::value, ec );
		if ( ec ) goto exit;

		put_num( ext_type, ec );
		if ( ec ) goto exit;

		put_num( value, ec );

	exit:
		return *this;
	}

/*
	template< class T >
	inline typename std::enable_if_t< std::is_arithmetic< T >::value && sizeof( T ) == 1, obstream& >	
	write_ext( std::uint8_t ext_type, T value )
	{
		put_num( typecode::fixext_1 );
		put_num( ext_type );
		put_num( value );
		return *this;
	}

	template< class T >
	inline typename std::enable_if_t< std::is_arithmetic< T >::value && sizeof( T ) == 2, obstream& >	
	write_ext( std::uint8_t ext_type, T value )
	{
		put_num( typecode::fixext_2 );
		put_num( ext_type );
		put_num( value );
		return *this;
	}

	template< class T >
	inline typename std::enable_if_t< std::is_arithmetic< T >::value && sizeof( T ) == 4, obstream& >	
	write_ext( std::uint8_t ext_type, T value )
	{
		put_num( typecode::fixext_4 );
		put_num( ext_type );
		put_num( value );
		return *this;
	}

	template< class T >
	inline typename std::enable_if_t< std::is_arithmetic< T >::value && sizeof( T ) == 8, obstream& >	
	write_ext( std::uint8_t ext_type, T value )
	{
		put_num( typecode::fixext_8 );
		put_num( ext_type );
		put_num( value );
		return *this;
	}
*/
	inline obstream&
	write_ext( std::uint8_t ext_type, buffer const& buf )
	{
		auto size = buf.size();
		switch ( size )
		{
			case 1:
				put_num( typecode::fixext_1 ).put_num( ext_type ).put_num( buf[ 0 ] );
				break;
			case 2:
				put_num( typecode::fixext_2 ).put_num( ext_type ).put_num( buf[ 0 ] ).put_num( buf[ 1 ] );
				break;
			case 4:
				put_num( typecode::fixext_4 ).put_num( ext_type ).putn( buf.const_data(), 4 );
				break;
			case 8:
				put_num( typecode::fixext_8 ).put_num( ext_type ).putn( buf.const_data(), 8 );
				break;
			case 16:
				put_num( typecode::fixext_16 ).put_num( ext_type ).putn( buf.const_data(), 16 );
				break;
			default:
				if ( size <= std::numeric_limits< std::uint8_t >::max() )
				{
					put_num( typecode::ext_8 ).put_num( static_cast< std::uint8_t >( size ) ).put_num( ext_type );
				}
				else if ( size <= std::numeric_limits< std::uint16_t >::max() )
				{
					put_num( typecode::ext_16 ).put_num( static_cast< std::uint16_t >( size ) ).put_num( ext_type );
				}
				else if ( size <= std::numeric_limits< std::uint32_t >::max() )
				{
					put_num( typecode::ext_32 ).put_num( static_cast< std::uint32_t >( size ) ).put_num( ext_type );
				}
				else
				{
					throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
				}
				putn( buf.const_data(), size );
		}
		return *this;
	}

	inline obstream&
	write_ext( std::uint8_t ext_type, buffer const& buf, std::error_code& ec )
	{
		clear_error( ec );
		try
		{
			write_ext( ext_type, buf );
		}
		catch ( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}

	inline obstream&
	write_ext( std::uint8_t ext_type, std::vector< std::uint8_t > const& vec )
	{
		auto size = vec.size();
		switch ( size )
		{
			case 1:
				put_num( typecode::fixext_1 ).put_num( ext_type ).put_num( vec[ 0 ] );
				break;
			case 2:
				put_num( typecode::fixext_2 ).put_num( ext_type ).put_num( vec[ 0 ] ).put_num( vec[ 1 ] );
				break;
			case 4:
				put_num( typecode::fixext_4 ).put_num( ext_type ).putn( vec.data(), 4 );
				break;
			case 8:
				put_num( typecode::fixext_8 ).put_num( ext_type ).putn( vec.data(), 8 );
				break;
			case 16:
				put_num( typecode::fixext_16 ).put_num( ext_type ).putn( vec.data(), 16 );
				break;
			default:
				if ( size <= std::numeric_limits< std::uint8_t >::max() )
				{
					put_num( typecode::ext_8 ).put_num( static_cast< std::uint8_t >( size ) ).put_num( ext_type );
				}
				else if ( size <= std::numeric_limits< std::uint16_t >::max() )
				{
					put_num( typecode::ext_16 ).put_num( static_cast< std::uint16_t >( size ) ).put_num( ext_type );
				}
				else if ( size <= std::numeric_limits< std::uint32_t >::max() )
				{
					put_num( typecode::ext_32 ).put_num( static_cast< std::uint32_t >( size ) ).put_num( ext_type );
				}
				else
				{
					throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
				}
				putn( vec.data(), size );
		}
		return *this;
	}

	inline obstream&
	write_ext( std::uint8_t ext_type, std::vector< std::uint8_t > const& vec, std::error_code& ec )
	{
		clear_error( ec );
		try
		{
			write_ext( ext_type, vec );
		}
		catch( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}

	inline obstream&
	write_ext_header( std::uint8_t ext_type, std::uint32_t size )
	{
		switch ( size )
		{
			case 1:
				put_num( typecode::fixext_1 ).put_num( ext_type );
				break;
			case 2:
				put_num( typecode::fixext_2 ).put_num( ext_type );
				break;
			case 4:
				put_num( typecode::fixext_4 ).put_num( ext_type );
				break;
			case 8:
				put_num( typecode::fixext_8 ).put_num( ext_type );
				break;
			case 16:
				put_num( typecode::fixext_16 ).put_num( ext_type );
				break;
			default:
				if ( size <= std::numeric_limits< std::uint8_t >::max() )
				{
					put_num( typecode::ext_8 ).put_num( static_cast< std::uint8_t >( size ) ).put_num( ext_type );
				}
				else if ( size <= std::numeric_limits< std::uint16_t >::max() )
				{
					put_num( typecode::ext_16 ).put_num( static_cast< std::uint16_t >( size ) ).put_num( ext_type );
				}
				else if ( size <= std::numeric_limits< std::uint32_t >::max() )
				{
					put_num( typecode::ext_32 ).put_num( static_cast< std::uint32_t >( size ) ).put_num( ext_type );
				}
				else
				{
					throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
				}
		}
		return *this;
	}

	inline obstream&
	write_ext_header( std::uint8_t ext_type, std::uint32_t size, std::error_code& ec  )
	{
		clear_error( ec );
		try
		{
			write_ext_header( ext_type, size );
		}
		catch( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}	

	obstream&
	write_ext( std::uint8_t ext_type, void* data, std::uint32_t size )
	{
		write_ext_header( ext_type, size );
		putn( data, size );
		return *this;
	}

	obstream&
	write_ext( std::uint8_t ext_type, void* data, std::uint32_t size, std::error_code& ec )
	{
		write_ext_header( ext_type, size, ec );
		if ( ec ) goto exit;
		putn( data, size, ec );
	exit:
		return *this;
	}

	inline obstream&
	write_null_ptr()
	{
		write_array_header(1);
		put(typecode::nil);
		return *this;
	}

	inline obstream&
	write_null_ptr( std::error_code& ec )
	{
		write_array_header( 1, ec );
		if ( ec ) goto exit;
		put( typecode::nil, ec );
	exit:
		return *this;
	}

	inline obstream&
	write_nil()
	{
		put(typecode::nil);
		return *this;
	}

	inline obstream&
	write_nil( std::error_code& ec )
	{
		put( typecode::nil, ec );
		return *this;
	}


	template<class T>
	inline obstream&
	write_shared_ptr(std::shared_ptr<T> ptr)
	{
		if (!ptr)
		{
			write_null_ptr();
		}
		else
		{
			if ( !maybe_write_shared_ptr( ptr.get() ) )
			{
				*this << *ptr;
			}
		}
		return *this;
	}

	template<class T>
	inline obstream&
	write_shared_ptr( std::shared_ptr<T> ptr, std::error_code& ec )
	{
		clear_error( ec );
		try
		{
			write_shared_ptr( ptr );
		}
		catch ( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}
		
	template<class T>
	inline obstream&
	write_as_unique_pointer(T const& obj)
	{
		write_array_header(1);
		*this << obj;
		return *this;
	}

	template<class T>
	inline obstream&
	write_as_unique_pointer( T const& obj, std::error_code& ec )
	{
		clear_error( ec );
		try
		{
			write_as_unique_pointer( obj );
		}
		catch ( std::system_error const& e )
		{
			ec = e.code();
		}
		return *this;
	}

protected:

	bool maybe_write_shared_ptr( void *ptr )
	{
		if ( ! m_context )
		{
			write_array_header(1);
			return false;
		}
		else
		{
			return m_context->maybe_write_shared_ptr( *this, ptr );
		}
	}
		
private:
		
	static inline std::size_t map_header_size(std::size_t map_size)
	{
		if (map_size < 16) 
			return 1;
		else if (map_size <= std::numeric_limits<std::uint16_t>::max())
			return 3;
		else return 5;
	}
		
	static inline std::size_t array_header_size(std::size_t array_size)
	{
		if (array_size < 16) 
			return 1;
		else if (array_size <= std::numeric_limits<std::uint16_t>::max())
			return 3;
		else return 5;
	}
		
	static inline std::size_t blob_header_size(std::size_t blob_size)
	{
		if (blob_size < 256) 
			return 2;
		else if (blob_size <= std::numeric_limits<std::uint16_t>::max())
			return 3;
		else return 5;
	}

	obs_context::ptr						m_context;
};

	namespace detail
	{
		template<class T>
		static auto test_serialize_method(int)
			-> utils::sfinae_true_if<decltype( std::declval<T>().serialize(std::declval<obstream&>()) )>;
		template<class>
		static auto test_serialize_method(long) -> std::false_type;
	} // namespace detail

	template<class T>
	struct has_serialize_method : decltype(detail::test_serialize_method<T>(0)) {};

	namespace detail
	{
		template<class T>
		static auto test_serializer(int)
			-> utils::sfinae_true_if<decltype(serializer<T>::put( std::declval<obstream&>(), std::declval<T>() ))>;
		template<class>
		static auto test_serializer(long) -> std::false_type;
	} // namespace detail

	template<class T>
	struct has_serializer : decltype(detail::test_serializer<T>(0)) {};

	namespace detail
	{
		template<class T>
		static auto test_obstream_insertion_operator(int)
			-> utils::sfinae_true_if<decltype( std::declval<obstream&>() << std::declval<T>() )>;
		template<class>
		static auto test_obstream_insertion_operator(long) -> std::false_type;
	} // namespace detail

	template<class T>
	struct has_obstream_insertion_operator : decltype(detail::test_obstream_insertion_operator<T>(0)) {};
	
	template<class T>
	struct serializer<T, typename std::enable_if_t<has_serialize_method<T>::value>>
	{
		static inline obstream& put(obstream& os, T const& value)
		{
			return value.serialize(os);
		}
	};
	
    template<class T>
    inline 
    typename std::enable_if_t
    < 
		has_serializer<T>::value,
        obstream& 
    >
    operator<<(obstream& os, const T& value)
    {
        return serializer<T>::put(os, value);
    }
	
	template<class T, class Enable = void>
	struct is_serializable : public std::false_type {};
	
    template<class T>
	struct is_serializable<T, std::enable_if_t<has_serializer<T>::value>> : public std::true_type {};

	template<class T>
	struct serializer<T, 
			typename std::enable_if_t<
				std::is_integral<T>::value && 
				!std::numeric_limits<T>::is_signed>>
	{
		inline static obstream& put(obstream& os, T value)
		{
			if (value <= typecode::positive_fixint_max)
			{
				std::uint8_t val = static_cast<std::uint8_t>(value);
				os.put(val);
			}
			else if (value <= std::numeric_limits<std::uint8_t>::max())
			{
				os.put(typecode::uint_8);
				os.put(static_cast<std::uint8_t>(value));
			}
			else if (value <= std::numeric_limits<std::uint16_t>::max())
			{
				os.put(typecode::uint_16);
				os.put_num(static_cast<std::uint16_t>(value));
			}
			else if (value <= std::numeric_limits<std::uint32_t>::max())
			{
				os.put(typecode::uint_32);
				os.put_num(static_cast<std::uint32_t>(value));
			}
			else
			{
				os.put(typecode::uint_64);
				os.put_num(static_cast<std::uint64_t>(value));
			}
			return os;
		}
	};
	
	template<class T>
	struct serializer<T, 
			typename std::enable_if_t<
				std::is_integral<T>::value && 
				std::numeric_limits<T>::is_signed>>
	{
		inline static obstream& put(obstream& os, T value)
		{
			if (value >= 0)
			{
				std::uint64_t uvalue = static_cast<std::uint64_t>(value);

				if (uvalue <= typecode::positive_fixint_max)
				{
					std::uint8_t val = static_cast<std::uint8_t>(uvalue);
					os.put(val);
				}
				else if (uvalue <= std::numeric_limits<std::uint8_t>::max())
				{
					os.put(typecode::uint_8);
					os.put(static_cast<std::uint8_t>(uvalue));
				}
				else if (uvalue <= std::numeric_limits<std::uint16_t>::max())
				{
					os.put(typecode::uint_16);
					os.put_num(static_cast<std::uint16_t>(uvalue));
				}
				else if (uvalue <= std::numeric_limits<std::uint32_t>::max())
				{
					os.put(typecode::uint_32);
					os.put_num(static_cast<std::uint32_t>(uvalue));
				}
				else
				{
					os.put(typecode::uint_64);
					os.put_num(uvalue);
				}
			}
			else
			{
				if (value >= -32)
				{
					os.put(static_cast<std::uint8_t>(value));
				}
				else if (value >= std::numeric_limits<std::int8_t>::min())
				{
					os.put(typecode::int_8);
					os.put_num(static_cast<std::int8_t>(value));
				}
				else if (value >= std::numeric_limits<std::int16_t>::min())
				{
					os.put(typecode::int_16);
					os.put_num(static_cast<std::int16_t>(value));
				}
				else if (value >= std::numeric_limits<std::int32_t>::min())
				{
					os.put(typecode::int_32);
					os.put_num(static_cast<std::int32_t>(value));
				}
				else
				{
					os.put(typecode::int_64);
					os.put_num(static_cast<std::int64_t>(value));
				}
			}
			return os;
		}
	};

	template<>
	struct serializer<float>
	{
		inline static obstream& put(obstream& os, float value)
		{
			os.put(typecode::float_32);
			os.put_num(value);
			return os;
		}
	};

	template<>
	struct serializer<double>
	{
		inline static obstream& put(obstream& os, double value)
		{
			os.put(typecode::float_64);
			os.put_num(value);
			return os;
		}
	};

	template<>
	struct serializer<bool>
	{
		static inline obstream& put(obstream& os, bool value)
		{
			if (value)
			{
				os.put(typecode::bool_true);
			}
			else
			{
				os.put(typecode::bool_false);
			}
			return os;
		}
	};

	template<>
	struct serializer<std::string_view>
	{
		static inline obstream& put(obstream& os, std::string_view const& value)
		{
			if (value.size() <= 31)
			{
				std::uint8_t code = 0xa0 | static_cast<std::uint8_t>(value.size());
				os.put(code);
				os.putn(value.data(), value.size());
			}
			else if (value.size() <= std::numeric_limits<std::uint8_t>::max())
			{
				os.put(typecode::str_8);
				os.put_num(static_cast<std::uint8_t>(value.size()));
				os.putn(value.data(), value.size());
			}
			else if (value.size() <= std::numeric_limits<std::uint16_t>::max())
			{
				os.put(typecode::str_16);
				os.put_num(static_cast<std::uint16_t>(value.size()));
				os.putn(value.data(), value.size());
			}
			else if (value.size() <= std::numeric_limits<std::uint32_t>::max())
			{
				os.put(typecode::str_32);
				os.put_num(static_cast<std::uint32_t>(value.size()));
				os.putn(value.data(), value.size());
			}
			else
			{
				throw type_error("string length exceeds limit");
			}
			return os;
		}
	};
	
	template<>
	struct serializer<nodeoze::string_alias>
	{
		static inline obstream& put( obstream& os, nodeoze::string_alias const& value )
		{
			return serializer<std::string_view>::put( os, value.view() );
		}
	};

	template<>
	struct serializer<std::string>
	{
		static inline obstream& put(obstream& os, std::string const& value)
		{
			if (value.size() <= 31)
			{
				std::uint8_t code = 0xa0 | static_cast<std::uint8_t>(value.size());
				os.put(code);
				os.putn(value.data(), value.size());
			}
			else if (value.size() <= std::numeric_limits<std::uint8_t>::max())
			{
				os.put(typecode::str_8);
				os.put_num(static_cast<std::uint8_t>(value.size()));
				os.putn(value.data(), value.size());
			}
			else if (value.size() <= std::numeric_limits<std::uint16_t>::max())
			{
				os.put(typecode::str_16);
				os.put_num(static_cast<std::uint16_t>(value.size()));
				os.putn(value.data(), value.size());
			}
			else if (value.size() <= std::numeric_limits<std::uint32_t>::max())
			{
				os.put(typecode::str_32);
				os.put_num(static_cast<std::uint32_t>(value.size()));
				os.putn(value.data(), value.size());
			}
			else
			{
				throw type_error("string length exceeds limit");
			}
			return os;
		}
	};
	
	template<class T>
	struct serializer<std::shared_ptr<T>>
	{
		inline static obstream& put(obstream& os, std::shared_ptr<T> ptr)
		{
			os.write_shared_ptr(ptr);
			return os;
		}
	};
	
	template<class T>
	struct serializer<std::unique_ptr<T>>
	{
		inline static obstream& put(obstream& os, std::unique_ptr<T> const& ptr)
		{
			if (!ptr)
			{
				os.write_null_ptr();
			}
			else
			{
				os.write_as_unique_pointer(*ptr);
			}
			return os;
		}
	};
	
    
    template<class T, class Alloc>
    struct serializer<std::deque<T, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::deque<T, Alloc>& deq)
        {
            os.write_array_header(deq.size());
			for (auto it = deq.begin(); it != deq.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
    template<class T, class Alloc>
    struct serializer<std::forward_list<T, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::forward_list<T, Alloc>& flist)
        {
			auto length = std::distance(flist.begin(), flist.end());
            os.write_array_header(length);
			for (auto it = flist.begin(); it != flist.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
    template<class T, class Alloc>
    struct serializer<std::list<T, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::list<T, Alloc>& lst)
        {
            os.write_array_header(lst.size());
			for (auto it = lst.begin(); it != lst.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
    template<class T, class Compare, class Alloc>
    struct serializer<std::set<T, Compare, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::set<T, Compare, Alloc>& s)
        {
            os.write_array_header(s.size());
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
    template<class T, class Hash, class Equal, class Alloc>
    struct serializer<std::unordered_set<T, Hash, Equal, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::unordered_set<T, Hash, Equal, Alloc>& s)
        {
            os.write_array_header(s.size());
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
    template<class T, class Compare, class Alloc>
    struct serializer<std::multiset<T, Compare, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::multiset<T, Compare, Alloc>& s)
        {
            os.write_array_header(s.size());
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
    template<class T, class Hash, class Equal, class Alloc>
    struct serializer<std::unordered_multiset<T, Hash, Equal, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::unordered_multiset<T, Hash, Equal, Alloc>& s)
        {
            os.write_array_header(s.size());
			for (auto it = s.begin(); it != s.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
    template<class T, class Alloc>
    struct serializer<std::vector<T, Alloc>>
    {
        inline static obstream& put(obstream& os, const std::vector<T, Alloc>& vec)
        {
            os.write_array_header(vec.size());
			for (auto it = vec.begin(); it != vec.end(); ++it)
			{
				os << *it;
			}
            return os;
        }
    };
	
	template<class K, class V, class Compare, class Alloc>
	struct serializer<std::map<K, V, Compare, Alloc>>
	{
		static inline obstream& put(obstream& os, std::map<K, V, Compare, Alloc> const& mp)
		{
			os.write_array_header(mp.size());
			for (auto it = mp.begin(); it != mp.end(); ++it)
			{
				os.write_array_header(2);
				os << it->first;
				os << it->second;
			}
			return os;
		}
	};
		
	template<class K, class V, class Compare, class Alloc>
	struct serializer<std::multimap<K, V, Compare, Alloc>>
	{
		static inline obstream& put(obstream& os, std::multimap<K, V, Compare, Alloc> const& mp)
		{
			os.write_array_header(mp.size());
			for (auto it = mp.begin(); it != mp.end(); ++it)
			{
				os.write_array_header(2);
				os << it->first;
				os << it->second;
			}
			return os;
		}
	};
		
	template<class K, class V, class Hash, class Equal, class Alloc>
	struct serializer<std::unordered_map<K, V, Hash, Equal, Alloc>>
	{
		static inline obstream& put(obstream& os, std::unordered_map<K, V, Hash, Equal, Alloc> const& map)
		{
			os.write_array_header(map.size());
			for (auto it = map.begin(); it != map.end(); ++it)
			{
//				os.write_array_header(2);
//				os << it->first;
//				os << it->second;
				os << *it;
			}
			return os;
		}
	};
		
	template<class K, class V, class Hash, class Equal, class Alloc>
	struct serializer<std::unordered_multimap<K, V, Hash, Equal, Alloc>>
	{
		static inline obstream& put(obstream& os, std::unordered_multimap<K, V, Hash, Equal, Alloc> const& map)
		{
			os.write_array_header(map.size());
			for (auto it = map.begin(); it != map.end(); ++it)
			{
				os.write_array_header(2);
				os << it->first;
				os << it->second;
			}
			return os;
		}
	};
	
	template<class T1, class T2>
	struct serializer<std::pair<T1, T2>>
	{
		static inline obstream& put(obstream& os, std::pair<T1, T2> const& p)
		{
			os.write_array_header(2);
			os << p.first;
			os << p.second;
			return os;
		}
	};
	
	template<class... Args>
	struct serializer<std::tuple<Args...>>
	{
		using tuple_type = std::tuple<Args...>;
		static inline obstream& put(obstream& os, tuple_type const& tup)
		{
			os.write_array_header(std::tuple_size<tuple_type>::value);
			put_members<0, Args...>(os, tup);
			return os;
		}
		
		template<unsigned int N, class First, class... Rest>
		static inline typename std::enable_if<(sizeof...(Rest) > 0)>::type
		put_members(obstream& os, tuple_type const& tup)
		{
			os << std::get<N>(tup);
			put_members<N+1, Rest...>(os, tup);
		}
		
		template<unsigned int N, class T>
		static inline void
		put_members(obstream& os, tuple_type const& tup)
		{
			os << std::get<N>(tup);
		}		
	};
		
	template<class T>
	struct serializer<T, typename std::enable_if_t<std::is_enum<T>::value>>
	{
		inline static obstream& put(obstream& os, T val)
		{		
			os << static_cast<typename std::underlying_type<T>::type>(val);
			return os;
		}
	};
	
	template<class Rep, class Ratio>
	struct serializer<std::chrono::duration<Rep,Ratio>>
	{
		using duration_type = std::chrono::duration<Rep,Ratio>;
		inline static obstream& put(obstream& os, duration_type val)
		{
			os << val.count();
			return os;
		}
	};
	
	template<class Clock, class Duration>
	struct serializer<std::chrono::time_point<Clock,Duration>>
	{
		using time_point_type = std::chrono::time_point<Clock,Duration>;
		inline static obstream& put(obstream& os, time_point_type val)
		{
			os << val.time_since_epoch().count();
			return os;
		}
	};

	template<>
	struct serializer< nodeoze::buffer >
	{
		inline static obstream& put( obstream& os, nodeoze::buffer const& val )
		{
			os.write_blob( val );
			return os;
		}
	};
	
} // namespace bstream
} // namespace nodeoze

#endif /* NODEOZE_BSTREAM_OBSTREAM_H */

