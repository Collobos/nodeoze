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

#include <nodeoze/json.h>
#include <nodeoze/tls.h>
#include <nodeoze/ws.h>
#include <nodeoze/macros.h>
#include <nodeoze/test.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <yajl_parse.h>

using namespace nodeoze;

namespace nodeoze {

namespace json {

struct parser_context
{
	parser_context( any &root )
	:
		m_handle( nullptr ),
		m_root( root )
	{
		reset();
	}
	
	~parser_context()
	{
		if ( m_handle )
		{
			yajl_free( m_handle );
			m_handle = nullptr;
		}
	}
	
	inline void
	on_root_object( json::parser::object_f func )
	{
		m_on_root_object = std::move( func );
	}
	
	inline std::error_code
	process( const char *buf, std::size_t len )
	{
		auto err = std::error_code();
		
		auto ret = yajl_parse( m_handle, reinterpret_cast< const unsigned char* >( buf ), len );

		if ( ret != yajl_status_ok )
		{
#if defined( DEBUG )

			char * error = ( char* ) yajl_get_error( m_handle, 1, reinterpret_cast< const unsigned char* >( buf ), len );
			fprintf( stderr, "error: %", error );

#endif
			err = make_error_code( std::errc::illegal_byte_sequence );
			reset();
		}
	
		return err;
	}
	
	inline bool
	add( any &val )
	{
		if ( m_val_stack.size() > 0 )
		{
			if ( m_val_stack.top().type() == any::type_t::array )
			{
				m_val_stack.top().emplace_back( std::move( val ) );
			}
			else if ( m_val_stack.top().type() == any::type_t::object )
			{
				assert( m_key_stack.size() > 0 );
				m_val_stack.top().emplace( m_key_stack.top(), std::move( val ) );
				m_key_stack.pop();
			}
			else
			{
			}
		}
		
		return true;
	}
	
	inline bool
	pop()
	{
		assert( m_val_stack.size() > 0 );

		if ( m_val_stack.size() > 1 )
		{
			auto any = std::move( m_val_stack.top() );
			m_val_stack.pop();

			add( any );
		}
		else
		{
			m_root = std::move( m_val_stack.top() );
			m_val_stack.pop();

			if ( m_on_root_object )
			{
				m_on_root_object( m_root );
			}
		}
		
		return true;
	}
	
	static int
	on_null( void *ctx )
	{
		auto self = static_cast< parser_context* >( ctx );
		
		auto child = any();

		self->add( child );
	
		return 1;
	}
	
	static int
	on_bool( void *ctx, int val )
	{
		auto self = static_cast< parser_context* >( ctx );

		auto child = any( val ? true : false );
		self->add( child );
	
		return 1;
	}

	static int
	on_integer( void *ctx, long long val )
	{
		auto self = static_cast< parser_context* >( ctx );
		
		auto child = any( static_cast< std::int64_t >( val ) );
		
		self->add( child );
		
		return 1;
	}
	
	static int
	on_double( void *ctx, double val )
	{
		auto self = static_cast< parser_context* >( ctx );
		
		auto child = any( val );

		self->add( child );
		
		return 1;
	}

	static int
	on_string( void *ctx, const unsigned char *val, size_t len )
	{
		auto self = static_cast< parser_context* >( ctx );

		auto child = any( any::string_type( val, val + len ) );
		self->add( child );
		
		return 1;
	}

	static int
	on_start_array( void *ctx )
	{
		auto self = static_cast< parser_context* >( ctx );

		self->m_val_stack.emplace( any::type_t::array );
		
		return 1;
	}

	static int
	on_end_array( void *ctx )
	{
		auto self = static_cast< parser_context* >( ctx );

		self->pop();

		return 1;
	}

	static int
	on_start_map( void *ctx )
	{
		auto self = static_cast< parser_context* >( ctx );

		self->m_val_stack.emplace( any::type_t::object );
		
		return 1;
	}

	static int
	on_map_key( void *ctx, const unsigned char *val, size_t len )
	{
		auto self = static_cast< parser_context* >( ctx );

		self->m_key_stack.emplace( val, val + len );

		return 1;
	}

	static int
	on_end_map( void *ctx )
	{
		auto self = static_cast< parser_context* >( ctx );

		self->pop();

		return 1;
	}
	
	void
	reset()
	{
		static yajl_callbacks	callbacks;
		static bool				first = true;
	
		if ( first )
		{
			callbacks.yajl_null			= on_null;
			callbacks.yajl_boolean		= on_bool;
			callbacks.yajl_integer		= on_integer;
			callbacks.yajl_double		= on_double;
			callbacks.yajl_number		= nullptr;
			callbacks.yajl_string		= on_string;
			callbacks.yajl_start_map	= on_start_map;
			callbacks.yajl_map_key		= on_map_key;
			callbacks.yajl_end_map		= on_end_map;
			callbacks.yajl_start_array	= on_start_array;
			callbacks.yajl_end_array	= on_end_array;
		}
		
		first = false;
	
		if ( m_handle )
		{
			yajl_free( m_handle );
		}
	
		m_handle = yajl_alloc( &callbacks, nullptr, this );
		yajl_config( m_handle, yajl_allow_multiple_values, 1 );
	}
	
	json::parser::object_f		m_on_root_object;
	std::stack< std::string >	m_key_stack;
	std::stack< any >			m_val_stack;
	yajl_handle					m_handle;
	nodeoze::any				&m_root;
};

}

}

template< class T >
static void
deflate( T &writer, const any &root )
{
	switch ( root.type() )
	{
		case any::type_t::null:
		{
			writer.Null();
		}
		break;
		
		case any::type_t::boolean:
		{
			writer.Bool( root.to_bool() );
		}
		break;
		
		case any::type_t::integer:
		{
			writer.Int64( root.to_int64() );
		}
		break;

		case any::type_t::floating:
		{
			writer.Double( root.to_floating() );
		}
		break;
		
		case any::type_t::string:
		case any::type_t::blob:
		{
			auto tmp = root.to_string();
			writer.String( tmp.data(), rapidjson::SizeType( tmp.size() ) );
		}
		break;
		
		case any::type_t::array:
		{
			writer.StartArray();
			
			root.visit( [&]( const std::string &key, const any &val ) mutable
			{
				nunused( key );
				deflate( writer, val );
			} );
			
			writer.EndArray();
		}
		break;

		case any::type_t::object:
		{
			writer.StartObject();
			
			root.visit( [&]( const std::string &key, const any &val ) mutable
			{
				writer.Key( key.data(), rapidjson::SizeType( key.size() ) );
				deflate( writer, val );
			} );
			
			writer.EndObject();
		}
		break;
		
		default:
		{
		}
		break;
	}
}


any
json::inflate( const std::string &s )
{
	any root;
	
	inflate( root, s );
	
	return root;
}


bool
json::inflate( any &root, const std::string &s )
{
	json::parser	p( root );
	auto			err = p.process( reinterpret_cast< const unsigned char* >( s.c_str() ), s.size() );
	bool			ok	= true;
	
	if ( err )
	{
		root = any::null();
		ok = false;
	}
	
	return ok;
}


std::string
json::deflate_to_string( const any &root, bool pretty )
{
	std::string ret;
	
	switch ( root.type() )
	{
		case any::type_t::array:
		case any::type_t::object:
		{
			rapidjson::GenericStringBuffer< rapidjson::UTF8<> > buffer;
			
			if ( pretty )
			{
				rapidjson::PrettyWriter< rapidjson::GenericStringBuffer< rapidjson::UTF8<> > > writer( buffer );
				::deflate( writer, root );
			}
			else
			{
				rapidjson::Writer< rapidjson::GenericStringBuffer< rapidjson::UTF8<> > > writer( buffer );
				::deflate( writer, root );
			}
			
			buffer.Flush();
			
			ret = std::string( buffer.GetString(), buffer.GetSize() );
		}
		break;

		default:
		{
			ret = root.to_string();
		}
		break;
	}
	
	return ret;
}


void
json::link()
{
}

#if defined( __APPLE__ )
#	pragma mark json::parser implementation
#endif

json::parser::parser( any &root )
:
	m_context( std::make_unique< parser_context >( root ) ),
	m_root( root )
{
}


json::parser::~parser()
{
}


void
json::parser::on_root_object( object_f func )
{
	m_context->on_root_object( func );
}


std::error_code
json::parser::process( const std::uint8_t *buf, std::size_t len )
{
	auto err = m_context->process( reinterpret_cast< const char* >( buf ), len );
	
	if ( err )
	{
		reset();
	}
	
	return err;
}


any&
json::parser::root()
{
	return m_root;
}


const any&
json::parser::root() const
{
	return m_root;
}


void
json::parser::reset()
{
	m_context->reset();
}


#if defined( __APPLE__ )
#	pragma mark json::rpc::connection implementation
#endif

#if 0
static connection::factory factory( { "jsonrpc+ws", "jsonrpc+wss" }, []( const uri &resource )
{
	return ( resource.scheme() == "jsonrpc+wss" ) ? true : false;
},

[]( const uri &resource )
{
	static std::unordered_map< std::uint64_t, json::rpc::connection::ptr >	*instances = nullptr;
	std::shared_ptr< json::rpc::connection >								connection;
	
	mlog( marker::json, log::level_t::info, "resource: %", resource.to_string() );
	
	if ( instances == nullptr )
	{
		instances = new std::unordered_map< std::uint64_t, json::rpc::connection::ptr >();
	}
	
	bool secure = ( resource.scheme() == "jsonrpc+wss" );
		
	mlog( marker::json, log::level_t::info, "secure: %", secure );
	
	connection = std::make_shared< json::rpc::connection >( resource );
	auto oid = make_oid( connection );
	
	if ( !proxy::manager::shared().bypass( resource ) )
	{
		// push( proxy::get()->create_adapter( secure ) );
	}
		
	if ( secure )
	{
		mlog( marker::json, log::level_t::info, "pushing tls filter" );
		connection->push( tls::filter::create( role_t::client ) );
	}
		
	if ( resource.scheme().find( "+ws" ) != std::string::npos )
	{
		mlog( marker::json, log::level_t::info, "pushing ws filter" );
		connection->push( ws::filter::create_client( resource ) );
	}
		
	notification::shared().subscribe( notification::local, make_oid( connection ), connection::close_event, [=]( const any &info )
	{
		nunused( info );
	
		mlog( marker::json, log::level_t::info, "close event" );
			
		auto it = instances->find( oid );
			
		if ( it != instances->end() )
		{
			instances->erase( it );
		}
		else
		{
			mlog( marker::json, log::level_t::warning, "unable to lookup jsonrpc connection for resource '%'", resource.to_string() );
		}
			
		return false;
	} );
		
	mlog( marker::json, log::level_t::info, "storing connection % in map", resource.to_string() );
	
	( *instances )[ oid ] = connection;
	
	return connection;
},
[]( const uri &resource, ip::tcp::socket socket  )
{
	static std::unordered_map< std::uint64_t, json::rpc::connection::ptr >	*instances = nullptr;
	std::shared_ptr< json::rpc::connection >								connection;
	
	mlog( marker::json, log::level_t::info, "resource: %", resource.to_string() );
	
	if ( instances == nullptr )
	{
		instances = new std::unordered_map< std::uint64_t, json::rpc::connection::ptr >();
	}
	
	bool secure = ( resource.scheme() == "jsonrpc+wss" );
		
	mlog( marker::json, log::level_t::info, "secure: %", secure );
	
	connection = std::make_shared< json::rpc::connection >( std::move( socket ) );
	auto oid = make_oid( connection );
	
	if ( !proxy::manager::shared().bypass( resource ) )
	{
		// push( proxy::get()->create_adapter( secure ) );
	}
	
		/*
	if ( secure )
	{
		mlog( marker::json, log::level_t::info, "pushing tls filter" );
		connection->push( tls::filter::create( role_t::client ) );
	}
		
	if ( resource.scheme().find( "+ws" ) != std::string::npos )
	{
		mlog( marker::json, log::level_t::info, "pushing ws filter" );
		connection->push( ws::filter::create_client( resource ) );
	}
	*/
		
	notification::shared().subscribe( notification::local, make_oid( connection ), connection::close_event, [=]( const any &info )
	{
		nunused( info );
	
		mlog( marker::json, log::level_t::info, "close event" );
			
		auto it = instances->find( oid );
			
		if ( it != instances->end() )
		{
			instances->erase( it );
		}
		else
		{
			mlog( marker::json, log::level_t::warning, "unable to lookup jsonrpc connection for resource '%'", resource.to_string() );
		}
			
		return false;
	} );
		
	mlog( marker::json, log::level_t::info, "storing connection % in map", resource.to_string() );
	
	( *instances )[ oid ] = connection;
	
	return connection;
} );


json::rpc::connection::connection( const uri &resource )
:
	nodeoze::rpc::connection( resource ),
	m_parser( m_root )
{
	mlog( marker::json, log::level_t::info, "this = %", this );
}


json::rpc::connection::connection( ip::tcp::socket sock )
:
	nodeoze::rpc::connection( std::move( sock ) ),
	m_parser( m_root )
{
	mlog( marker::json, log::level_t::info, "this = %", this );
}


json::rpc::connection::connection( http::connection::ptr wrapped )
:
	nodeoze::rpc::connection( wrapped ),
	m_parser( m_root )
{
	mlog( marker::json, log::level_t::info, "this = %", this );
}


json::rpc::connection::~connection()
{
	mlog( marker::json, log::level_t::info, "this = %", this );
}


nodeoze::buffer
json::rpc::connection::deflate( const nodeoze::any &message )
{
	return json::deflate_to_string( message );
}


std::error_code
json::rpc::connection::process( const buffer &buf )
{
	auto err = m_parser.process( buf.data(), buf.size() );
	ncheck_error( !err, exit, "unable to process JSON encoded rpc message" );
	
	nodeoze::rpc::manager::shared().dispatch( m_root, [=]( const any &message, bool close )
	{
		nodeoze::connection::send( json::deflate_to_string( message ) )
		.then( [=]() mutable
		{
			if ( close )
			{
				nodeoze::connection::close();
			}
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			
			if ( close )
			{
				nodeoze::connection::close();
			}
		} );
#if promise_has_finally
#endif
	} );
	
exit:
	
	return err;
}

#endif