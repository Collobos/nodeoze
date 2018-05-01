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

/*!
 *	\file nany.h
 *	\author Scott Herscher (with contributions by David Curtis)
 */

namespace nodeoze {

/*! \class any
 *
 *	\brief A variant, discriminated value type.
 *
 *	The any class corresponds closely to the abstract model underlying JSON data types. 
 *	An any value contains a discriminant value of type \ref any::type_t, and a value
 *	of the type indicated by the discriminant.
 *	An instance of type any can hold a value of one of the following types:
 *		- integer
 *		- floating
 *		- string
 *		- boolean
 *		- blob (an uninterpreted sequence of bytes)
 *		- array (a vector of values of type any)
 *		- object (an unordered set of string/value pairs, where the value is type any)
 *		- null (no assigned value)
 * 
 *	The type of an any value can be accessed through the member function type(), 
 *	or determined by invoking predicate member functions such as is_string(), or
 *	is_int(). The variant value can be accessed through typed accessors such as
 *	to_string(), or to_array(). Invoking an accessor for a type that does not
 * 	match the current value will yield an empty (e.g., empty string, zero-length array)
 *	or zero (in the case of numeric types) value. In general, the type of an any should
 *	always be checked before invoking an accessor.
 */
class any
{
public:

	/*!	\brief The type used internally to represent integer values.
	 */
	using integer_type = 						std::int64_t;

	/*!	\brief The type used internally to represent floating values.
	 */
	using floating_type = 						double;

	/*!	\brief The type used internally to represent boolean values.
	 */
	using boolean_type = 						bool;

	/*!	\brief The type used internally to represent string values.
	 */
	using string_type = 						std::string;

	/*!	\brief The type used internally to represent blob values.
	 */
	using blob_type = 							nodeoze::buffer;

	/*!	\brief The type used internally to represent array values.
	 */
	using array_type = 							deque< any >;

	/*!	\brief The type used internally to represent object values.
	 */
	using object_type = 						unordered_map< std::string, any >;

	using keys = 								std::vector<std::string>;

	struct find_t;

	/*! \enum type_t
	 *
	 *	\brief Discriminant type for any values
	 */
	enum class type_t
	{
		null,
		boolean,
		integer,
		floating,
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

	/*!	\brief Returns a reference to a const null-valued any.
	 *
	 *	\return reference to a const null-valued any.
	 *  
	 */
	static const any&
	null();
	

	/*!	\brief Returns an any containing an empty array.
	 *
	 *	\return an any containing an empty array.
	 */
	static any
	array();
	
	/*!	\brief Returns an any containing an empty object.
	 *
	 *	\return an any containing an empty object.
	 */
	static any
	object();
	
	/*!	\brief
	 *
	 *
	 */
	inline static any
	build( std::initializer_list< any > list, bool type_deduction = true, type_t manual_type = type_t::array )
	{
		return any( list, type_deduction, manual_type );
	}
	
	/*!	\brief Constructs an instance whose type is null.
	 *
	 *
	 */
	inline any()
	:
		m_type( type_t::null )
	{
	}
	
	/*!	\brief Constructs an instance containing a boolean value.
	 *	\param val boolean value assigned to constructed instance.
	 *
	 */
	inline any( bool val )
	:
		m_type( type_t::boolean )
	{
		m_data.m_bool = val;
	}

	/*!	\brief Constructs an instance containing an integer value from the specified enum value.
	 *	\param val value of an enum type
	 *
	 *	Converts the enum value of parameter \c val to an integer of the enum's underlying type,
	 *	and assigns it to the constructed instance.
	 */
	template< typename T >
	inline any( T val, typename std::enable_if< std::is_enum< T >::value >::type* = 0 )
	:
		m_type( type_t::integer )
	{
		m_data.m_integer = static_cast< std::int64_t >( val );
	}

	/*!	\brief Constructs an instance containing an integer value.
	 *	\param val integer value assigned to the constructed instance.
	 *
	 */
	template< typename T >
	inline any( T val, typename std::enable_if< std::is_integral< T >::value >::type* = 0 )
	:
		m_type( type_t::integer )
	{
		m_data.m_integer = static_cast< std::int64_t >( val );
	}

	/*!	\brief Constructs an instance containing an integer value from the specifed time_point.
	 *	\param val a time_point value
	 *
	 *	Converts the time_point value of parameter \c val to a \c time_t value, and assigns it to the constructed instance
	 * 	as an integer.
	 */
	inline any( std::chrono::system_clock::time_point val )
	:
		m_type( type_t::integer )
	{
		m_data.m_integer = static_cast< std::int64_t >( std::chrono::system_clock::to_time_t( val ) );
	}
	
	/*!	\brief Constructs an instance containing a floating value.
	 *	\param val a float value assigned to the constructed instance.
	 *
	 */
	inline any( float val )
	:
		m_type( type_t::floating )
	{
		m_data.m_floating = val;
	}
	
	/*!	\brief Constructs an instance containing a floating value.
	 *	\param val a double value assigned to the constructed instance.
	 *
	 */
	inline any( double val )
	:
		m_type( type_t::floating )
	{
		m_data.m_floating = val;
	}
	
	/*!	\brief Constructs an instance containing a string value.
	 *	\param val A C-style, null-terminated character string.
	 *
	 */
	inline any( char *val )
	:
		m_type( type_t::string )
	{
		new ( &m_data.m_string ) string_rep( std::string(val) );
	}
	
	/*!	\brief Constructs an instance containing a string value.
	 *	\param val A C-style, null-terminated character string.
	 *
	 */
	inline any( const char *val )
	:
		m_type( type_t::string )
	{
		new ( &m_data.m_string ) string_rep( std::string(val) );
	}
	
	/*!	\brief Constructs an instance containing a string value.
	 *	\param val a value of type std::string, whose contents are copied to the constructed instance.
	 *
	 */
	inline any( const string_type &val )
	:
		m_type( type_t::string )
	{
		new ( &m_data.m_string ) string_rep( val );
	}
	
	/*!	\brief Constructs an instance containing a string value.
	 *	\param val a value of type std::string whose contents are moved to the constructed instance.
	 *
	 */
	inline any( string_type&& val )
	:
		m_type( type_t::string )
	{
		new ( &m_data.m_string ) string_rep( std::move( val ) );
	}
		
	/*!	\brief Constructs an instance containing a blob value.
	 *	\param val a value of type nodeoze::buffer, whose contents are moved to the constructed instance.
	 *
	 */
	inline any( blob_type&& val )
	:
		m_type( type_t::blob )
	{
		new ( &m_data.m_blob ) blob_type( std::move( val ) );
	}
	
	/*!	\brief Constructs an instance containing a blob value.
	 *	\param val a value of type nodeoze::buffer, whose contents are copied to the constructed instance.
	 *
	 */
	inline any( const blob_type& val )
	:
		m_type( type_t::blob )
	{
		new ( &m_data.m_blob )  blob_type( val.const_data(), val.size() );
	}
	
	/*!	\brief Constucts an instance containing an array value.
	 *	\param val a value of type std::deque<any>, whose contents are copied to the constructed instance.
	 *
	 */
	inline any( const array_type &val )
	:
		m_type( type_t::array )
	{
		new ( &m_data.m_array ) array_type( val );
	}
	
	/*!	\brief Constructs an instance containing an array value.
	 *	\param val a value of type std::vector<any>, whose contents are copied to the constructed instance.
	 *
	 */
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

	/*!	\brief Constructs an instance containing an object value.
	 *	\param val a value of type std::unordered_map<std::string, any>, whose contents are copied to the constructed instance.
	 */
	inline any( const object_type &val )
	:
		m_type( type_t::object )
	{
		new ( &m_data.m_object ) object_type( val );
	}
	
	/*!	\brief Constructs an instance containing the default value for the specified type.
	 *	\param val the discriminant value specifying the type contained by the constructed instance.
	 *
	 *	Default values for numeric types are zero; for boolean, false; for string, blob, array and object,
	 *	empty instances of the corresponding type internal representation types.
	 */ 
	any( type_t val )
	:
		m_type( type_t::null )
	{
		set_type( val );
	}

	/*!	\brief Copy constructor.
	 *	\param rhs an any whose contents are copied to the constucted instance.
	 *
	 */
	inline
	any( const any &rhs )
	:
		m_type( type_t::null )
	{
		copy( rhs );
	}
	
	/*!	\brief Move constructor.
	 *	\param rhs value whose contents are moved to the constructed instance.
	 *
	 */
	inline
	any( any &&rhs )
	:
		m_type( type_t::null )
	{
		move( rhs );
	}

	// TODO: this needs some thought
	/*!	\brief
	 *
	 *
	 */
	any( std::initializer_list< any > list, bool type_deduction = true, type_t manual_type = type_t::array );
	
	/*!	\brief Constructs an instance by deserializing the value from specified binary stream.
	 *	\param is a binary input stream containing a serialized value.
	 *
	 */
	any(bstream::ibstream& is);

	/*!	\brief Serializes this instance to the specified binary stream.
	 *	\param os a binary out stream to which this instance's contents will be serialized.
	 *	\return a reference to the stream passed as os parameter.
	 */
	inline bstream::obstream& 
	serialize(bstream::obstream& os) const
	{
		return put(os);
	}

	/*!	\brief Destructor.
	 *
	 *
	 */
	inline
	~any()
	{
		clear();
	}
	
	/*!	\brief Compares this instance with the specified value.
	 *	\param rhs an value to be compared with this instance.
	 *	\return true if the specified value is equal to this instance, false otherwise.
	 */
	bool
	equals( const any &rhs ) const;
	
	/*!	\brief Assigns a boolean value to this instance.
	 *	\param rhs a boolean value.
	 *	\return a reference to this instance
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
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

	/*!	\brief Assigns an enum value to this instance, as an integer value.
	 *	\param val a value of an enum type.
	 *	\return a reference to this instance.
	 *
	 *	Converts the specified enum value to the underlying integer type, 
	 * 	and assigns that value to this instance. Any previous value contained 
	 *	by the target instance is destroyed.
	 */
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
	
	/*!	\brief Assigns an integer value to this instance.
	 *	\param rhs an integer value.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	template< typename T >
	inline typename std::enable_if< std::is_integral< T >::value, any& >::type
	operator=( T rhs )
	{
		if ( !is_integer() )
		{
			clear();
			m_type = type_t::integer;
		}

		m_data.m_integer = static_cast<std::int64_t>( rhs );
		return *this;
	}
	
	/*!	\brief Assings a floating value to this instance.
	 *	\param rhs a float value.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( float rhs )
	{
		if ( !is_floating() )
		{
			clear();
			m_type = type_t::floating;
		}
		
		m_data.m_floating = rhs;
		return *this;
	}
	
	/*!	\brief Assigns a floating value to this instance.
	 *	\param val a double value.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( double rhs )
	{
		if ( !is_floating() )
		{
			clear();
			m_type = type_t::floating;
		}
		
		m_data.m_floating = rhs;
		return *this;
	}
	
	/*!	\brief Assigns a time_point value to this instance as an integer value.
	 *	\param rhs a time_point value.
	 *	\return a reference to this instance.
	 *
	 *	The time_point specified by the rhs parameter is converted to type time_t, and
	 * 	assigned to this instance as an integer value.
	 *	Any previous value contained by the target instance is destroyed.
	 *
	 */
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
	
	/*!	\brief Assigns a string value to this instance.
	 *	\param rhs a C-style, null-terminated character string.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( char *rhs )
	{
		clear();
		m_type = type_t::string;
		new ( &m_data.m_string ) string_rep( std::string( rhs ) );
		
		return *this;
	}
	
	/*!	\brief Assigns a string value to this instance.
	 *	\param rhs a C-style, null-terminated character string.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( const char *rhs )
	{
		clear();
		m_type = type_t::string;
		new ( &m_data.m_string ) string_rep( std::string( rhs ) );
		
		return *this;
	}
	
	/*!	\brief Assigns a string value to this instance.
	 *	\param rhs a value of type std::string, whose contents are copied to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( const string_type &rhs )
	{
		clear();
		m_type = type_t::string;
		new ( &m_data.m_string ) string_rep ( rhs );
		
		return *this;
	}

	/*!	\brief Assigns a string value to this instance.
	 *	\param rhs a value of type std::string, whose contents are moved to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( string_type&& rhs )
	{
		clear();
		m_type = type_t::string;
		new ( &m_data.m_string ) string_rep ( std::move( rhs ) );
	
		return *this;
	}
	
	/*!	\brief
	 *
	 *
	 */
	// TODO: should this exist?
	inline any&
	operator=( const blob_type& rhs )
	{
		clear();
		m_type = type_t::blob;
		new ( &m_data.m_blob ) blob_type( rhs.const_data(), rhs.size() );
		
		return *this;
	}
	
	/*!	\brief Assigns a blob value to this instance.
	 *	\param rhs a value of type nodeoze::buffer, whose contents are moved to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( blob_type&& rhs )
	{
		clear();
		m_type = type_t::blob;
		new ( &m_data.m_blob ) blob_type( std::move( rhs ) );
		
		return *this;
	}
	
	/*!	\brief Assigns a blob value to this instance.
	 *	\param rhs a value of type nodeoze::buffer, whose contents are copied to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( const array_type &rhs )
	{
		clear();
		m_type = type_t::array;
		new ( &m_data.m_array ) array_type( rhs );
		
		return *this;
	}
	
	/*!	\brief Assigns an object value to this instance.
	 *	\param rhs a value of type std::unordered_map<std::string, any>, whose contents are copied to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( const object_type &rhs )
	{
		clear();
		m_type = type_t::object;
		new ( &m_data.m_object ) object_type( rhs );
		
		return *this;
	}
	
	/*!	\brief Assigns an array of strings to this instance.
	 *	\param rhs a value of type std::vector<std::string>, whose contents are copied to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
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

	/*!	\brief Copy assignment operator.
	 *	\param rhs an any value copied to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( const any &rhs )
	{
		if ( this != &rhs )
		{
			copy( rhs );
		}
	
		return *this;
	}
	
	/*!	\brief Move assignment operator.
	 *	\param rhs an any value moved to this instance.
	 *	\return a reference to this instance.
	 *
	 *	Any previous value contained by the target instance is destroyed.
	 */
	inline any&
	operator=( any &&rhs )
	{
		if ( this != &rhs )
		{
			move( rhs );
		}
		
		return *this;
	}

	/*!	\brief Equality comparison operator.
	 *	\param rhs an any value compared with this instance.
	 *	\return true if the \c rhs param contains the same type and value as this instance, false otherwise.
	 *
	 *	Equality is determined by invoking the equality operator of the underlying internal representation type.
	 */
	inline bool
	operator==( const any &rhs ) const
	{
		return equals( rhs );
	}
	
	/*!	\brief Inequality comparison operator.
	 *	\param rhs an any value compared with this instance.
	 *	\return false if the \c rhs param contains the same type and value as this instance, true otherwise.
	 *
	 *	Equality is determined by invoking the equality operator of the underlying internal representation type.
	 */
	inline bool
	operator!=( const any &rhs ) const
	{
		return !equals( rhs );
	}

	// TODO: WTF?

	/*!	\brief
	 *
	 *
	 */
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

	/*!	\brief Returns a reference to the specified element in an array contained by this instance.
	 *	\param index the index of the array element.
	 *	\return a reference to the specified array element.
	 *
	 *	If this instance does not contain an array, the current contents are destroyed
	 *	and replaced with an array of size (index + 1) containing null values.
	 *
	 *	If this instance contains an array, and the size is less than (index + 1), null
	 *	values are appended to the contained array until it's size is (index + 1).
	 *
	 */
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

	/*!	\brief Returns a const reference to the specified element in an array contained by this instance.
	 *	\param index the index of the array element.
	 *	\return a reference to the specified array element.
	 *
	 *	If this instance does not contain an array, or it contains an array of size
	 *	less than (index + 1), a null value is returned.
	 */
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
	
	/*!	\brief Returns a reference to the specified element in an array contained by this instance.
	 *	\param index the index of the array element.
	 *	\return a reference to the specified array element.
	 *
	 *	If this instance does not contain an array, the current contents are destroyed
	 *	and replaced with an array of size (index + 1) containing null values.
	 *
	 *	If this instance contains an array, and the size is less than (index + 1), null
	 *	values are appended to the contained array until it's size is (index + 1).
	 *
	 */
	template< class T >
	inline typename std::enable_if_t< std::is_integral<T>::value, any& >
	operator[]( T index )
	{
		return at_index( static_cast<std::size_t>( index ) );
	}

	/*!	\brief Returns a const reference to the specified element in an array contained by this instance.
	 *	\param index the index of the array element.
	 *	\return a reference to the specified array element.
	 *
	 *	If this instance does not contain an array, or it contains an array of size
	 *	less than (index + 1), a null value is returned.
	 */
	template< class T>
	inline typename std::enable_if_t< std::is_integral<T>::value, const any& >
	operator[]( T index ) const
	{
		return at_index( static_cast<std::size_t>( index ) );
	}

#if 0

	/*!	\brief Returns a reference to the specified element in an array contained by this instance.
	 *	\param index the index of the array element.
	 *	\return a reference to the specified array element.
	 *
	 *	If this instance does not contain an array, the current contents are destroyed
	 *	and replaced with an array of size (index + 1) containing null values.
	 *
	 *	If this instance contains an array, and the size is less than (index + 1), null
	 *	values are appended to the contained array until it's size is (index + 1).
	 *
	 */
	inline any&
	operator[]( std::uint8_t index )
	{
		return at_index( index );
	}
	
	/*!	\brief Returns a const reference to the specified element in an array contained by this instance.
	 *	\param index the index of the array element.
	 *	\return a reference to the specified array element.
	 *
	 *	If this instance does not contain an array, or it contains an array of size
	 *	less than (index + 1), a null value is returned.
	 */
	inline const any&
	operator[]( std::uint8_t index ) const
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::int8_t index )
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::int8_t index ) const
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::uint16_t index )
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::uint16_t index ) const
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::int16_t index )
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::int16_t index ) const
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::uint32_t index )
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::uint32_t index ) const
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::int32_t index )
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::int32_t index ) const
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::uint64_t index )
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::uint64_t index ) const
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::int64_t index )
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::int64_t index ) const
	{
		return at_index( static_cast< std::size_t >( index ) );
	}
	
#endif

#if defined( NODEOZE_SIZE_T_IS_UNIQUE_TYPE )
	/*!	\brief
	 *
	 *
	 */
	inline any&
	operator[]( std::size_t index )
	{
		return at_index( index );
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline const any&
	operator[]( std::size_t index ) const
	{
		return at_index( index );
	}
#endif
	
	/*!	\brief Returns a reference to the value associated with the specified key.
	 *	\param key a C-style null-terminated string used to locate or create an associated value in an object
	 *	\return The value associated with the key.
	 *	
	 *	If the type of this instance is object, and an entry with the key exists in 
	 *	the object, a reference to the associated value is returned.
	 *
	 *	If the type of this instance is object, and no entry exists with the key,
	 *	an entry is created with the key and a value of type null.
	 *
	 *	If the type of this instance is not object, the current value is destroyed, 
	 *	and the value is set to an object with a single entry comprising the key 
	 *	and a null object associated with the key.
	 */
	inline any&
	operator[]( const char *key )
	{
		std::string s( key );
		return operator[]( s );
	}

	/*!	\brief
	 *
	 *
	 */
	/*!	\brief Returns a reference to the value associated with the specified key.
	 *	\param key a string used to locate or create an entry in an object
	 *	\return The value associated with the key.
	 *	
	 *	If the type of this instance is object, and an entry with the key exists in 
	 *	the object, a reference to the associated value is returned.
	 *
	 *	If the type of this instance is object, and no entry exists with the key,
	 *	an entry is created with the key and a value of type null.
	 *
	 *	If the type of this instance is not object, the current value is destroyed, 
	 *	and the value is set to an object with a single entry comprising the key 
	 *	and a null object associated with the key.
	 */
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
	
	/*!	\brief Returns a reference to a value associated with the specified key.
	 *	\param key a C-style, null-terminated string used to locate or create an object entry.
	 *	\return a const reference to the associated value, if an entry for the key exists.
	 *
	 *	If the type of this instance is object, and an entry for the key is present, 
	 *	a reference to the associated value is returned. Otherwise, A reference to a
	 *	value of type null is returned.
	 */
	inline const any&
	operator[]( const char *key ) const
	{
		return operator[]( std::string( key ) );
	}

	/*!	\brief Returns a reference to a value associated with the specified key.
	 *	\param key a string used to locate or create an entry in an object
	 *	\return a const reference to the associated value, if an entry for the key exists.
	 *
	 *	If the type of this instance is object, and an entry for the key is present, 
	 *	a reference to the associated value is returned. Otherwise, A reference to a
	 *	value of type null is returned.
	 */
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

	/*!	\brief Returns the type contained by this instance.
	 *	\return the type contained by this instance.
	 *
	 */
	inline type_t
	type() const
	{
		return m_type;
	}
	
	// TODO: if the current type is the same as val, should the value be cleared?
	/*!	\brief
	 *
	 *
	 */
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
			
			case type_t::floating:
			{
				m_data.m_floating = 0.0f;
			}
			break;
			
			case type_t::string:
			{
				new ( &m_data.m_string ) string_rep();
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

			case type_t::blob:
			{
				new ( &m_data.m_blob ) blob_type();
			}
			break;
			
			default:
			{
			}
            break;
		}
	}

	/*!	\brief Returns true if the type contained by this instance is null.
	 *	\return true if the type contained by this instance is null, false otherwise.
	 *
	 */
	inline bool
	is_null() const
	{
		return ( m_type == type_t::null );
	}

	/*!	\brief Returns true if the type contained by this instance is boolean.
	 *	\return true if the type contained by this instance is boolean, false otherwise.
	 *
	 */
	inline bool
	is_bool() const
	{
		return ( m_type == type_t::boolean );
	}

	/*!	\brief Returns true if the type contained by this instance is integer.
	 *	\return true if the type contained by this instance is integer, false otherwise.
	 *
	 */
	inline bool
	is_integer() const
	{
		return ( m_type == type_t::integer );
	}
	
	/*!	\brief Returns true if the type contained by this instance is floating.
	 *	\return true if the type contained by this instance is floating, false otherwise.
	 *
	 */
	inline bool
	is_floating() const
	{
		return ( m_type == type_t::floating );
	}

	/*!	\brief Returns true if the type contained by this instance is string.
	 *	\return true if the type contained by this instance is string, false otherwise.
	 *
	 */
	inline bool
	is_string() const
	{
		return ( m_type == type_t::string );
	}
	
	/*!	\brief Returns true if the type contained by this instance is blob.
	 *	\return true if the type contained by this instance is blob, false otherwise.
	 *
	 */
	inline bool
	is_blob() const
	{
		return ( m_type == type_t::blob );
	}
		
	/*!	\brief Returns true if the type contained by this instance is array.
	 *	\return true if the type contained by this instance is array, false otherwise.
	 *
	 */
	inline bool
	is_array() const
	{
		return ( m_type == type_t::array );
	}

	/*!	\brief Returns true if the type contained by this instance is object.
	 *	\return true if the type contained by this instance is object, false otherwise.
	 *
	 */
	inline bool
	is_object() const
	{
		return ( m_type == type_t::object );
	}

	/*!	\brief Returns the boolean value contained by this instance.
	 *	\return the contained boolean value.
	 *
	 *	If the type contained by this instance is not boolean, the returned
	 *	value is false.
	 */
	inline bool
	to_bool() const
	{
		return is_bool() ? m_data.m_bool : false;
	}
	
	template<class T>
	inline typename std::enable_if_t< std::is_integral<T>::value, T >
	to_integer() const
	{
		return is_integer() ? static_cast< T >( m_data.m_integer ) : 0;
	}

	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::uint8_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::uint8_t
	to_uint8() const
	{
		return is_integer() ? static_cast< std::uint8_t >( m_data.m_integer ) : 0;
	}
	
	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::int8_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::int8_t
	to_int8() const
	{
		return is_integer() ? static_cast< std::int8_t >( m_data.m_integer ) : 0;
	}
	
	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::uint16_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::uint16_t
	to_uint16() const
	{
		return is_integer() ? static_cast< std::uint16_t >( m_data.m_integer ) : 0;
	}
	
	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::int16_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::int16_t
	to_int16() const
	{
		return is_integer() ? static_cast< std::int16_t >( m_data.m_integer ) : 0;
	}
	
	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::uint32_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::uint32_t
	to_uint32() const
	{
		return is_integer() ? static_cast< std::uint32_t >( m_data.m_integer ) : 0;
	}
	
	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::int32_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::int32_t
	to_int32() const
	{
		return is_integer() ? static_cast< std::int32_t >( m_data.m_integer ) : 0;
	}
	
	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::int64_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::int64_t
	to_int64() const
	{
		return is_integer() ? m_data.m_integer : 0;
	}
	
	/*!	\brief Returns the integer value contained by this instance.
	 *	\return the contained integer value.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type std::uint64_t. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 *	
	 */
	inline std::uint64_t
	to_uint64() const
	{
		return ( is_integer() && m_data.m_integer >= 0 ) ? static_cast< std::uint64_t >( m_data.m_integer ) : 0;
	}

	/*!	\brief
	 *
	 *
	 */
	inline auto
	to_oid() const
	{
		return to_int64();
	}
	
	/*!	\brief Returns the integer value contained by this instance as a time_point.
	 *	\return the contained integer value, converted to a time_point.
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type time_t, and converted to a time_point. If the type contained by this instance is not integer,
	 *	the returned value is zero.
	 */
	std::chrono::system_clock::time_point
	to_time() const
	{
		return std::chrono::system_clock::from_time_t( static_cast< std::time_t >( is_integer() ? m_data.m_integer : 0 ) );
	}
	
	/*!	\brief Returns the contained integer value, as a value of type log::level_t
	 *	\return the contained value, cast to type log::level_t
	 *
	 *	If this instance contains an integer value, the value of the internal representation
	 *	is cast to type log::level_t. If the type contained by this instance is not integer,
	 *	the returned value is log::level_t::info.
	 */
	log::level_t
	to_log_level() const
	{
		return is_integer() ? static_cast< log::level_t >( m_data.m_integer ) : log::level_t::info;
	}

	/*!	\brief Returns the floating value contained by this instance.
	 *	\return the contained floating value.
	 *
	 *	If the type contained by this instance is not floating, the returned value is zero (0.0).
	 */
	double
	to_floating() const
	{
		return is_floating() ? m_data.m_floating : 0;
	}
	
	/*!	\brief Returns a string_view of the string value contained by this instance.
	 *	\return a view of the contained string value.
	 *
	 *	If the type contained by this instance is not string, the returned value is a
	 *	zero-length string_view.
	 */
	std::string_view
	to_string() const
	{
		return is_string() ? m_data.m_string.view() : std::string_view{};
	}
	
	/*!	\brief Returns a vector of strings reflecting the contents of an array contained by this instance.
	 *	\return A vector of strings.
	 *
	 *	If this instance contains an array, the resulting vector contains values corresponding to 
	 *	the result of to_string() being called on each member of the array, in order. The resulting
	 *	 values for non-string array elements are zero-length strings.
	 */
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
	
	nodeoze::buffer
	to_blob() const
	{
		return is_blob() ? m_data.m_blob : nodeoze::buffer{};
	}
	
	/*!	\brief Returns the array value contained by this instance.
	 *	\return A refernce to the contained array.
	 *
	 *	TODO: we should not be returning a non-const reference to the m_empty_array, I think. Maybe.
	 */
	array_type&
	to_array()
	{
		return is_array() ? m_data.m_array : m_empty_array;
	}
	
	/*!	\brief
	 *
	 *
	 */
	const array_type&
	to_array() const
	{
		return is_array() ? m_data.m_array : m_empty_array;
	}
	
	/*!	\brief
	 *
	 *	TODO: we should not be returning a non-const reference to m_empty_object.
	 */
	object_type&
	to_object()
	{
		return is_object() ? m_data.m_object : m_empty_object;
	}
	
	/*!	\brief
	 *
	 *
	 */
	const object_type&
	to_object() const
	{
		return is_object() ? m_data.m_object : m_empty_object;
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline object_type::iterator
	object_begin()
	{
		return is_object() ? m_data.m_object.begin() : m_empty_object.begin();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline object_type::const_iterator
	object_begin() const
	{
		return is_object() ? m_data.m_object.begin() : m_empty_object.begin();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline object_type::const_iterator
	object_cbegin() const
	{
		return is_object() ? m_data.m_object.cbegin() : m_empty_object.cbegin();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline object_type::iterator
	object_end()
	{
		return is_object() ? m_data.m_object.end() : m_empty_object.end();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline object_type::const_iterator
	object_end() const
	{
		return is_object() ? m_data.m_object.end() : m_empty_object.end();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline object_type::const_iterator
	object_cend() const
	{
		return is_object() ? m_data.m_object.cend() : m_empty_object.cend();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline array_type::iterator
	begin()
	{
		return is_array() ? m_data.m_array.begin() : m_empty_array.begin();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline array_type::const_iterator
	begin() const
	{
		return is_array() ? m_data.m_array.begin() : m_empty_array.begin();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline array_type::const_iterator
	cbegin() const
	{
		return is_array() ? m_data.m_array.cbegin() : m_empty_array.cbegin();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline array_type::iterator
	end()
	{
		return is_array() ? m_data.m_array.end() : m_empty_array.end();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline array_type::const_iterator
	end() const
	{
		return is_array() ? m_data.m_array.end() : m_empty_array.end();
	}
	
	/*!	\brief
	 *
	 *
	 */
	inline array_type::const_iterator
	cend() const
	{
		return is_array() ? m_data.m_array.cend() : m_empty_array.cend();
	}
		
	// TODO: duh
	/*!	\brief
	 *
	 *
	 */
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
	
	/*!	\brief
	 *
	 *
	 */
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
	
	/*!	\brief
	 *
	 *
	 */
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

	/*!	\brief
	 *
	 *
	 */
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

	/*!	\brief
	 *
	 *
	 */
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

	/*!	\brief
	 *
	 *
	 */
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
	
	/*!	\brief
	 *
	 *
	 */
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

	/*!	\brief
	 *
	 *
	 */
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

	/*!	\brief
	 *
	 *
	 */
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
	
	/*!	\brief
	 *
	 *
	 */
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

	/*!	\brief
	 *
	 *
	 */
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
	
	/*!	\brief
	 *
	 *
	 */
	find_t
	find( const std::string_view &pointer ) const;
	
	/*!	\brief
	 *
	 *
	 */
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
	
	/*!	\brief
	 *
	 *
	 */
	bool // TODO: figure out what's going on here
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

	/*!	\brief
	 *
	 *
	 */
	void
	sanitize();
	
	/*!	\brief
	 *
	 *
	 */
	std::error_code
	patch( const any& patches );

private:

	struct string_rep
	{
		inline 
		string_rep()
		:
		m_is_alias{ false }
		{
			new ( &m_value ) string_type{};
		}

		inline 
		string_rep(std::string const& val)
		:
		m_is_alias{ false }
		{
			new ( &m_value ) string_type{ val };
		}

		inline 
		string_rep(std::string&& val)
		:
		m_is_alias{ false }
		{
			new ( &m_value ) string_type{ std::move( val ) };
		}

		inline 
		string_rep(string_rep const& rhs)
		:
		m_is_alias{ rhs.m_is_alias }
		{
			if ( m_is_alias )
			{
				new ( &m_alias ) string_alias{ rhs.m_alias };
			}
			else
			{
				new ( &m_value ) string_type{ rhs.m_value };
			}
		}

		inline 
		string_rep( string_rep&& rhs )
		:
		m_is_alias{ rhs.m_is_alias }
		{
			if ( m_is_alias )
			{
				new ( &m_alias ) string_alias{ std::move( rhs.m_alias ) };
			}
			else
			{
				new ( &m_value ) string_type{ std::move( rhs.m_value ) };
			}
		}

		inline
		string_rep( string_alias const& alias )
		:
		m_is_alias{ true }
		{
			new ( &m_alias ) string_alias( alias );
		}

		inline
		string_rep( string_alias&& alias )
		:
		m_is_alias{ true }
		{
			new ( &m_alias ) string_alias( std::move( alias ) );
		}

		inline std::string_view 
		view() const
		{
			if ( !m_is_alias )
			{
				return std::string_view{ m_value };
			}
			else
			{
				return m_alias.view();
			}
		}

		inline std::string 
		value() const
		{
			return std::string(view());
		}

		inline bool
		empty() const
		{
			return size() == 0;
		}

		inline std::size_t
		size() const
		{
			return view().size();
		}

		inline 
		~string_rep()
		{
			clear();
		}

		inline void
		clear()
		{
			if ( m_is_alias )
			{
				m_alias.~string_alias();
			}
			else
			{
				m_value.~string_type();
			}
		}

		bool m_is_alias;

		union
		{
			string_alias	m_alias;
			string_type		m_value;
		};
	};

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
			m_floating( val )
		{
		}
		
		bool			m_bool;
		std::int64_t	m_integer;
		double			m_floating;
		string_rep		m_string;
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
			std::string lower{obj.to_string()};
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
			
			case type_t::floating:
			{
				m_data.m_floating = rhs.m_data.m_floating;
			}
			break;
		
			// TODO: should copy always make a string_type value copy?
			case type_t::string:
			{
				new ( &m_data.m_string ) string_rep( rhs.m_data.m_string );
			}
			break;
			
			case type_t::blob:
			{
				new ( &m_data.m_blob ) blob_type( rhs.m_data.m_blob );
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
			
			case type_t::floating:
			{
				m_data.m_floating = rhs.m_data.m_floating;
			}
			break;
			
			case type_t::string:
			{
				new ( &m_data.m_string ) string_rep( std::move( rhs.m_data.m_string ) );
			}
			break;
			
			// TODO: figure out what to do on move
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
				m_data.m_string.~string_rep();
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

	static array_type			m_empty_array;
	static object_type			m_empty_object;
	type_t						m_type;
	data						m_data;
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
	
		case any::type_t::floating:
		{
			os << "floating";
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
