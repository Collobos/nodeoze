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

#ifndef _nodeoze_any_h
#define _nodeoze_any_h

#include <nodeoze/nbuffer.h>
#include <nodeoze/nstream.h>
#include <nodeoze/nstring.h>
#include <nodeoze/ndeque.h>
#include <nodeoze/nbase64.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/unordered_map.h>
#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <nodeoze/nlog.h>
#include <streambuf>
#include <iostream>
#include <cctype>
#include <chrono>
#include <string>
#include <ctime>

#if defined( __APPLE__ )
#	define NODEOZE_SIZE_T_IS_UNIQUE_TYPE
#	define NODEOZE_LONG_IS_UNIQUE_TYPE
#	define NODEOZE_ULONG_IS_UNIQUE_TYPE
#endif

#if defined( WIN32 )
#	define NODEOZE_LONG_IS_UNIQUE_TYPE
#	define NODEOZE_ULONG_IS_UNIQUE_TYPE
#endif

#if defined( __linux__ )
#	define NODEOZE_LONGLONG_IS_UNIQUE_TYPE
#	define NODEOZE_ULONGLONG_IS_UNIQUE_TYPE
#endif

namespace nodeoze {

class any
{
public:

	typedef std::string							string_type;
	typedef nodeoze::buffer						blob_type;
	typedef deque< any >						array_type;
	typedef unordered_map< std::string, any >	object_type;
	typedef std::vector<std::string>			keys;
	
	struct find_t;

	enum class type_t
	{
		null,
		boolean,
		integer,
		real,
		string,
		blob,
		array,
		object
	};
	
	template< class T >
	static inline any
	replace( const std::string &path, T val )
	{
		any patches;
		
		add_replace< T >( patches, path, std::move( val ) );
		
		return patches;
	}
	
	template< class T >
	static inline void
	add_replace( any &patches, const std::string &path, T val )
	{
		add_patch( patches, "replace", path, std::string(), val, {} );
	}
	
	static inline any
	diff( const any& lhs, const any& rhs, const std::vector< std::string > &exclusions, bool case_sensitive )
	{
		any patches;

		create_patches( patches, std::string(), lhs, rhs, exclusions, case_sensitive );

		return patches;
	}
	
	static inline any
	patch( const any &root, const any &patches )
	{
		any tmp( root );
		
		tmp.patch( patches );
		
		return tmp;
	}

	static const any&
	null();
	
	static any
	array();
	
	static any
	object();
	
	inline static any
	build( std::initializer_list< any > list, bool type_deduction = true, type_t manual_type = type_t::array )
	{
		return any( list, type_deduction, manual_type );
	}
	
	inline any()
	:
		m_type( type_t::null )
	{
	}
	
	inline any( bool val )
	:
		m_type( type_t::boolean )
	{
		m_data.m_bool = val;
	}

	template< typename T >
	inline any( T val, typename std::enable_if< std::is_enum< T >::value >::type* = 0 )
	:
		m_type( type_t::integer )
	{
		m_data.m_integer = static_cast< std::int64_t >( val );
	}

	template< typename T >
	inline any( T val, typename std::enable_if< std::is_integral< T >::value >::type* = 0 )
	:
		m_type( type_t::integer )
	{
		m_data.m_integer = static_cast< std::int64_t >( val );
	}

	inline any( std::chrono::system_clock::time_point val )
	:
		m_type( type_t::integer )
	{
		m_data.m_integer = static_cast< std::int64_t >( std::chrono::system_clock::to_time_t( val ) );
	}
	
	inline any( float val )
	:
		m_type( type_t::real )
	{
		m_data.m_real = val;
	}
	
	inline any( double val )
	:
		m_type( type_t::real )
	{
		m_data.m_real = val;
	}
	
	inline any( char *val )
	:
		m_type( type_t::string )
	{
		new ( &m_data.m_string ) string_type( val );
	}
	
	inline any( const char *val )
	:
		m_type( type_t::string )
	{
		new ( &m_data.m_string ) string_type( val );
	}
	
	inline any( const string_type &val )
	:
		m_type( type_t::string )
	{
		new ( &m_data.m_string ) string_type( val );
	}
		
	inline any( blob_type&& val )
	:
		m_type( type_t::blob )
	{
		new ( &m_data.m_blob ) blob_type( std::move( val ) );
	}
	
	inline any( const blob_type& val )
	:
		m_type( type_t::blob )
	{
		new ( &m_data.m_blob ) blob_type( val.data(), val.size() );
	}
	
	inline any( const array_type &val )
	:
		m_type( type_t::array )
	{
		new ( &m_data.m_array ) array_type( val );
	}
	
	template< typename T >
	inline any( const std::vector< T > &val )
	:
		m_type( type_t::array )
	{
		new ( &m_data.m_array ) array_type();
		
		for ( auto &item : val )
		{
			emplace_back( item );
		}
	}

	inline any( const object_type &val )
	:
		m_type( type_t::object )
	{
		new ( &m_data.m_object ) object_type( val );
	}
	
	any( type_t val )
	:
		m_type( type_t::null )
	{
		set_type( val );
	}

	inline
	any( const any &rhs )
	:
		m_type( type_t::null )
	{
		copy( rhs );
	}
	
	inline
	any( any &&rhs )
	:
		m_type( type_t::null )
	{
		move( rhs );
	}

	any( std::initializer_list< any > list, bool type_deduction = true, type_t manual_type = type_t::array );
	
	any(bstream::ibstream& is);

	inline bstream::obstream& 
	serialize(bstream::obstream& os) const
	{
		return put(os);
	}

	inline
	~any()
	{
		clear();
	}
	
	bool
	equals( const any &rhs ) const;
	
	inline any&
	operator=( bool rhs )
	{
		if ( !is_bool() )
		{
			clear();
			m_type = type_t::boolean;
		}
		
		m_data.m_bool = rhs;
		return *this;
	}

	template< typename T >
	inline typename std::enable_if< std::is_enum< T >::value, any& >::type
	operator=( T val )
	{
		if ( !is_integer() )
		{
			clear();
			m_type = type_t::integer;
		}

		m_data.m_integer = static_cast<std::int64_t>( val );
		return *this;
	}
	
	template< typename T >
	inline typename std::enable_if< std::is_integral< T >::value, any& >::type
	operator=( T val )
	{
		if ( !is_integer() )
		{
			clear();
			m_type = type_t::integer;
		}

		m_data.m_integer = static_cast<std::int64_t>( val );
		return *this;
	}
	
	inline any&
	operator=( float rhs )
	{
		if ( !is_real() )
		{
			clear();
			m_type = type_t::real;
		}
		
		m_data.m_real = rhs;
		return *this;
	}
	
	inline any&
	operator=( double rhs )
	{
		if ( !is_real() )
		{
			clear();
			m_type = type_t::real;
		}
		
		m_data.m_real = rhs;
		return *this;
	}
	
	inline any&
	operator=( std::chrono::system_clock::time_point rhs )
	{
		if ( !is_integer() )
		{
			clear();
			m_type = type_t::integer;
		}
		
		m_data.m_integer = static_cast< std::int64_t >( std::chrono::system_clock::to_time_t( rhs ) );
		return *this;
	}
	
	inline any&
	operator=( char *rhs )
	{
		clear();
		m_type = type_t::string;
		new ( &m_data.m_string ) string_type( rhs );
		
		return *this;
	}
	
	inline any&
	operator=( const char *rhs )
	{
		clear();
		m_type = type_t::string;
		new ( &m_data.m_string ) string_type( rhs );
		
		return *this;
	}
	
	inline any&
	operator=( const string_type &rhs )
	{
		clear();
		m_type = type_t::string;
		new ( &m_data.m_string ) string_type( rhs );
		
		return *this;
	}
	
	
	inline any&
	operator=( const blob_type& rhs )
	{
		clear();
		m_type = type_t::blob;
		new ( &m_data.m_blob ) blob_type( rhs.data(), rhs.size() );
		
		return *this;
	}
	
	inline any&
	operator=( blob_type&& rhs )
	{
		clear();
		m_type = type_t::blob;
		new ( &m_data.m_blob ) blob_type( std::move( rhs ) );
		
		return *this;
	}
	
	inline any&
	operator=( const array_type &rhs )
	{
		clear();
		m_type = type_t::array;
		new ( &m_data.m_array ) array_type( rhs );
		
		return *this;
	}
	
	inline any&
	operator=( const object_type &rhs )
	{
		clear();
		m_type = type_t::object;
		new ( &m_data.m_object ) object_type( rhs );
		
		return *this;
	}
	
	inline any&
	operator=( const std::vector< std::string > &rhs )
	{
		clear();
		m_type = type_t::array;
		new ( &m_data.m_object ) array_type();
		
		for ( auto &string : rhs )
		{
			emplace_back( string );
		}
		
		return *this;
	}

	inline any&
	operator=( const any &rhs )
	{
		if ( this != &rhs )
		{
			copy( rhs );
		}
	
		return *this;
	}
	
	inline any&
	operator=( any &&rhs )
	{
		if ( this != &rhs )
		{
			move( rhs );
		}
		
		return *this;
	}

	inline bool
	operator==( const any &rhs ) const
	{
		return equals( rhs );
	}
	
	inline bool
	operator!=( const any &rhs ) const
	{
		return !equals( rhs );
	}

	inline any&
	operator<<( const any &rhs )
	{
		switch ( m_type )
		{
			case type_t::object:
			{
				if ( rhs.m_type == type_t::object )
				{
					for ( auto &it : rhs.m_data.m_object )
					{
						m_data.m_object.emplace( std::piecewise_construct, std::forward_as_tuple( it.first ), std::forward_as_tuple( it.second ) );
					}
				}
			}
			break;

			default:
			{
			}
			break;
		}

		return *this;
	}

	inline any&
	at_index( std::size_t index )
	{
		if ( m_type != type_t::array )
		{
			clear();
			m_type = type_t::array;
			new ( &m_data.m_array ) array_type();
		}
		
		auto old_size = m_data.m_array.size();
		
		if ( index >= old_size )
		{
			array_type append( index - old_size + 1 );
			m_data.m_array.insert( m_data.m_array.end(), append.begin(),append.end() );
		}
		
		return m_data.m_array[ index ];
	}

	inline const any&
	at_index( std::size_t index ) const
	{
		if ( m_type == type_t::array )
		{
			auto old_size = m_data.m_array.size();
		
			if ( index < old_size )
			{
				return m_data.m_array[ index ];
			}
		}
		
		return null();
	}
	
	inline any&
	operator[]( std::uint8_t index )
	{
		return at_index( index );
	}
	
	inline const any&
	operator[]( std::uint8_t index ) const
	{
		return at_index( index );
	}
	
	inline any&
	operator[]( std::int8_t index )
	{
		return at_index( index );
	}
	
	inline const any&
	operator[]( std::int8_t index ) const
	{
		return at_index( index );
	}
	
	inline any&
	operator[]( std::uint16_t index )
	{
		return at_index( index );
	}
	
	inline const any&
	operator[]( std::uint16_t index ) const
	{
		return at_index( index );
	}
	
	inline any&
	operator[]( std::int16_t index )
	{
		return at_index( index );
	}
	
	inline const any&
	operator[]( std::int16_t index ) const
	{
		return at_index( index );
	}
	
	inline any&
	operator[]( std::uint32_t index )
	{
		return at_index( index );
	}
	
	inline const any&
	operator[]( std::uint32_t index ) const
	{
		return at_index( index );
	}
	
	inline any&
	operator[]( std::int32_t index )
	{
		return at_index( index );
	}
	
	inline const any&
	operator[]( std::int32_t index ) const
	{
		return at_index( index );
	}
	
	inline any&
	operator[]( std::uint64_t index )
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
	inline const any&
	operator[]( std::uint64_t index ) const
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
	inline any&
	operator[]( std::int64_t index )
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
	inline const any&
	operator[]( std::int64_t index ) const
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
#if defined( NODEOZE_SIZE_T_IS_UNIQUE_TYPE )
	inline any&
	operator[]( std::size_t index )
	{
		return at_index( index );
	}
	
	inline const any&
	operator[]( std::size_t index ) const
	{
		return at_index( index );
	}
#endif
	
	inline any&
	operator[]( const char *key )
	{
		std::string s( key );
		return operator[]( s );
	}

	inline any&
	operator[]( const std::string &key )
	{
		if ( m_type != type_t::object )
		{
			clear();
			m_type = type_t::object;
			
			new ( &m_data.m_object ) object_type();
		}
		
		auto it = m_data.m_object.find( key );
		
		if ( it != m_data.m_object.end() )
		{
			return it->second;
		}
		else
		{
			m_data.m_object[ key ] = any();
			return m_data.m_object[ key ];
		}
	}
	
	inline const any&
	operator[]( const char *key ) const
	{
		return operator[]( std::string( key ) );
	}

	inline const any&
	operator[]( const std::string &key ) const
	{
		if ( m_type == type_t::object )
		{
			auto it = m_data.m_object.find( key );
			
			if ( it != m_data.m_object.end() )
			{
				return it->second;
			}
		}
		
		return null();
	}

	inline type_t
	type() const
	{
		return m_type;
	}
	
	inline void
	set_type( type_t val )
	{
		clear();

		m_type = val;
	
		switch ( m_type )
		{
			case type_t::boolean:
			{
				m_data.m_bool = false;
			}
			break;
			
			case type_t::integer:
			{
				m_data.m_integer = 0;
			}
			break;
			
			case type_t::real:
			{
				m_data.m_real = 0.0f;
			}
			break;
			
			case type_t::string:
			{
				new ( &m_data.m_string ) string_type();
			}
			break;
			
			case type_t::array:
			{
				new ( &m_data.m_array ) array_type();
			}
            break;
			
			case type_t::object:
			{
				new ( &m_data.m_object ) object_type();
			}
            break;
			
			default:
			{
			}
            break;
		}
	}

	inline bool
	is_null() const
	{
		return ( m_type == type_t::null );
	}

	inline bool
	is_bool() const
	{
		return ( m_type == type_t::boolean );
	}

	inline bool
	is_integer() const
	{
		return ( m_type == type_t::integer );
	}
	
	inline bool
	is_real() const
	{
		return ( m_type == type_t::real );
	}

	inline bool
	is_string() const
	{
		return ( m_type == type_t::string );
	}
	
	inline bool
	is_blob() const
	{
		return ( m_type == type_t::blob );
	}
		
	inline bool
	is_array() const
	{
		return ( m_type == type_t::array );
	}

	inline bool
	is_object() const
	{
		return ( m_type == type_t::object );
	}

	inline bool
	to_bool() const
	{
		return is_bool() ? m_data.m_bool : false;
	}
	
	inline std::uint8_t
	to_uint8() const
	{
		return is_integer() ? static_cast< std::uint8_t >( m_data.m_integer ) : 0;
	}
	
	inline std::int8_t
	to_int8() const
	{
		return is_integer() ? static_cast< std::int8_t >( m_data.m_integer ) : 0;
	}
	
	inline std::uint16_t
	to_uint16() const
	{
		return is_integer() ? static_cast< std::uint16_t >( m_data.m_integer ) : 0;
	}
	
	inline std::int16_t
	to_int16() const
	{
		return is_integer() ? static_cast< std::int16_t >( m_data.m_integer ) : 0;
	}
	
	inline std::uint32_t
	to_uint32() const
	{
		return is_integer() ? static_cast< std::uint32_t >( m_data.m_integer ) : 0;
	}
	
	inline std::int32_t
	to_int32() const
	{
		return is_integer() ? static_cast< std::int32_t >( m_data.m_integer ) : 0;
	}
	
	inline std::int64_t
	to_int64() const
	{
		if ( is_string() )
		{
			try
			{
				auto mutable_this = const_cast< any* >( this );
				std::int64_t bignum = std::stoll( m_data.m_string );
				mutable_this->clear();
				mutable_this->m_type = type_t::integer;
				mutable_this->m_data.m_integer = static_cast< std::int64_t >( bignum );
				return bignum;
			}
			catch( const std::exception & /*unused*/ )
			{
				return 0LL;
			}
		}
		else
		{
			return is_integer() ? static_cast< std::int64_t >( m_data.m_integer ) : 0;
		}
	}
	
	inline std::uint64_t
	to_uint64() const
	{
		if ( is_string() )
		{
			try
			{
				auto mutable_this = const_cast< any* >( this );
				std::uint64_t bignum = std::stoull( m_data.m_string );
				mutable_this->clear();
				mutable_this->m_type = type_t::integer;
				mutable_this->m_data.m_integer = bignum;
				return bignum;
			}
			catch( const std::exception & /*unused*/ )
			{
				return 0ULL;
			}
		}
		else
		{
			return is_integer() ? static_cast< std::uint64_t >( m_data.m_integer ) : 0;
		}
	}

	inline auto
	to_oid() const
	{
		return to_int64();
	}
	
	std::chrono::system_clock::time_point
	to_time() const
	{
		return std::chrono::system_clock::from_time_t( static_cast< std::time_t >( is_integer() ? m_data.m_integer : 0 ) );
	}
	
	log::level_t
	to_log_level() const
	{
		return is_integer() ? static_cast< log::level_t >( m_data.m_integer ) : log::level_t::info;
	}

	double
	to_real() const
	{
		return is_real() ? m_data.m_real : 0;
	}
	
	string_type
	to_string() const
	{
		std::string ret;
		
		if ( is_string() )
		{
			ret = m_data.m_string;
		}
		else if ( is_bool() )
		{
			ret = ( m_data.m_bool ) ? "true" : "false";
		}
		else if ( is_integer() )
		{
			ret = std::to_string( m_data.m_integer );
		}
		else if ( is_real() )
		{
			ret = std::to_string( m_data.m_real );
		}
		else if ( is_blob() )
		{
			ret = codec::base64::encode( m_data.m_blob );
		}
		
		return ret;
	}
	
	std::vector< std::string >
	to_strings() const
	{
		std::vector< std::string > ret;
		
		if ( is_array() )
		{
			for ( auto &child : *this )
			{
				ret.emplace_back( child.to_string() );
			}
		}
		
		return ret;
	}
	
	blob_type
	to_blob() const
	{
		if ( is_blob() )
		{
			return blob_type( ( void* ) m_data.m_blob.data(), m_data.m_blob.size(), m_data.m_blob.size(), blob_type::do_not_delete_data );
		}
		else if ( is_string() )
		{
			any *self = const_cast< any* >( this );
			*self = codec::base64::decode( m_data.m_string );
			return blob_type( ( void* ) m_data.m_blob.data(), m_data.m_blob.size(), m_data.m_blob.size(), blob_type::do_not_delete_data );
		}
		else
		{
			return blob_type();
		}
	}
	
	array_type&
	to_array()
	{
		return is_array() ? m_data.m_array : m_empty_array;
	}
	
	const array_type&
	to_array() const
	{
		return is_array() ? m_data.m_array : m_empty_array;
	}
	
	object_type&
	to_object()
	{
		return is_object() ? m_data.m_object : m_empty_object;
	}
	
	const object_type&
	to_object() const
	{
		return is_object() ? m_data.m_object : m_empty_object;
	}
	
	inline object_type::iterator
	object_begin()
	{
		return is_object() ? m_data.m_object.begin() : m_empty_object.begin();
	}
	
	inline object_type::const_iterator
	object_begin() const
	{
		return is_object() ? m_data.m_object.begin() : m_empty_object.begin();
	}
	
	inline object_type::const_iterator
	object_cbegin() const
	{
		return is_object() ? m_data.m_object.cbegin() : m_empty_object.cbegin();
	}
	
	inline object_type::iterator
	object_end()
	{
		return is_object() ? m_data.m_object.end() : m_empty_object.end();
	}
	
	inline object_type::const_iterator
	object_end() const
	{
		return is_object() ? m_data.m_object.end() : m_empty_object.end();
	}
	
	inline object_type::const_iterator
	object_cend() const
	{
		return is_object() ? m_data.m_object.cend() : m_empty_object.cend();
	}
	
	inline array_type::iterator
	begin()
	{
		return is_array() ? m_data.m_array.begin() : m_empty_array.begin();
	}
	
	inline array_type::const_iterator
	begin() const
	{
		return is_array() ? m_data.m_array.begin() : m_empty_array.begin();
	}
	
	inline array_type::const_iterator
	cbegin() const
	{
		return is_array() ? m_data.m_array.cbegin() : m_empty_array.cbegin();
	}
	
	inline array_type::iterator
	end()
	{
		return is_array() ? m_data.m_array.end() : m_empty_array.end();
	}
	
	inline array_type::const_iterator
	end() const
	{
		return is_array() ? m_data.m_array.end() : m_empty_array.end();
	}
	
	inline array_type::const_iterator
	cend() const
	{
		return is_array() ? m_data.m_array.cend() : m_empty_array.cend();
	}
		
	template < class Visitor >
	inline void
	visit( Visitor visitor ) const
	{
		if ( is_array() )
		{
			for ( auto &it : m_data.m_array )
			{
				visitor( std::string(), it );
			}
		}
		else if ( is_object() )
		{
			for ( auto &it : m_data.m_object )
			{
				visitor( it.first, it.second );
			}
		}
	}
	
	inline bool
	is_member( const std::string &key ) const
	{
		bool ok = false;
	
		if ( m_type == type_t::object )
		{
			auto it = m_data.m_object.find( key );
			ok = ( it != m_data.m_object.end() );
		}
		
		return ok;
	}
	
	keys
	all_keys() const
	{
		any::keys ret;
	
		if ( is_object() )
		{
			for ( auto &it : m_data.m_object )
			{
				ret.push_back( it.first );
			}
		}
		
		return ret;
	}

	template < class T >
	inline void
	push_back( T val )
	{
		if ( m_type != type_t::array )
		{
			clear();
			m_type = type_t::array;
			new ( &m_data.m_array ) array_type();
		}
	
		m_data.m_array.push_back( val );
	}

	inline void
	push_back( const any &val )
	{
		if ( m_type != type_t::array )
		{
			clear();
			m_type = type_t::array;
			new ( &m_data.m_array ) array_type();
		}
	
		m_data.m_array.push_back( val );
	}

	inline void
	emplace_back( any &&val )
	{
		if ( m_type != type_t::array )
		{
			clear();
			m_type = type_t::array;
			new ( &m_data.m_array ) array_type();
		}
	
		m_data.m_array.emplace_back( std::move( val ) );
	}
	
	std::pair< object_type::iterator, bool >
	emplace( const std::string &key, any &&val )
	{
		if ( m_type != type_t::object )
		{
			clear();
			m_type = type_t::object;
			new ( &m_data.m_object ) object_type();
		}
		
		return m_data.m_object.emplace( key, std::move( val ) );
	}

	std::error_code
	erase( std::size_t index )
	{
		auto ret = std::error_code();

		ncheck_error_action_quiet( m_type == type_t::array, ret = make_error_code( std::errc::invalid_argument ), exit );
		ncheck_error_action_quiet( index < size(), ret = make_error_code( std::errc::invalid_argument ), exit );
		m_data.m_array.erase( m_data.m_array.begin() + index );

	exit:

		return ret;
	}

	void
	erase( const std::string &key )
	{
		object_type::iterator it;

		ncheck_error( m_type == type_t::object, exit, "any (%) is not an object", m_type );
		it = m_data.m_object.find( key );
		ncheck_error( it != m_data.m_object.end(), exit, "key % does not exist in object", key );
		m_data.m_object.erase( it );

	exit:

		return;
	}
	
	inline bool
	empty() const
	{
		auto ret = false;
		
		switch ( m_type )
		{
			case type_t::string:
			{
				ret = m_data.m_string.empty();
			}
			break;
			
			case type_t::blob:
			{
				ret = m_data.m_blob.empty();
			}
			break;
			
			case type_t::array:
			{
				ret = m_data.m_array.empty();
			}
			break;
			
			case type_t::object:
			{
				ret = m_data.m_object.empty();
			}
			break;
			
			default:
			{
			}
			break;
		}
		
		return ret;
	}

	inline std::size_t
	size() const
	{
		auto ret = std::size_t( 0 );
		
		switch ( m_type )
		{
			case type_t::string:
			{
				ret = m_data.m_string.size();
			}
			break;
			
			case type_t::blob:
			{
				ret = m_data.m_blob.size();
			}
			break;
			
			case type_t::array:
			{
				ret = m_data.m_array.size();
			}
			break;
			
			case type_t::object:
			{
				ret = m_data.m_object.size();
			}
			break;
			
			default:
			{
			}
			break;
		}
		
		return ret;
	}
	
	find_t
	find( const std::string &pointer ) const;
	
	inline bool
	matches( const any& pattern ) const
	{
		bool match = true;
		
		if ( pattern.is_object() && is_object() )
		{
			for ( auto it = pattern.object_begin(); it != pattern.object_end(); ++it )
			{
				if ( ! ( is_member( it->first ) && it->second == operator[]( it->first ) ) )
				{
					match = false;
					break;
				}
			}
		}
		else if ( ! equals( pattern ) )
		{
			match = false;
		}
	
		return match;
	}
	
	bool
	equals_ignoring( const any& value, const std::set< std::string >& exclusions ) const
	{
		bool match = true;
		
		if ( value.is_object() && is_object() )
		{
			std::size_t my_key_count = 0;
			for ( auto it = object_begin(); it != object_end(); ++it )
			{
				if ( exclusions.count( it->first ) == 0 )
				{
					my_key_count++;
				}
			}
			std::size_t value_key_count = 0;
			for ( auto it = value.object_begin(); it != value.object_end(); ++it )
			{
				if ( exclusions.count( it->first ) == 0 )
				{
					value_key_count++;
				}
			}
			if ( my_key_count != value_key_count )
			{
				match = false;
			}
			else
			{
				for ( auto it = object_begin(); it != object_end(); ++it )
				{
					if ( exclusions.count( it->first ) == 0 )
					{
						if ( ! ( value.is_member( it->first ) && value[ it->first ] == it->second ) )
						{
							match = false;
							break;
						}
					}
				}
			}
		}
		else if ( ! equals( value ) )
		{
			match = false;
		}
	
		return match;
	}

	void
	sanitize();
	
	std::error_code
	patch( const any& patches );
	
private:


	bstream::obstream& put(bstream::obstream& os) const;

	typedef std::vector< std::string >::iterator token_iterator_t;

	union data
	{
		data()
		{
		}
		
		~data()
		{
		}
		
		data( bool val )
		:
			m_bool( val )
		{
		}

		data( int val )
		:
			m_integer(val)
		{
		}
		
		data( double val )
		:
			m_real( val )
		{
		}
		
		bool			m_bool;
		std::int64_t	m_integer;
		double			m_real;
		string_type		m_string;
		blob_type		m_blob;
		array_type		m_array;
		object_type		m_object;
	};
	
	enum class patch_opcode_t
	{
		add,
		remove,
		replace,
		copy,
		move,
		test,
		invalid
	};

	static bool
	is_hex_digit(char digit);

	static bool
	is_white_space(char whiteSpace);

	static void
	read_string(std::istream &input, std::string &result);

	static void
	read_object(std::istream &input, object_type &result);

	static void
	read_array(std::istream &input, array_type &result);

	static void
	read_number(std::istream &input, any &result);

	static void
	read_to_non_white_space(std::istream &input, char &currentCharacter);

	static void
	create_patches( any &patches, const std::string &path, const any &lhs, const any &rhs, const std::vector< std::string > &exclusions, bool case_sensitive );

	static void
	add_patch( any &patches, const std::string &op, const std::string &path, const std::string &suffix, const any &obj, const std::vector< std::string > &exclusions );
	
	std::error_code
	apply( const any &patch );
	
	inline patch_opcode_t
	decode_patch_operation( const any &obj )
	{
		patch_opcode_t op( patch_opcode_t::invalid );
		
		if ( obj.is_string() )
		{
			std::string lower = obj.to_string();
			std::transform( lower.begin(), lower.end(), lower.begin(), []( char c ) { return std::tolower( c ); } );
			
			if ( lower == "add" )
			{
				op = patch_opcode_t::add;
			}
			else if ( lower == "remove" )
			{
				op = patch_opcode_t::remove;
			}
			else if ( lower == "replace" )
			{
				op = patch_opcode_t::replace;
			}
			else if ( lower == "copy" )
			{
				op = patch_opcode_t::copy;
			}
			else if ( lower == "move" )
			{
				op = patch_opcode_t::move;
			}
			else if ( lower == "test" )
			{
				op = patch_opcode_t::test;
			}
		}
		
		return op;
	}
	
	inline void
	copy( const any &rhs )
	{
#if defined( NODEOZE_PROFILE_ANY )
		nlog( log::level_t::warning, "any copy" );
#endif

		clear();
	
		m_type = rhs.m_type;
		
		switch ( m_type )
		{
			case type_t::boolean:
			{
				m_data.m_bool = rhs.m_data.m_bool;
			}
			break;
			
			case type_t::integer:
			{
				m_data.m_integer = rhs.m_data.m_integer;
			}
			break;
			
			case type_t::real:
			{
				m_data.m_real = rhs.m_data.m_real;
			}
			break;
			
			case type_t::string:
			{
				new ( &m_data.m_string ) string_type( rhs.m_data.m_string );
			}
			break;
			
			case type_t::blob:
			{
				new ( &m_data.m_blob ) blob_type( rhs.m_data.m_blob.data(), rhs.m_data.m_blob.size() );
			}
			break;
			
			case type_t::array:
			{
				new ( &m_data.m_array ) array_type( rhs.m_data.m_array );
			}
            break;
			
			case type_t::object:
			{
				new ( &m_data.m_object ) object_type( rhs.m_data.m_object );
			}
            break;
			
			default:
			{
			}
            break;
		}
	}
	
	inline void
	move( any &rhs )
	{
#if defined( NODEOZE_PROFILE_ANY )
		nlog( log::level_t::warning, "any move" );
#endif
		clear();

		m_type = rhs.m_type;
		
		switch ( m_type )
		{
			case type_t::boolean:
			{
				m_data.m_bool = rhs.m_data.m_bool;
			}
			break;
			
			case type_t::integer:
			{
				m_data.m_integer = rhs.m_data.m_integer;
			}
			break;
			
			case type_t::real:
			{
				m_data.m_real = rhs.m_data.m_real;
			}
			break;
			
			case type_t::string:
			{
				new ( &m_data.m_string ) string_type( std::move( rhs.m_data.m_string ) );
			}
			break;
			
			case type_t::blob:
			{
				new ( &m_data.m_blob ) blob_type( std::move( rhs.m_data.m_blob ) );
			}
			break;
			
			case type_t::array:
			{
				new ( &m_data.m_array ) array_type( std::move( rhs.m_data.m_array ) );
			}
            break;
			
			case type_t::object:
			{
				new ( &m_data.m_object ) object_type( std::move( rhs.m_data.m_object ) );
			}
            break;
			
			default:
			{
			}
            break;
		}
	}
	
	inline void
	clear()
	{
		switch ( m_type )
		{
			case type_t::string:
			{
				m_data.m_string.~string_type();
			}
			break;
			
			case type_t::blob:
			{
				m_data.m_blob.~blob_type();
			}
			break;
			
			case type_t::array:
			{
				m_data.m_array.~array_type();
			}
            break;
	
			case type_t::object:
			{
				m_data.m_object.~object_type();
			}
            break;
			
			default:
			{
			}
            break;
		}
		
		m_type = type_t::null;
	}

	static array_type	m_empty_array;
	static object_type	m_empty_object;
	type_t				m_type;
	data				m_data;
};

struct any::find_t
{
	find_t()
	:
		parent( nullptr ),
		valid( true )
	{
	}
		
	std::vector< std::string >	tokens;
	std::size_t					index;
	any							*parent;
	bool						valid;
		
	/*
	 * for arrays
	 */
		
	any::array_type::iterator	a_child;
		
	/*
	 * for objects
	 */
		
	any::object_type::iterator	o_child;
		
	inline std::size_t
	token_mismatch_distance() const
	{
		return tokens.size() - index;
	}
		
	explicit operator bool() const
	{
		return valid;
	}
};

inline std::ostream&
operator<<(std::ostream &os, const any::type_t type )
{

	switch ( type )
	{
		case any::type_t::null:
		{
			os << "null";
		}
		break;
	
		case any::type_t::boolean:
		{
			os << "boolean";
		}
		break;
	
		case any::type_t::integer:
		{
			os << "integer";
		}
		break;
	
		case any::type_t::real:
		{
			os << "real";
		}
		break;
	
		case any::type_t::string:
		{
			os << "string";
		}
		break;
	
		case any::type_t::blob:
		{
			os << "blob";
		}
		break;
	
		case any::type_t::array:
		{
			os << "array";
		}
		break;
	
		case any::type_t::object:
		{
			os << "object";
		}
		break;
	
		default:
		{
			os << "unknown";
		}
		break;
	}
	
	return os;
}

nodeoze::ostream&
operator<<( nodeoze::ostream &os, const any &root );

std::ostream&
operator<<( std::ostream &os, const any &root );

}

#endif
