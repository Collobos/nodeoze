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

#include <nodeoze/nany.h>
#include <nodeoze/nunicode.h>
#include <nodeoze/nlog.h>
#include <nodeoze/njson.h>
#include <nodeoze/nmsgpack.h>
#include <nodeoze/nmachine.h>
#include <nodeoze/ntimer.h>
#include <nodeoze/njson.h>
#include <nodeoze/ntest.h>
#include <nodeoze/bstream/msgpack.h>
#include "nany_tests.h"
#include <cassert>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <stack>
#include <list>

#undef min

using namespace nodeoze;

static struct force_link
{
	force_link()
	{
		json::link();
		mpack::link();
	}
	
} g_link;


nodeoze::ostream&
nodeoze::operator<<( nodeoze::ostream &os, const any &root )
{
	return json::deflate_to_stream( os, root );
}


std::ostream&
nodeoze::operator<<( std::ostream &os, const any &root )
{
	return json::deflate_to_stream( os, root );
}


any::array_type		any::m_empty_array;
any::object_type	any::m_empty_object;



any
any::array()
{
	return any( type_t::array );
}


any
any::object()
{
	return any( type_t::object );
}


const any&
any::null()
{
	static const any v;
	
	return v;
}


any::any( std::initializer_list< any > init, bool type_deduction, type_t manual_type )
{
	/*
	 * This code was swiped from:
	 *
	 * https://github.com/nlohmann/json/blob/develop/src/json.hpp
	 *
	 * Many thanks to Niels Lohmann
	 */
	
	/*
	 * check if each element is an array with two elements whose first
	 * element is a string
	 */
		
	bool is_an_object = ( init.size() > 0 ) && std::all_of( init.begin(), init.end(), []( const any &element )
	{
		return element.is_array() && ( element.size() == 2 ) && ( element[0].is_string() );
	} );

	/*
	 * adjust type if type deduction is not wanted
	 */
	
	if ( !type_deduction )
	{
		/*
		 * if array is wanted, do not create an object though possible
		 */
		
		if ( manual_type == type_t::array )
		{
			is_an_object = false;
		}

		/*
		 * if object is wanted but impossible, initialize with null
		 */
		
		if ( ( manual_type == type_t::object ) && !is_an_object )
		{
		}
	}

	if ( is_an_object )
	{
		m_type = type_t::object;
		new ( &m_data.m_object ) object_type();

		std::for_each( init.begin(), init.end(), [this]( const any &element )
		{
			m_data.m_object.emplace( std::piecewise_construct, std::forward_as_tuple( element[ 0 ].to_string() ), std::forward_as_tuple( element[ 1 ] ) );
		} );
	}
	else
	{
		m_type = type_t::array;
		new ( &m_data.m_array ) array_type();

		std::for_each( init.begin(), init.end(), [this]( const any &element )
		{
			m_data.m_array.emplace_back( element );
		} );
	}
}

any::any(bstream::ibstream& is)
{
	using namespace bstream;

	auto code = is.peek();
	if (typecode::is_int(code))
	{
		m_type = type_t::integer;
		m_data.m_integer = static_cast< std::uint64_t >(is.read_as<std::int64_t>());
	}
	else if (typecode::is_floating(code))
	{
		m_type = type_t::real;
		m_data.m_real = is.read_as<double>();
	}
	else if (typecode::is_string(code))
	{
		m_type = type_t::string;
		new ( &m_data.m_string ) string_type(is.read_as<std::string>());
	}
	else if (typecode::is_bool(code))
	{
		m_type = type_t::boolean;
		m_data.m_bool = is.read_as<bool>();
	}
	else if (typecode::is_blob(code))
	{
		m_type = type_t::blob;
		auto blob_size = is.read_blob_header();
		new ( &m_data.m_blob ) blob_type(is.get_bytes(blob_size), blob_size);
	}
	else if (typecode::is_array(code))
	{
		m_type = type_t::array;
		auto array_size = is.read_array_header();
		new ( &m_data.m_array ) array_type();
		for(auto i = 0ul; i < array_size; ++i)
		{
			 m_data.m_array.emplace_back(is);
		}
	}
	else if (typecode::is_map(code))
	{
		m_type = type_t::object;
		auto map_size = is.read_map_header();
		new ( &m_data.m_object ) object_type();
		for (auto i = 0ul; i < map_size; ++i)
		{
			std::string key = is.read_as<std::string>();
			m_data.m_object.emplace( std::move(key), is);
		}
		
	}
	else if (typecode::is_nil(code))
	{
		m_type = type_t::null;
	}
	else
	{
		assert(false);
	}

}

bstream::obstream& any::put(bstream::obstream& os) const
{
	switch ( m_type )
	{
		case type_t::null:
		{
			os.write_nil();
		}
		break;
		
		case type_t::boolean:
		{
			os << m_data.m_bool;
		}
		break;
		
		case type_t::integer:
		{
			os << m_data.m_integer;
		}
		break;

		case type_t::real:
		{
			os << m_data.m_real;
		}
		break;
		
		case type_t::string:
		{
			os << m_data.m_string;
		}
		break;
		
		case type_t::blob:
		{
			os.write_blob_header(m_data.m_blob.size());
			os.write_blob_body(m_data.m_blob.data(), m_data.m_blob.size());
		}
		break;

		case type_t::array:
		{
			os.write_array_header(m_data.m_array.size());
			for (auto it = m_data.m_array.cbegin(); it != m_data.m_array.cend(); ++it)
			{
				it->put(os);
			}
		}
		break;

		case type_t::object:
		{
			os.write_map_header(m_data.m_object.size());
			for (auto it = m_data.m_object.cbegin(); it != m_data.m_object.cend(); ++it)
			{
				os << it->first;
				it->second.put(os);
			}
		}
		break;
		
		default:
		{
			assert(false);
		}
		break;
	}
	return os;
}


bool
any::equals( const any &rhs ) const
{
	bool ok;

	if ( this == &rhs )
	{
		ok = true;
		goto exit;
	}
	
	if ( m_type != rhs.m_type )
	{
		ok = false;
		goto exit;
	}
	
	switch ( m_type )
	{
		case type_t::null:
		{
			ok = true;
		}
		break;
		
		case type_t::boolean:
		{
			ok = ( m_data.m_bool == rhs.m_data.m_bool );
		}
		break;
		
		case type_t::integer:
		{
			ok = ( m_data.m_integer == rhs.m_data.m_integer );
		}
		break;

		case type_t::real:
		{
			ok = ( m_data.m_real == rhs.m_data.m_real );
		}
		break;
		
		case type_t::string:
		{
			ok = ( m_data.m_string == rhs.m_data.m_string );
		}
		break;
		
		case type_t::blob:
		{
			ok = ( m_data.m_blob == rhs.m_data.m_blob );
		}
		break;

		case type_t::array:
		{
			ok = ( m_data.m_array == rhs.m_data.m_array );
		}
		break;

		case type_t::object:
		{
			ok = ( m_data.m_object == rhs.m_data.m_object );
		}
		break;
		
		default:
		{
			ok = false;
		}
		break;
	}
	
exit:

	return ok;
}


any::find_t
any::find( const std::string &pointer ) const
{
	auto ret = find_t();
	
	if ( pointer.size() > 0 )
	{
		ret.tokens = string::split( pointer, '/' );
		
		if ( pointer.back() == '/' )
		{
			ret.tokens.emplace_back( std::string() );
		}
		
		for ( ret.index = 0; ret.index < ret.tokens.size(); ret.index++ )
		{
			auto token = ret.tokens[ ret.index ];
			
			string::find_and_replace_in_place( token, "~1", "/" );
			string::find_and_replace_in_place( token, "~0", "~" );
			
			if ( ret.parent )
			{
				if ( ret.parent->is_array() )
				{
					ret.parent = std::addressof( *ret.a_child );
				}
				else if ( ret.parent->is_object() )
				{
					ret.parent = std::addressof( ret.o_child->second );
				}
				else
				{
					ret.valid = false;
					break;
				}
			}
			else
			{
				ret.parent = const_cast< any* >( this );
			}
			
			if ( ret.parent->type() == type_t::array )
			{
				if ( token == "-" )
				{
					ret.a_child = ret.parent->m_data.m_array.end();
					break;
				}
				else
				{
					try
					{
						auto index = std::uint32_t( std::stoi( token ) );
						
						if ( index < ret.parent->size() )
						{
							ret.a_child	= ret.parent->m_data.m_array.begin() + index;
						}
						else
						{
							ret.valid = false;
							break;
						}
					}
					catch( ... )
					{
						ret.valid = false;
						break;
					}
				}
			}
			else if ( ret.parent->type() == type_t::object )
			{
				auto it = ret.parent->m_data.m_object.find( token );
				
				if ( it != ret.parent->m_data.m_object.end() )
				{
					ret.o_child	= it;
				}
				else
				{
					ret.valid = false;
					break;
				}
			}
			else
			{
				ret.valid = false;
				break;
			}
		}
	}
	
	return ret;
}


std::error_code
any::patch( const any &patches )
{
	auto ret = std::error_code();
	
	ncheck_error_action_quiet( patches.is_array(), ret = make_error_code( std::errc::invalid_argument ), exit );
	
	for ( auto i = 0u; i < patches.size(); i++ )
	{
		ret = apply( patches[ i ] );
		
		if ( ret )
		{
			break;
		}
	}
	
exit:
	
	return ret;
}


std::error_code
any::apply( const any &patch )
{
	auto op		= decode_patch_operation( patch[ "op" ] );
	auto path	= patch[ "path" ];
	auto from	= patch[ "from" ];
	auto ret	= std::error_code();
	
	ncheck_error_action( op != patch_opcode_t::invalid, ret = make_error_code( std::errc::invalid_argument ), exit, "op is not valid (%)", patch[ "op" ] );
	ncheck_error_action( path.is_string(), ret = make_error_code( std::errc::invalid_argument ), exit, "path must be string" );
	
	if ( op == patch_opcode_t::test )
	{
		auto find = this->find( path.to_string() );
		ncheck_error_action( find, ret = make_error_code( std::errc::invalid_argument ), exit, "bad path" );
	}
	else if ( path.size() == 0 )
	{
		if ( op == patch_opcode_t::remove )
		{
			set_type( type_t::null );
		}
			
		if ( ( op == patch_opcode_t::replace ) || ( op == patch_opcode_t::add ) )
		{
			auto value = patch[ "value" ];
			ncheck_error_action( !value.is_null(), ret = make_error_code( std::errc::invalid_argument ), exit, "no value for replace/add" );
			*this = std::move( value );
		}
	}
	else
	{
		auto	find	= this->find( path.to_string() );
		auto	value	= any();
		auto	add		= true;
		
		switch ( op )
		{
			case patch_opcode_t::add:
			{
				value = patch[ "value" ];
				
				ncheck_error_action( find || ( find.token_mismatch_distance() == 1 ), ret = make_error_code( std::errc::invalid_argument ), exit, "no value for replace/add" );
				ncheck_error_action( find.parent, ret = make_error_code( std::errc::invalid_argument ), exit, "bad path" );
				ncheck_error_action( !value.is_null(), ret = make_error_code( std::errc::invalid_argument ), exit, "no value for add" );
			}
			break;
			
			case patch_opcode_t::remove:
			{
				ncheck_error_action( find, ret = make_error_code( std::errc::invalid_argument ), exit, "bad path %", path.to_string() );
				ncheck_error_action( find.parent, ret = make_error_code( std::errc::invalid_argument ), exit, "bad path %", path.to_string() );
				
				if ( find.parent->is_array() )
				{
					find.parent->m_data.m_array.erase( find.a_child );
				}
				else if ( find.parent->is_object() )
				{
					find.parent->m_data.m_object.erase( find.o_child );
				}
				else
				{
				}
				
				add = false;
			}
			break;
			
			case patch_opcode_t::replace:
			{
				value = patch[ "value" ];

				if ( !find )
				{
					nlog( log::level_t::info, "uhoh" );
				}
				
				ncheck_error_action( find, ret = make_error_code( std::errc::invalid_argument ), exit, "path not found" );
				ncheck_error_action( find.parent, ret = make_error_code( std::errc::invalid_argument ), exit, "no parent" );
				ncheck_error_action( !value.is_null(), ret = make_error_code( std::errc::invalid_argument ), exit, "no value for add" );

				if ( find.parent->is_array() )
				{
					*find.a_child = value;
				}
				else
				{
					find.o_child->second = value;
				}

				add = false;
			}
			break;
			
			case patch_opcode_t::copy:
			{
				auto from		= find_t();
				auto from_spec	= patch[ "from" ];
				ncheck_error_action( from_spec.is_string(), ret = make_error_code( std::errc::invalid_argument ), exit, "from not string" );
				find = this->find( path.to_string() );
				ncheck_error_action( find || ( find.token_mismatch_distance() == 1 ), ret = make_error_code( std::errc::invalid_argument ), exit, "bad path" );
				from = this->find( from_spec.to_string() );
				ncheck_error_action( from, ret = make_error_code( std::errc::invalid_argument ), exit, "bad from" );
				
				if ( from.parent->is_array() )
				{
					value = *from.a_child;
				}
				else
				{
					value = from.o_child->second;
				}
			}
			break;
			
			case patch_opcode_t::move:
			{
				auto from		= find_t();
				auto from_spec	= patch[ "from" ];
				ncheck_error_action( from_spec.is_string(), ret = make_error_code( std::errc::invalid_argument ), exit, "from not string" );
				find = this->find( path.to_string() );
				ncheck_error_action( find || ( find.token_mismatch_distance() == 1 ), ret = make_error_code( std::errc::invalid_argument ), exit, "bad path" );
				from = this->find( from_spec.to_string() );
				ncheck_error_action( from, ret = make_error_code( std::errc::invalid_argument ), exit, "bad from" );
				
				if ( from.parent->is_array() )
				{
					value = *from.a_child;
					from.parent->m_data.m_array.erase( from.a_child );
				}
				else
				{
					value = from.o_child->second;
					from.parent->m_data.m_object.erase( from.o_child );
				}
			}
			break;
			
			default:
			{
			}
			break;
		}
		
		if ( add )
		{
			if ( find.parent->is_array() )
			{
				if ( find.tokens.back() == "-" )
				{
					find.parent->m_data.m_array.emplace_back( std::move( value ) );
				}
				else
				{
					find.parent->m_data.m_array.insert( find.parent->m_data.m_array.begin() + std::stoi( find.tokens.back() ), value );
				}
			}
			else if ( find.parent->is_object() )
			{
				if ( find )
				{
					find.o_child->second = std::move( value );
				}
				else
				{
					find.parent->m_data.m_object.emplace( std::piecewise_construct, std::forward_as_tuple( find.tokens.front() ), std::forward_as_tuple( std::move( value ) ) );
				}
			}
			else
			{
			}
		}
	}
	
exit:
	
	return ret;
}


void
any::create_patches( any &patches, const std::string &path, const any &lhs, const any &rhs, const std::vector< std::string > &exclusions, bool case_sensitive )
{
	if ( lhs.type() != rhs.type() )
	{
		add_patch( patches, "replace", path, std::string(), rhs, exclusions );
	}
	else
	{
		switch ( lhs.type() )
		{
			case type_t::boolean:
			{
				if ( lhs.to_bool() != rhs.to_bool() )
				{
					add_patch( patches, "replace", path, std::string(), rhs, exclusions );
				}
			}
			break;

			case type_t::integer:
			{
				if ( lhs.to_uint64() != rhs.to_uint64() )
				{
					add_patch( patches, "replace", path, std::string(), rhs, exclusions );
				}
			}
			break;

			case type_t::real:
			{
				if ( lhs.to_real() != rhs.to_real() )
				{
					add_patch( patches, "replace", path, std::string(), rhs, exclusions );
				}
			}
			break;

			case type_t::string:
			{
				if ( lhs.to_string() != rhs.to_string() )
				{
					add_patch( patches, "replace", path, std::string(), rhs, exclusions );
				}
			}
			break;

			case type_t::blob:
			{
				buffer lhs_buf( lhs.to_blob() );
				buffer rhs_buf( rhs.to_blob() );
				
				if ( lhs_buf != rhs_buf )
				{
					add_patch( patches, "replace", path, std::string(), rhs, exclusions);
				}
			}
			break;

			case type_t::array:
			{
				/*
				 * generate patches for all array elements that exist in both lhs and rhs
				 */

				auto min = std::min( lhs.size(), rhs.size() );

				for ( std::size_t index = 0; index < min; index++ )
				{
					create_patches( patches, path + "/" + std::to_string( index ), lhs[ index ], rhs[ index ], exclusions, case_sensitive );
				}

				/*
				 * remove leftover elements from lhs that are not in rhs
				 */

				for ( std::size_t index = min; index < lhs.size(); index++ )
				{
					add_patch( patches, "remove", path, std::to_string( min ), any(), exclusions );
				}

				/*
				 * add new elements to lhs
				 */
				
				for ( std::size_t index = min; index < rhs.size(); index++ )
				{
					add_patch( patches, "add", path, "-", rhs[ index ], exclusions );
				}
			}
			break;

			case type_t::object:
			{
				for ( auto it = lhs.object_begin(); it != lhs.object_end(); it++ )
				{
					if ( rhs.is_member( it->first ) )
					{
						create_patches( patches, path + "/" + it->first, it->second, rhs[ it->first ], exclusions, case_sensitive );
					}
					else
					{
						add_patch( patches, "remove", path, it->first, any(), exclusions );
					}
				}

				for ( auto it = rhs.object_begin(); it != rhs.object_end(); it++ )
				{
					if ( !lhs.is_member( it->first ) )
					{
						add_patch( patches, "add", path, it->first, it->second, exclusions );
					}
				}
			}
			break;
			
			default:
			{
			}
			break;
		}
	}
}


void
any::add_patch( any &patches, const std::string &op, const std::string &path, const std::string &suffix, const any &obj, const std::vector< std::string > &exclusions )
{
	auto actual_path	= suffix.size() > 0 ? path + "/" + suffix : path;
	auto exclude		= std::find( exclusions.begin(), exclusions.end(), actual_path ) != exclusions.end();
	
	if ( !exclude )
	{
		auto patch = any();

		patch[ "op" ]	= op;
		patch[ "path" ]	= actual_path;

		if ( !obj.is_null() )
		{
			patch[ "value"  ] = obj;
		}

		patches.push_back( patch );
	}
}


void
any::sanitize()
{
	switch ( type() )
	{
		case type_t::null:
		{
		}
		break;
		
		case type_t::boolean:
		{
		}
		break;
		
		case type_t::integer:
		{
		}
		break;
		
		case type_t::real:
		{
		}
		break;
		
		case type_t::string:
		{
			std::replace_if( m_data.m_string.begin(), m_data.m_string.end(), []( char c )
			{
				return ( ( c < 32 ) || ( c == '"' ) );
			}, '_' );
		}
		break;
		
		case type_t::blob:
		{
		}
		break;
		
		case type_t::array:
		{
			for ( auto &val : m_data.m_array )
			{
				val.sanitize();
			}
		}
		break;
		
		case type_t::object:
		{
			for ( auto &val : m_data.m_object )
			{
				val.second.sanitize();
			}
		}
		break;
	}
}
	

#if 0
bool
any::operator==(const any &rhs) const
{
	return equal( rhs );
}


bool
any::operator!=(const any &rhs) const
{
	return !equal( rhs );
}


bool
any::operator<(const any &rhs) const
{
	bool result = false;

	if (this != &rhs)
	{
		if ( m_kind == rhs.m_kind)
		{
			switch ( m_kind )
			{
				case type_t::string:
					result = (*m_data.m_string < *m_data.m_string);
					break;

				case type_t::integer:
					result = (*m_data.m_integer < *m_data.m_integer);
					break;

				case type_t::real:
					result = (*m_data.m_real < *m_data.m_real);
					break;

				case type_t::object:
					result = (*m_data.m_object < *m_data.m_object);
					break;

				case type_t::array:
					result = (*m_data.m_array < *m_data.m_array);
					break;

				case type_t::boolean:
					result = (*m_data.m_bool < *m_data.m_bool);
					break;

				default:
					break;
				}

			}
		}

		return result;
	}


bool
any::operator<=(const any &rhs) const
{
	return *this < rhs || *this == rhs;
}


bool
any::operator>(const any &rhs) const
{
		bool result = false;

		if (this != &rhs) {
			if ( m_kind == rhs.m_kind) {
				switch (m_kind) {
				case type_t::string:
					result = (*m_data.m_string > *m_data.m_string);
					break;

				case type_t::integer:
					result = (*m_data.m_integer > *m_data.m_integer);
					break;

				case type_t::real:
					result = (*m_data.m_real > *m_data.m_real);
					break;

				case type_t::object:
					result = (*m_data.m_object > *m_data.m_object);
					break;

				case type_t::array:
					result = (*m_data.m_array > *m_data.m_array);
					break;

				case type_t::boolean:
					result = (*m_data.m_bool	> *m_data.m_bool);
					break;

				default:
					break;
				}

			}
		}

		return result;
	}


bool
any::operator>=(const any &rhs) const
{
	return *this > rhs || *this == rhs;
}
#endif
TEST_CASE( "nodeoze/smoke/any/bstream_blob" )
{
	buffer a_blob("some very stringy blob contents");
	any root(a_blob);
	REQUIRE(root.is_blob());
	bstream::obstream os{1024};
	os << root;
	bstream::ibstream is{std::move(os)};
	any copy(is);
	REQUIRE(copy.type() == any::type_t::blob);
	REQUIRE(root == copy);
}

TEST_CASE( "nodeoze/smoke/any/bstream_write_read" )
{
	buffer a_blob("some stringy contents for a blob");
	any root = any::build(
	{
		{"a", 27 },
		{"b", 1.2345 },
		{"c", "stringy" },
		{"d", true },
		{"e", { 1, 2, 3.0, "more stringy" } },
		{"f", { { "nested", { {"x", "ecks"}, {"y", "why"}, {"z", "zee"} } } } },
		{"g", { 4, 5, 6, { "seven", "eight", 9 } } }
	});
	root["h"] = a_blob;
	REQUIRE(root["h"].is_blob());
	bstream::obstream os{1024};
	os << root;
	bstream::ibstream is{std::move(os)};
	any other(is);
	REQUIRE(other == root);
}	

TEST_CASE( "nodeoze/smoke/any")
{
	SUBCASE( "null check" )
	{
		any root;
		CHECK( root.is_null() );
	}

	SUBCASE( "copy" )
	{
		any root = any::build(
		{
			{ "a", 1 },
			{ "b", 2 }
		} );
		
		any copy( root );
		
		CHECK( copy.is_member( "a" ) );
		CHECK( copy.is_member( "b" ) );
	}

	SUBCASE( "blob" )
	{
		buffer b( { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 } );

		CHECK( b.size() == 6 );
		CHECK( b[ 0 ] == 0x1 );
		CHECK( b[ 1 ] == 0x2 );
		CHECK( b[ 2 ] == 0x3 );
		CHECK( b[ 3 ] == 0x4 );
		CHECK( b[ 4 ] == 0x5 );
		CHECK( b[ 5 ] == 0x6 );

		any a = b;
		CHECK( a.type() == any::type_t::blob );
		CHECK( a.size() == 6 );

		std::string s = a.to_string();

		any c = s;
		CHECK( c.type() == any::type_t::string );
		auto b3 = codec::base64::decode( s );
		buffer b2( c.to_blob() );
		CHECK( c.type() == any::type_t::blob );
		CHECK( b.size() == b2.size() );

		CHECK( b2[ 0 ] == 0x1 );
		CHECK( b2[ 1 ] == 0x2 );
		CHECK( b2[ 2 ] == 0x3 );
		CHECK( b2[ 3 ] == 0x4 );
		CHECK( b2[ 4 ] == 0x5 );
		CHECK( b2[ 5 ] == 0x6 );
	}

	SUBCASE( "in-place array" )
	{
		auto root = any::build(
		{
			"a",
			"b",
			"c"
		} );
		
		CHECK( root.is_array() );
		CHECK( root.size() == 3 );
		CHECK( root[ 0 ] == "a" );
		CHECK( root[ 1 ] == "b" );
		CHECK( root[ 2 ] == "c" );
	}

	SUBCASE( "in-place object" )
	{
		auto root = any::build(
		{
			{ "a", 7 },
			{ "b", 8 },
			{ "c", 9 }
		} );
		
		CHECK( root.is_object() );
		CHECK( root.size() == 3 );
		CHECK( root[ "a" ] == 7 );
		CHECK( root[ "b" ] == 8 );
		CHECK( root[ "c" ] == 9 );
	}

	SUBCASE( "in-place complex json" )
	{
		auto root = any::build(
		{
			{
				{ "a", 7 },
				{ "b", 8 },
				{ "c", 9 }
			},
			{
				{ "a", 10 },
				{ "b", 11 },
				{ "c", 12 }
			}
		} );
		
		CHECK( root.is_array() );
		CHECK( root.size() == 2 );
		CHECK( root[ 0 ][ "a" ] == 7 );
		CHECK( root[ 0 ][ "b" ] == 8 );
		CHECK( root[ 0 ][ "c" ] == 9 );
		CHECK( root[ 1 ][ "a" ] == 10 );
		CHECK( root[ 1 ][ "b" ] == 11 );
		CHECK( root[ 1 ][ "c" ] == 12 );
	}

	SUBCASE( "pointer" )
	{
		auto root = any::build(
		{
			{
				{ "a", 7 }
			},
			{
				{ "b",
					{
						{ "c", "test" },
						{ "d", false }
					},
				}
			},
			{
				{ "e", true }
			},
			{
				{ "f",
					{
						"7",
						true,
						"hello"
					},
				}
			},
			{
				{ "g", "there" }
			}
		} );
		
		REQUIRE( root.is_array() );
		REQUIRE( root.size() == 5 );
		
		auto ret = root.find( std::string() );
		REQUIRE( ret );
		REQUIRE( !ret.parent );
		
		ret = root.find( "/" );
		REQUIRE( !ret );
		
		ret = root.find( "/0/a" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_object() );
		REQUIRE( ret.o_child->first == "a" );
		REQUIRE( ret.o_child->second.is_integer() );
		REQUIRE( ret.o_child->second.to_uint32() == 7 );
		
		ret = root.find( "/0/b" );
		REQUIRE( !ret );
		
		ret = root.find( "/1/b" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_object() );
		REQUIRE( ret.parent->size() == 1 );
		REQUIRE( ret.o_child->first == "b" );
		REQUIRE( ret.o_child->second.is_object() );
		REQUIRE( ret.o_child->second.size() == 2 );
		
		ret = root.find( "/1/b/c" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_object() );
		REQUIRE( ret.parent->size() == 2 );
		REQUIRE( ret.o_child->first == "c" );
		REQUIRE( ret.o_child->second.is_string() );
		REQUIRE( ret.o_child->second.to_string() == "test" );
		
		ret = root.find( "/1/b/d" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_object() );
		REQUIRE( ret.parent->size() == 2 );
		REQUIRE( ret.o_child->first == "d" );
		REQUIRE( ret.o_child->second.is_bool() );
		REQUIRE( ret.o_child->second.to_bool() == false );
		
		ret = root.find( "/2/e" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_object() );
		REQUIRE( ret.parent->size() == 1 );
		REQUIRE( ret.o_child->first == "e" );
		REQUIRE( ret.o_child->second.is_bool() );
		REQUIRE( ret.o_child->second.to_bool() == true );
		
		ret = root.find( "/3/f" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_object() );
		REQUIRE( ret.parent->size() == 1 );
		REQUIRE( ret.o_child->first == "f" );
		REQUIRE( ret.o_child->second.is_array() );
		REQUIRE( ret.o_child->second.size() == 3 );
		
		ret = root.find( "/3/f/0" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_array() );
		REQUIRE( ret.parent->size() == 3 );
		REQUIRE( ret.a_child->is_string() );
		REQUIRE( ret.a_child->to_string() == "7" );
		
		ret = root.find( "/3/f/1" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_array() );
		REQUIRE( ret.parent->size() == 3 );
		REQUIRE( ret.a_child->is_bool() );
		REQUIRE( ret.a_child->to_bool() == true );
		
		ret = root.find( "/3/f/2" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_array() );
		REQUIRE( ret.parent->size() == 3 );
		REQUIRE( ret.a_child->is_string() );
		REQUIRE( ret.a_child->to_string() == "hello" );
		
		ret = root.find( "/3/f/-" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_array() );
		REQUIRE( ret.parent->size() == 3 );
		REQUIRE( ret.a_child == ret.parent->end() );
		
		ret = root.find( "/3/f/3" );
		REQUIRE( !ret );
		
		ret = root.find( "/4/g" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_object() );
		REQUIRE( ret.parent->size() == 1 );
		REQUIRE( ret.o_child->first == "g" );
		REQUIRE( ret.o_child->second.is_string() );
		REQUIRE( ret.o_child->second.to_string() == "there" );
		
		ret = root.find( "/4/h" );
		REQUIRE( !ret );
		
		ret = root.find( "/-" );
		REQUIRE( ret );
		REQUIRE( ret.parent->is_array() );
		REQUIRE( ret.parent->size() == 5 );
		REQUIRE( ret.a_child == ret.parent->end() );
	}

	SUBCASE( "diff" )
	{
		auto m1 = machine::self();
		auto m2 = machine::self();
		
		auto patches = any::diff( m1.to_any(), m2.to_any(), {}, false );
		
		REQUIRE( patches.size() == 0 );
	}

	SUBCASE( "diff with exclusions" )
	{
		auto m1 = machine::self();
		auto m2 = machine::self();
		
		m2.set_cores( 32 );
		
		auto patches = any::diff( m1.to_any(), m2.to_any(), {}, false );
		
		REQUIRE( patches.size() != 0 );
		
		patches = any::diff( m1.to_any(), m2.to_any(), { "/cores" }, false );
		
		REQUIRE( patches.size() == 0 );
	}


	SUBCASE( "diff by removing all members of array" )
	{
		any a = any::array();
		a.emplace_back( "a" );
		a.emplace_back( "b" );
		a.emplace_back( "c" );

		REQUIRE( a.is_array() );
		REQUIRE( a.size() == 3 );

		any b = any::array();

		REQUIRE( b.is_array() );
		REQUIRE( b.size() == 0 );

		auto patches = any::diff( a, b, {}, false );

		REQUIRE( patches.size() == 3 );

		any patched = any::patch( a, patches );

		REQUIRE( patched.is_array() );
		REQUIRE( patched.size() == 0 );
	}

	SUBCASE( "diff by removing some members of array" )
	{
		any a = any::array();
		a.emplace_back( "a" );
		a.emplace_back( "b" );
		a.emplace_back( "c" );
		a.emplace_back( "d" );

		REQUIRE( a.is_array() );
		REQUIRE( a.size() == 4 );

		any b = any::array();
		b.emplace_back( "a" );
		b.emplace_back( "b" );

		REQUIRE( b.is_array() );
		REQUIRE( b.size() == 2 );

		auto patches = any::diff( a, b, {}, false );

		REQUIRE( patches.size() == 2 );

		any patched = any::patch( a, patches );

		REQUIRE( patched.is_array() );
		REQUIRE( patched.size() == 2 );
		REQUIRE( patched[ 0 ].to_string() == "a" );
		REQUIRE( patched[ 1 ].to_string() == "b" );
	}


	SUBCASE( "diff by replacing and removing some members of array" )
	{
		any a = any::array();
		a.emplace_back( "a" );
		a.emplace_back( "b" );
		a.emplace_back( "c" );
		a.emplace_back( "d" );

		REQUIRE( a.is_array() );
		REQUIRE( a.size() == 4 );

		any b = any::array();
		b.emplace_back( "a" );
		b.emplace_back( "c" );

		REQUIRE( b.is_array() );
		REQUIRE( b.size() == 2 );

		auto patches = any::diff( a, b, {}, false );

		REQUIRE( patches.size() == 3 );

		any patched = any::patch( a, patches );

		REQUIRE( patched.is_array() );
		REQUIRE( patched.size() == 2 );
		REQUIRE( patched[ 0 ].to_string() == "a" );
		REQUIRE( patched[ 1 ].to_string() == "c" );
	}

	SUBCASE( "diff by replacing and adding some members of array" )
	{
		any a = any::array();
		a.emplace_back( "a" );
		a.emplace_back( "b" );

		REQUIRE( a.is_array() );
		REQUIRE( a.size() == 2 );

		any b = any::array();
		b.emplace_back( "a" );
		b.emplace_back( "c" );
		b.emplace_back( "d" );
		b.emplace_back( "e" );

		REQUIRE( b.is_array() );
		REQUIRE( b.size() == 4 );

		auto patches = any::diff( a, b, {}, false );

		REQUIRE( patches.size() == 3 );

		any patched = any::patch( a, patches );

		REQUIRE( patched.is_array() );
		REQUIRE( patched.size() == 4 );
		REQUIRE( patched[ 0 ].to_string() == "a" );
		REQUIRE( patched[ 1 ].to_string() == "c" );
		REQUIRE( patched[ 2 ].to_string() == "d" );
		REQUIRE( patched[ 3 ].to_string() == "e" );
	}

	SUBCASE( "diff by replacing and removing some members of object" )
	{
		any a = any::build(
		{
			{ "a", 1 },
			{ "b", 2 },
			{ "c", 3 },
			{ "d", 4 }
		} );

		REQUIRE( a.is_object() );
		REQUIRE( a.size() == 4 );

		any b = any::build(
		{
			{ "a", true },
			{ "b", 7.5 }
		} );

		REQUIRE( b.is_object() );
		REQUIRE( b.size() == 2 );

		auto patches = any::diff( a, b, {}, false );

		REQUIRE( patches.size() == 4 );

		any patched = any::patch( a, patches );

		REQUIRE( patched.is_object() );
		REQUIRE( patched.size() == 2 );
		REQUIRE( patched[ "a" ].is_bool() );
		REQUIRE( patched[ "a" ].to_bool() == true );
		REQUIRE( patched[ "b" ].is_real() );
		REQUIRE( patched[ "b" ].to_real() == 7.5 );
	}

	SUBCASE( "patch empty list, empty docs")
	{
		any doc( any::type_t::object );
		any patches( any::type_t::array );
		
		auto patched = any::patch( doc, patches );
		
		REQUIRE( patched.is_object() );
		REQUIRE( patched.size() == 0 );
	}
	
	SUBCASE( "patch empty patch list" )
	{
		any doc(
		{
			{ "foo", 1 },
		} );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 1 );
		
		auto patched	= any::patch( doc, {} );
		auto expected	= any::build(
		{
			{ "foo", 1 },
		} );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch rearrangements OK?" )
	{
		any doc(
		{
			{ "foo", 1 },
			{ "bar", 2 }
		} );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 2 );
		
		any patches( {} );
		
		auto patched	= any::patch( doc, {} );
		auto expected	= any::build(
		{
			{ "bar", 2 },
			{ "foo", 1 }
		} );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch rearrangements OK?  How about one level down ... array" )
	{
		any doc(
		{
			{
				{ "foo", 1 },
				{ "bar", 2 }
			}
		} );
		
		REQUIRE( doc.is_array() );
		REQUIRE( doc.size() == 1 );
		
		any patches( {} );
		
		auto patched	= any::patch( doc, {} );
		auto expected	= any::build(
		{
			{
				{ "bar", 2 },
				{ "foo", 1 }
			}
		} );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch add replaces any existing field" )
	/*
	 * "patch": [{"op": "add", "path": "/foo", "value":1}],
	 * "expected": {"foo": 1} },
	 */
	{
		any doc(
		{
			{ "foo", any::null() }
		} );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 1 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	"/foo" },
				{ "value",	1 }
			}
		} );
		
		auto patched	= any::patch( doc, patches );
		auto expected	= any::build(
		{
			{ "foo", 1 }
		} );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch toplevel array" )
	/*
      "doc": [],
      "patch": [{"op": "add", "path": "/0", "value": "foo"}],
      "expected": ["foo"] },
	*/
	{
		any doc( {} );
		
		REQUIRE( doc.is_array() );
		REQUIRE( doc.size() == 0 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	"/0" },
				{ "value",	"foo" }
			}
		} );
		
		auto patched	= any::patch( doc, patches );
		auto expected	= any::build(
		{
			"foo"
		} );
		
		REQUIRE( expected.is_array() );
		REQUIRE( expected.size() == 1 );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch toplevel array, no change" )
	/*
      "doc": ["foo"],
      "patch": [],
      "expected": ["foo"] },
	*/
	{
		auto doc = any::build(
		{
			"foo"
		} );
		
		REQUIRE( doc.is_array() );
		REQUIRE( doc.size() == 1 );
		
		any patches( {} );
		
		auto patched	= any::patch( doc, patches );
		auto expected	= any::build(
		{
			"foo"
		} );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch toplevel object, numeric string" )
	/*
      "doc": {},
      "patch": [{"op": "add", "path": "/foo", "value": "1"}],
      "expected": {"foo":"1"} },
	*/
	{
		any doc( any::type_t::object );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 0 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	"/foo"},
				{ "value",	"1" }
			}
		} );
		
		auto patched	= any::patch( doc, patches );
		auto expected	= any::build(
		{
			{ "foo", "1" }
		} );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch toplevel object, integer" )
	/*
      "doc": {},
      "patch": [{"op": "add", "path": "/foo", "value": 1}],
      "expected": {"foo":1} },
	*/
	{
		any doc( any::type_t::object );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 0 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	"/foo"},
				{ "value",	1 }
			}
		} );
		
		auto patched	= any::patch( doc, patches );
		auto expected	= any::build(
		{
			{ "foo", 1 }
		} );
		
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch Toplevel scalar values OK?" )
	/*
      "doc": "foo",
      "patch": [{"op": "replace", "path": "", "value": "bar"}],
      "expected": "bar",
      "disabled": true },
	*/
	{
	/*
		any doc( any::type_t::object );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 0 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	"/foo"},
				{ "value",	1 }
			}
		} );
		
		auto patched = any::patch( doc, patches );
		
		any expected(
		{
			{ "foo", 1 }
		} );
		
		REQUIRE( patched == expected );
	*/
	}
	
	SUBCASE( "patch replace object document with array document?" )
	/*
      "doc": {},
      "patch": [{"op": "add", "path": "", "value": []}],
      "expected": [] },
	*/
	{
		any doc( any::type_t::object );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 0 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	""},
				{ "value",	any( any::type_t::array ) }
			}
		} );
		
		auto patched	= any::patch( doc, patches );
		auto expected	= any::build(
		{
		} );
		
		REQUIRE( expected.is_array() );
		REQUIRE( expected.size() == 0 );
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch replace array document with object document?" )
	/*
      "doc": [],
      "patch": [{"op": "add", "path": "", "value": {}}],
      "expected": {} },
	*/
	{
		any doc( any::type_t::array );
		
		REQUIRE( doc.is_array() );
		REQUIRE( doc.size() == 0 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	""},
				{ "value",	any( any::type_t::object ) }
			}
		} );
		
		auto patched = any::patch( doc, patches );
		
		any expected( any::type_t::object );
		
		REQUIRE( expected.is_object() );
		REQUIRE( expected.size() == 0 );
		REQUIRE( patched == expected );
	}
	
	SUBCASE( "patch replace boolean property" )
	{
		any doc(
		{
			{ "created", false }
		} );
		
		REQUIRE( doc.is_object() );
		REQUIRE( doc.size() == 1 );
		
		any patches(
		{
			{
				{ "op",		"add" },
				{ "path",	"/created"},
				{ "value",	true }
			}
		} );
		
		auto patched = any::patch( doc, patches );
		
		any expected(
		{
			{ "created", true }
		} );
		
		REQUIRE( expected.is_object() );
		REQUIRE( expected.size() == 1 );
		REQUIRE( patched == expected );
	}

	SUBCASE ( "sanitize" )
	{
		auto val = any::build(
		{
			{
				"markers",
				{
					{
						{ "name",		"black cartridge hp cc364x.\r" },
						{ "type",		"toner" },
						{ "level",		25 },
						{ "color",		"#000000" },
						{ "low_level",	0 },
						{ "high_level",	100 }
					}
				}
			}
		} );
		
		REQUIRE( val.is_object() );
		REQUIRE( val.is_member( "markers" ) );
		REQUIRE( val[ "markers" ].is_array() );
		REQUIRE( val[ "markers" ].size() == 1 );
		
		auto copy = val;
		
		REQUIRE( val[ "markers" ][ 0 ][ "name" ] == "black cartridge hp cc364x.\r" );
		REQUIRE( copy[ "markers" ][ 0 ][ "name" ] == "black cartridge hp cc364x.\r" );
		REQUIRE( val == copy );
		
		copy.sanitize();
		
		REQUIRE( val != copy );
		REQUIRE( val[ "markers" ][ 0 ][ "name" ] == "black cartridge hp cc364x.\r" );
		REQUIRE( copy[ "markers" ][ 0 ][ "name" ] == "black cartridge hp cc364x._" );
	}
	
		
#if 0
[





 

 
 
    { "comment": "append to root array document?",
      "doc": [],
      "patch": [{"op": "add", "path": "/-", "value": "hi"}],
      "expected": ["hi"] },

    { "comment": "Add, / target",
      "doc": {},
      "patch": [ {"op": "add", "path": "/", "value":1 } ],
      "expected": {"":1} },

    { "comment": "Add, /foo/ deep target (trailing slash)",
      "doc": {"foo": {}},
      "patch": [ {"op": "add", "path": "/foo/", "value":1 } ],
      "expected": {"foo":{"": 1}} },

    { "comment": "Add composite value at top level",
      "doc": {"foo": 1},
      "patch": [{"op": "add", "path": "/bar", "value": [1, 2]}],
      "expected": {"foo": 1, "bar": [1, 2]} },

    { "comment": "Add into composite value",
      "doc": {"foo": 1, "baz": [{"qux": "hello"}]},
      "patch": [{"op": "add", "path": "/baz/0/foo", "value": "world"}],
      "expected": {"foo": 1, "baz": [{"qux": "hello", "foo": "world"}]} },

    { "doc": {"bar": [1, 2]},
      "patch": [{"op": "add", "path": "/bar/8", "value": "5"}],
      "error": "Out of bounds (upper)" },

    { "doc": {"bar": [1, 2]},
      "patch": [{"op": "add", "path": "/bar/-1", "value": "5"}],
      "error": "Out of bounds (lower)" },

    { "doc": {"foo": 1},
      "patch": [{"op": "add", "path": "/bar", "value": true}],
      "expected": {"foo": 1, "bar": true} },

    { "doc": {"foo": 1},
      "patch": [{"op": "add", "path": "/bar", "value": false}],
      "expected": {"foo": 1, "bar": false} },

    { "doc": {"foo": 1},
      "patch": [{"op": "add", "path": "/bar", "value": null}],
      "expected": {"foo": 1, "bar": null} },

    { "comment": "0 can be an array index or object element name",
      "doc": {"foo": 1},
      "patch": [{"op": "add", "path": "/0", "value": "bar"}],
      "expected": {"foo": 1, "0": "bar" } },

    { "doc": ["foo"],
      "patch": [{"op": "add", "path": "/1", "value": "bar"}],
      "expected": ["foo", "bar"] },

    { "doc": ["foo", "sil"],
      "patch": [{"op": "add", "path": "/1", "value": "bar"}],
      "expected": ["foo", "bar", "sil"] },

    { "doc": ["foo", "sil"],
      "patch": [{"op": "add", "path": "/0", "value": "bar"}],
      "expected": ["bar", "foo", "sil"] },

    { "comment": "push item to array via last index + 1",
      "doc": ["foo", "sil"],
      "patch": [{"op":"add", "path": "/2", "value": "bar"}],
      "expected": ["foo", "sil", "bar"] },

    { "comment": "add item to array at index > length should fail",
      "doc": ["foo", "sil"],
      "patch": [{"op":"add", "path": "/3", "value": "bar"}],
      "error": "index is greater than number of items in array" },
      
    { "comment": "test against implementation-specific numeric parsing",
      "doc": {"1e0": "foo"},
      "patch": [{"op": "test", "path": "/1e0", "value": "foo"}],
      "expected": {"1e0": "foo"} },

    { "comment": "test with bad number should fail",
      "doc": ["foo", "bar"],
      "patch": [{"op": "test", "path": "/1e0", "value": "bar"}],
      "error": "test op shouldn't get array element 1" },

    { "doc": ["foo", "sil"],
      "patch": [{"op": "add", "path": "/bar", "value": 42}],
      "error": "Object operation on array target" },

    { "doc": ["foo", "sil"],
      "patch": [{"op": "add", "path": "/1", "value": ["bar", "baz"]}],
      "expected": ["foo", ["bar", "baz"], "sil"],
      "comment": "value in array add not flattened" },

    { "doc": {"foo": 1, "bar": [1, 2, 3, 4]},
      "patch": [{"op": "remove", "path": "/bar"}],
      "expected": {"foo": 1} },

    { "doc": {"foo": 1, "baz": [{"qux": "hello"}]},
      "patch": [{"op": "remove", "path": "/baz/0/qux"}],
      "expected": {"foo": 1, "baz": [{}]} },

    { "doc": {"foo": 1, "baz": [{"qux": "hello"}]},
      "patch": [{"op": "replace", "path": "/foo", "value": [1, 2, 3, 4]}],
      "expected": {"foo": [1, 2, 3, 4], "baz": [{"qux": "hello"}]} },

    { "doc": {"foo": [1, 2, 3, 4], "baz": [{"qux": "hello"}]},
      "patch": [{"op": "replace", "path": "/baz/0/qux", "value": "world"}],
      "expected": {"foo": [1, 2, 3, 4], "baz": [{"qux": "world"}]} },

    { "doc": ["foo"],
      "patch": [{"op": "replace", "path": "/0", "value": "bar"}],
      "expected": ["bar"] },

    { "doc": [""],
      "patch": [{"op": "replace", "path": "/0", "value": 0}],
      "expected": [0] },

    { "doc": [""],
      "patch": [{"op": "replace", "path": "/0", "value": true}],
      "expected": [true] },

    { "doc": [""],
      "patch": [{"op": "replace", "path": "/0", "value": false}],
      "expected": [false] },

    { "doc": [""],
      "patch": [{"op": "replace", "path": "/0", "value": null}],
      "expected": [null] },

    { "doc": ["foo", "sil"],
      "patch": [{"op": "replace", "path": "/1", "value": ["bar", "baz"]}],
      "expected": ["foo", ["bar", "baz"]],
      "comment": "value in array replace not flattened" },

    { "comment": "replace whole document",
      "doc": {"foo": "bar"},
      "patch": [{"op": "replace", "path": "", "value": {"baz": "qux"}}],
      "expected": {"baz": "qux"} },

    { "comment": "spurious patch properties",
      "doc": {"foo": 1},
      "patch": [{"op": "test", "path": "/foo", "value": 1, "spurious": 1}],
      "expected": {"foo": 1} },

    { "doc": {"foo": null},
      "patch": [{"op": "test", "path": "/foo", "value": null}],
      "comment": "null value should be valid obj property" },

    { "doc": {"foo": null},
      "patch": [{"op": "replace", "path": "/foo", "value": "truthy"}],
      "expected": {"foo": "truthy"},
      "comment": "null value should be valid obj property to be replaced with something truthy" },

    { "doc": {"foo": null},
      "patch": [{"op": "move", "from": "/foo", "path": "/bar"}],
      "expected": {"bar": null},
      "comment": "null value should be valid obj property to be moved" },

    { "doc": {"foo": null},
      "patch": [{"op": "copy", "from": "/foo", "path": "/bar"}],
      "expected": {"foo": null, "bar": null},
      "comment": "null value should be valid obj property to be copied" },

    { "doc": {"foo": null},
      "patch": [{"op": "remove", "path": "/foo"}],
      "expected": {},
      "comment": "null value should be valid obj property to be removed" },

    { "doc": {"foo": "bar"},
      "patch": [{"op": "replace", "path": "/foo", "value": null}],
      "expected": {"foo": null},
      "comment": "null value should still be valid obj property replace other value" },

    { "doc": {"foo": {"foo": 1, "bar": 2}},
      "patch": [{"op": "test", "path": "/foo", "value": {"bar": 2, "foo": 1}}],
      "comment": "test should pass despite rearrangement" },

    { "doc": {"foo": [{"foo": 1, "bar": 2}]},
      "patch": [{"op": "test", "path": "/foo", "value": [{"bar": 2, "foo": 1}]}],
      "comment": "test should pass despite (nested) rearrangement" },

    { "doc": {"foo": {"bar": [1, 2, 5, 4]}},
      "patch": [{"op": "test", "path": "/foo", "value": {"bar": [1, 2, 5, 4]}}],
      "comment": "test should pass - no error" },

    { "doc": {"foo": {"bar": [1, 2, 5, 4]}},
      "patch": [{"op": "test", "path": "/foo", "value": [1, 2]}],
      "error": "test op should fail" },

    { "comment": "Whole document",
      "doc": { "foo": 1 },
      "patch": [{"op": "test", "path": "", "value": {"foo": 1}}],
      "disabled": true },

    { "comment": "Empty-string element",
      "doc": { "": 1 },
      "patch": [{"op": "test", "path": "/", "value": 1}] },

    { "doc": {
            "foo": ["bar", "baz"],
            "": 0,
            "a/b": 1,
            "c%d": 2,
            "e^f": 3,
            "g|h": 4,
            "i\\j": 5,
            "k\"l": 6,
            " ": 7,
            "m~n": 8
            },
      "patch": [{"op": "test", "path": "/foo", "value": ["bar", "baz"]},
                {"op": "test", "path": "/foo/0", "value": "bar"},
                {"op": "test", "path": "/", "value": 0},
                {"op": "test", "path": "/a~1b", "value": 1},
                {"op": "test", "path": "/c%d", "value": 2},
                {"op": "test", "path": "/e^f", "value": 3},
                {"op": "test", "path": "/g|h", "value": 4},
                {"op": "test", "path":  "/i\\j", "value": 5},
                {"op": "test", "path": "/k\"l", "value": 6},
                {"op": "test", "path": "/ ", "value": 7},
                {"op": "test", "path": "/m~0n", "value": 8}] },

    { "comment": "Move to same location has no effect",
      "doc": {"foo": 1},
      "patch": [{"op": "move", "from": "/foo", "path": "/foo"}],
      "expected": {"foo": 1} },

    { "doc": {"foo": 1, "baz": [{"qux": "hello"}]},
      "patch": [{"op": "move", "from": "/foo", "path": "/bar"}],
      "expected": {"baz": [{"qux": "hello"}], "bar": 1} },

    { "doc": {"baz": [{"qux": "hello"}], "bar": 1},
      "patch": [{"op": "move", "from": "/baz/0/qux", "path": "/baz/1"}],
      "expected": {"baz": [{}, "hello"], "bar": 1} },

    { "doc": {"baz": [{"qux": "hello"}], "bar": 1},
      "patch": [{"op": "copy", "from": "/baz/0", "path": "/boo"}],
      "expected": {"baz":[{"qux":"hello"}],"bar":1,"boo":{"qux":"hello"}} },

    { "comment": "replacing the root of the document is possible with add",
      "doc": {"foo": "bar"},
      "patch": [{"op": "add", "path": "", "value": {"baz": "qux"}}],
      "expected": {"baz":"qux"}},

    { "comment": "Adding to \"/-\" adds to the end of the array",
      "doc": [ 1, 2 ],
      "patch": [ { "op": "add", "path": "/-", "value": { "foo": [ "bar", "baz" ] } } ],
      "expected": [ 1, 2, { "foo": [ "bar", "baz" ] } ]},

    { "comment": "Adding to \"/-\" adds to the end of the array, even n levels down",
      "doc": [ 1, 2, [ 3, [ 4, 5 ] ] ],
      "patch": [ { "op": "add", "path": "/2/1/-", "value": { "foo": [ "bar", "baz" ] } } ],
      "expected": [ 1, 2, [ 3, [ 4, 5, { "foo": [ "bar", "baz" ] } ] ] ]},

    { "comment": "test remove with bad number should fail",
      "doc": {"foo": 1, "baz": [{"qux": "hello"}]},
      "patch": [{"op": "remove", "path": "/baz/1e0/qux"}],
      "error": "remove op shouldn't remove from array with bad number" },

    { "comment": "test remove on array",
      "doc": [1, 2, 3, 4],
      "patch": [{"op": "remove", "path": "/0"}],
      "expected": [2, 3, 4] },

    { "comment": "test repeated removes",
      "doc": [1, 2, 3, 4],
      "patch": [{ "op": "remove", "path": "/1" },
                { "op": "remove", "path": "/2" }],
      "expected": [1, 3] },

    { "comment": "test remove with bad index should fail",
      "doc": [1, 2, 3, 4],
      "patch": [{"op": "remove", "path": "/1e0"}],
      "error": "remove op shouldn't remove from array with bad number" },

    { "comment": "test replace with bad number should fail",
      "doc": [""],
      "patch": [{"op": "replace", "path": "/1e0", "value": false}],
      "error": "replace op shouldn't replace in array with bad number" },

    { "comment": "test copy with bad number should fail",
      "doc": {"baz": [1,2,3], "bar": 1},
      "patch": [{"op": "copy", "from": "/baz/1e0", "path": "/boo"}],
      "error": "copy op shouldn't work with bad number" },

    { "comment": "test move with bad number should fail",
      "doc": {"foo": 1, "baz": [1,2,3,4]},
      "patch": [{"op": "move", "from": "/baz/1e0", "path": "/foo"}],
      "error": "move op shouldn't work with bad number" },

    { "comment": "test add with bad number should fail",
      "doc": ["foo", "sil"],
      "patch": [{"op": "add", "path": "/1e0", "value": "bar"}],
      "error": "add op shouldn't add to array with bad number" },

    { "comment": "missing 'value' parameter to add",
      "doc": [ 1 ],
      "patch": [ { "op": "add", "path": "/-" } ],
      "error": "missing 'value' parameter" },

    { "comment": "missing 'value' parameter to replace",
      "doc": [ 1 ],
      "patch": [ { "op": "replace", "path": "/0" } ],
      "error": "missing 'value' parameter" },

    { "comment": "missing 'value' parameter to test",
      "doc": [ null ],
      "patch": [ { "op": "test", "path": "/0" } ],
      "error": "missing 'value' parameter" },

    { "comment": "missing value parameter to test - where undef is falsy",
      "doc": [ false ],
      "patch": [ { "op": "test", "path": "/0" } ],
      "error": "missing 'value' parameter" },

    { "comment": "missing from parameter to copy",
      "doc": [ 1 ],
      "patch": [ { "op": "copy", "path": "/-" } ],
      "error": "missing 'from' parameter" },

    { "comment": "missing from parameter to move",
      "doc": { "foo": 1 },
      "patch": [ { "op": "move", "path": "" } ],
      "error": "missing 'from' parameter" },

    { "comment": "duplicate ops",
      "doc": { "foo": "bar" },
      "patch": [ { "op": "add", "path": "/baz", "value": "qux",
                   "op": "move", "from":"/foo" } ],
      "error": "patch has two 'op' members",
      "disabled": true },

    { "comment": "unrecognized op should fail",
      "doc": {"foo": 1},
      "patch": [{"op": "spam", "path": "/foo", "value": 1}],
      "error": "Unrecognized op 'spam'" },

    { "comment": "test with bad array number that has leading zeros",
      "doc": ["foo", "bar"],
      "patch": [{"op": "test", "path": "/00", "value": "foo"}],
      "error": "test op should reject the array value, it has leading zeros" },

    { "comment": "test with bad array number that has leading zeros",
      "doc": ["foo", "bar"],
      "patch": [{"op": "test", "path": "/01", "value": "bar"}],
      "error": "test op should reject the array value, it has leading zeros" },

    { "comment": "Removing nonexistent field",
      "doc": {"foo" : "bar"},
      "patch": [{"op": "remove", "path": "/baz"}],
      "error": "removing a nonexistent field should fail" },

    { "comment": "Removing nonexistent index",
      "doc": ["foo", "bar"],
      "patch": [{"op": "remove", "path": "/2"}],
      "error": "removing a nonexistent index should fail" },

    { "comment": "Patch with different capitalisation than doc",
       "doc": {"foo":"bar"},
       "patch": [{"op": "add", "path": "/FOO", "value": "BAR"}],
       "expected": {"foo": "bar", "FOO": "BAR"}
    }

]
#endif
}


TEST_CASE( "nodeoze/perf/any" )
{
	SUBCASE( "deflating" )
	{
		auto input = std::string( "{ \"table\": \"service\", \"created_logical\": 103, \"oid\": 48, \"acl\": [], \"created\": 1499359490, \"modified\": 1505243815, \"path\": \"/services/48\", \"name\": \"Brother HL-L6200DW (Windows Development)\", \"modified_logical\": 7284, \"tags\": [ \"Test 1\" ], \"location\": { \"latitude\": 37.44950103759765625, \"longitude\": -122.17974090576171875, \"horizontal_accuracy\": 0.0, \"altitude\": 0.0, \"size\": 0.0, \"vertical_accuracy\": 0.0 }, \"port\": 9631, \"agent\": 56, \"type\": [ \"_ipp\", \"_tcp\" ], \"status\": 0, \"host\": [ \"NAUSEA\", \"dnssd\", \"presto\" ], \"mac_addresses\": [ \"ABVdndfI\", \"ABxCDS4S\" ], \"addresses\": [ [ 172, 16, 80, 1 ], [ 192, 168, 1, 60 ] ], \"text_record\": \"CXR4dHZlcnM9MQhxdG90YWw9MQpwcmlvcml0eT0wB0NvbG9yPUYIRHVwbGV4PVQIQ29waWVzPVQtVVVJRD1Ccm90aGVyIEhMLUw2MjAwRFcgKFdpbmRvd3MgRGV2ZWxvcG1lbnQpBHFwPU4jcHJvZHVjdD0oQnJvdGhlciBITC1MNjIwMERXIHNlcmllcykcdHk9QnJvdGhlciBITC1MNjIwMERXIHNlcmllcyFVUkY9VzgsRE0zLFNSR0IyNCxDUDI1NSxSUzMwMC02MDAycGRsPWFwcGxpY2F0aW9uL3BkZixpbWFnZS9qcGVnLGltYWdlL3BuZyxpbWFnZS91cmYOcnA9c2VydmljZXMvNDgHVExTPTEuMhVhaXI9dXNlcm5hbWUscGFzc3dvcmQFbm90ZT0=\", \"priority\": 0, \"weight\": 0, \"printer\": { \"note\": \"\", \"info\": \"Brother HL-L6200DW (Windows Development)\", \"resource\": \"winspool://127.0.0.1/brother_hl_l6200dw__windows_development_\", \"name\": \"Brother HL-L6200DW (Windows Development)\", \"page_count\": 74, \"uuid\": \"Brother HL-L6200DW (Windows Development)\", \"media_ready\": [ \"na_letter_8.5x11in\", \"na_legal_8.5x14in\" ], \"kind\": 1, \"side_supported\": [ 0, 1, 2 ], \"make_and_model\": \"Brother HL-L6200DW series\", \"device_id\": \"\", \"quality_default\": 4, \"markers\": [ { \"name\": \"BK\", \"level\": 100, \"type\": \"toner\", \"low_level\": 7, \"color\": \"#000000\", \"high_level\": 100 } ], \"bpp\": 8, \"ipp\": true, \"side_default\": 0, \"copy\": 0, \"orientation_default\": 3, \"quality_supported\": [ 3, 4, 5 ], \"orientation_supported\": [ 3, 4 ], \"media_default\": \"na_letter_8.5x11in\", \"state\": 3, \"release\": false, \"reasons\": 0, \"secure\": true, \"strict\": false, \"provider\": \"System\", \"connection\": \"net://192.168.1.84\", \"airscan\": false, \"airprint\": true, \"snmp\": true } }" );
		
		const std::size_t count = 100000;
		
		auto any = json::inflate( input );
		
		ostringstream os;
		os << any;
		
		auto any2 = json::inflate( os.str() );
		
		//CHECK( any == any2 );
		
		auto elapsed = stopwatch::run( [&]() mutable
		{
			for ( auto i = 0u; i < count; i++ )
			{
				std::ostringstream os;
				os << any;
			}
		} );
		
		auto milli = static_cast< double >( std::chrono::duration_cast< std::chrono::milliseconds >( elapsed ).count() );
		auto per = milli / static_cast< double >( count );
		
		fprintf( stderr, "write: elapsed time: %f\n", milli );
		fprintf( stderr, "write: %f msec per object\n", per );
		fprintf( stderr, "write: %f objects per second\n", 1000.00 / per );
		
		elapsed = stopwatch::run( [&]() mutable
		{
			for ( auto i = 0u; i < count; i++ )
			{
				ostringstream os;
				os << any;
			}
		} );
		
		milli = static_cast< double >( std::chrono::duration_cast< std::chrono::milliseconds >( elapsed ).count() );
		per = milli / static_cast< double >( count );
		
		fprintf( stderr, "write: elapsed time: %f\n", milli );
		fprintf( stderr, "write: %f msec per object\n", per );
		fprintf( stderr, "write: %f objects per second\n", 1000.00 / per );
	}
}


