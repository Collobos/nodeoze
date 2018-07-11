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
 
#include <nodeoze/msgpack.h>
#include <nodeoze/json.h>
#include <nodeoze/tls.h>
#include <nodeoze/ws.h>
#include <nodeoze/macros.h>
#include <nodeoze/test.h>
#include <msgpack.hpp>

using namespace nodeoze;

static std::error_code
deflate( const any &root, ::msgpack::packer< nodeoze::bufwriter > &packer );

static std::error_code
inflate( any &root, ::msgpack::object &obj );

nodeoze::buffer
nodeoze::mpack::deflate( const any &root )
{
	nodeoze::bufwriter buf{ 16 * 1024 };
	
	msgpack::packer< nodeoze::bufwriter > packer( buf );
	
	::deflate( root, packer );
	
	return buf.get_buffer();
}


any
nodeoze::mpack::inflate( const std::string &s )
{
	any root;
	
	inflate( root, s );
	
	return root;
}


std::error_code
nodeoze::mpack::inflate( any &root, const std::string &s )
{
	mpack::parser p( root );
	
	return p.process( reinterpret_cast< const unsigned char* >( s.c_str() ), s.size() );
}


any
nodeoze::mpack::inflate( const buffer& buf )
{
	any root;

	if ( inflate( root, buf ) )
	{
		return root;
	}
	else
	{
		return any::null();
	}
}


std::error_code
nodeoze::mpack::inflate( any& root, const buffer& buf )
{
	if ( buf.size() < 1 )
	{
		return std::error_code();
	}
	else
	{
		mpack::parser p( root );
	
		return p.process( buf.const_data(), buf.size() );
	}
}



void
nodeoze::mpack::link()
{
}


nodeoze::mpack::parser::parser( any &root )
:
	m_root( root )
{
}


nodeoze::mpack::parser::~parser()
{
}


std::error_code
nodeoze::mpack::parser::process( const std::uint8_t *buf, std::size_t len )
{
	msgpack::unpacked result;
	msgpack::unpack(result, reinterpret_cast< const char* >( buf ), len );
	msgpack::object obj = result.get();
	
	return ::inflate( m_root, obj );
}


static std::error_code
deflate( const any &root, ::msgpack::packer< nodeoze::bufwriter > &packer )
{
	auto err = std::error_code();
	
	switch ( root.type() )
	{
		case any::type_t::null:
		{
			packer.pack_nil();
		}
		break;
			
		case any::type_t::boolean:
		{
			packer.pack( root.to_bool() );
		}
		break;
			
		case any::type_t::integer:
		{
			packer.pack( root.to_int64() );
		}
		break;

		case any::type_t::floating:
		{
			packer.pack( root.to_floating() );
		}
		break;
		
		case any::type_t::string:
		{
			packer.pack( std::string{ root.to_string() } );
		}
		break;
		
		case any::type_t::blob:
		{
			auto buf = root.to_blob();
			packer.pack_bin( static_cast< std::uint32_t >( root.size() ) ).pack_bin_body( reinterpret_cast< const char* >( buf.const_data() ), static_cast< std::uint32_t >( buf.size() ) );
		}
		break;

		case any::type_t::array:
		{
			packer.pack_array( static_cast< std::uint32_t >( root.size() ) );
			
			root.visit( [&]( const std::string &key, const any &val ) mutable
			{
				nunused( key );
	
				ncheck_error( !err, exit );
				err = ::deflate( val, packer );
				ncheck_error( !err, exit );
				
			exit:
			
				return;
			} );
			
			ncheck_error( !err, exit );
		}
		break;
		
		case any::type_t::object:
		{
			packer.pack_map( static_cast< std::uint32_t >( root.size() ) );
			
			root.visit( [&]( const std::string &key, const any &val ) mutable
			{
				ncheck_error( !err, exit );
				packer.pack( key );
				err = ::deflate( val, packer );
				ncheck_error( !err, exit );
				
			exit:
			
				return;
			} );
			
			ncheck_error( !err, exit );
		}
		break;
			
		default:
		{
			err = make_error_code( std::errc::function_not_supported );
			ncheck_error( !err, exit );
		}
		break;
	}
	
exit:
		
	return err;
}


static std::error_code
inflate( any &root, ::msgpack::object &obj )
{
	auto err = std::error_code();
	
	switch ( obj.type )
	{
		case ::msgpack::type::object_type::NIL:
		{
			root = any::null();
		}
		break;
			
		case ::msgpack::type::object_type::BOOLEAN:
		{
			root = obj.via.boolean;
		}
		break;
			
		case ::msgpack::type::object_type::POSITIVE_INTEGER:
		{
			root = obj.via.u64;
		}
		break;
			
		case ::msgpack::type::object_type::NEGATIVE_INTEGER:
		{
			root = obj.via.i64;
		}
		break;

		case ::msgpack::type::object_type::FLOAT32:
		case ::msgpack::type::object_type::FLOAT64:
		{
			root = obj.via.f64;
		}
		break;

		/*
		case ::msgpack::type::object_type::FLOAT32:
		{
			root = obj.via.f64;
		}
		break;

		case ::msgpack::type::object_type::FLOAT64:
		{
			root = obj.via.f64;
		}
		break;
		*/
			
		case ::msgpack::type::object_type::STR:
		{
			root = std::string( obj.via.str.ptr, obj.via.str.size );
		}
		break;
		
		case ::msgpack::type::object_type::BIN:
		{
			root = nodeoze::buffer( obj.via.bin.ptr, obj.via.bin.size );
		}
		break;
			
		case ::msgpack::type::object_type::ARRAY:
		{
			root = any::array();
			
			for ( auto i = 0u; i < obj.via.array.size; ++i )
			{
				any element;
				err = inflate( element, obj.via.array.ptr[i] );
				ncheck_error( !err, exit );
				root.emplace_back( std::move( element ) );
			}
		}
		break;
			
		case ::msgpack::type::object_type::MAP:
		{
			root = any::object();
			
			for ( auto i = 0u; i < obj.via.map.size; ++i )
			{
				any val;
				ncheck_error_action( obj.via.map.ptr[i].key.type == ::msgpack::type::object_type::STR, err = make_error_code( std::errc::illegal_byte_sequence ), exit );
				err = inflate( val, obj.via.map.ptr[i].val );
				ncheck_error( !err, exit );
				root.emplace( std::string( obj.via.map.ptr[i].key.via.str.ptr, obj.via.map.ptr[i].key.via.str.size ), std::move( val ) );
			}
		}
		break;
		
		default:
		{
		}
		break;
	}
	
exit:
	
	return err;
}

#if defined( __APPLE__ )
#	pragma mark msgpack::rpc::connection implementation
#endif

#if 0

static connection::factory factory( { "msgpackrpc+ws", "msgpackrpc+wss" }, []( const uri &resource )
{
	return ( resource.scheme() == "msgpackrpc+wss" ) ? true : false;
},

[]( const uri &resource )
{
	static std::unordered_map< std::uint64_t, nodeoze::mpack::rpc::connection::ptr >	*instances = nullptr;
	std::shared_ptr< nodeoze::mpack::rpc::connection >									connection;
	
	mlog( marker::msgpack, log::level_t::info, "resource: %", resource.to_string() );
	
	if ( instances == nullptr )
	{
		instances = new std::unordered_map< std::uint64_t, nodeoze::mpack::rpc::connection::ptr >();
	}
	
	bool secure = ( resource.scheme() == "msgpackrpc+wss" );
		
	mlog( marker::rpc, log::level_t::info, "secure: %", secure );
	
	connection = std::make_shared< nodeoze::mpack::rpc::connection >( resource );
	auto oid = make_oid( connection );
	
	if ( !proxy::manager::shared().bypass( resource ) )
	{
		// push( proxy::get()->create_adapter( secure ) );
	}
		
	if ( secure )
	{
		mlog( marker::rpc, log::level_t::info, "pushing tls filter" );
		connection->push( tls::filter::create( role_t::client ) );
	}
		
	if ( resource.scheme().find( "+ws" ) != std::string::npos )
	{
		mlog( marker::rpc, log::level_t::info, "pushing ws filter" );
		connection->push( ws::filter::create_client( resource ) );
	}
		
	notification::shared().subscribe( notification::local, make_oid( connection ), connection::close_event, [=]( const any &info )
	{
		nunused( info );
	
		mlog( marker::msgpack, log::level_t::info, "close event" );
			
		auto it = instances->find( oid );
			
		if ( it != instances->end() )
		{
			instances->erase( it );
		}
		else
		{
			mlog( marker::msgpack, log::level_t::warning, "unable to lookup msgpack connection for resource '%'", resource.to_string() );
		}
			
		return false;
	} );
		
	mlog( marker::msgpack, log::level_t::info, "storing connection % in map", resource.to_string() );
	
	( *instances )[ oid ] = connection;
	
	return connection;
},
[]( const uri &resource, ip::tcp::socket socket  )
{
	static std::unordered_map< std::uint64_t, nodeoze::mpack::rpc::connection::ptr >	*instances = nullptr;
	std::shared_ptr< nodeoze::mpack::rpc::connection >									connection;
	
	mlog( marker::msgpack, log::level_t::info, "resource: %", resource.to_string() );
	
	if ( instances == nullptr )
	{
		instances = new std::unordered_map< std::uint64_t, nodeoze::mpack::rpc::connection::ptr >();
	}
	
	bool secure = ( resource.scheme() == "msgpackrpc+wss" );
		
	mlog( marker::msgpack, log::level_t::info, "secure: %", secure );
	
	connection = std::make_shared< nodeoze::mpack::rpc::connection >( std::move( socket ) );
	auto oid = make_oid( connection );
	
	if ( !proxy::manager::shared().bypass( resource ) )
	{
		// push( proxy::get()->create_adapter( secure ) );
	}
	
		/*
	if ( secure )
	{
		mlog( marker::msgpackrpc, log::level_t::info, "pushing tls filter" );
		connection->push( tls::filter::create( role_t::client ) );
	}
		
	if ( resource.scheme().find( "+ws" ) != std::string::npos )
	{
		mlog( marker::msgpackrpc, log::level_t::info, "pushing ws filter" );
		connection->push( ws::filter::create_client( resource ) );
	}
	*/
		
	notification::shared().subscribe( notification::local, make_oid( connection ), connection::close_event, [=]( const any &info )
	{
		nunused( info );
	
		mlog( marker::msgpack, log::level_t::info, "close event" );
			
		auto it = instances->find( oid );
			
		if ( it != instances->end() )
		{
			instances->erase( it );
		}
		else
		{
			mlog( marker::msgpack, log::level_t::warning, "unable to lookup msgpack connection for resource '%'", resource.to_string() );
		}
			
		return false;
	} );
		
	mlog( marker::msgpack, log::level_t::info, "storing connection % in map", resource.to_string() );
	
	( *instances )[ oid ] = connection;
	
	return connection;
} );


nodeoze::mpack::rpc::connection::connection( const uri &resource )
:
	nodeoze::rpc::connection( resource ),
	m_parser( m_root )
{
	mlog( marker::msgpack, log::level_t::info, "this = %", this );
}


nodeoze::mpack::rpc::connection::connection( ip::tcp::socket sock )
:
	nodeoze::rpc::connection( std::move( sock ) ),
	m_parser( m_root )
{
	mlog( marker::msgpack, log::level_t::info, "this = %", this );
}


nodeoze::mpack::rpc::connection::connection( http::connection::ptr wrapped )
:
	nodeoze::rpc::connection( wrapped ),
	m_parser( m_root )
{
	mlog( marker::msgpack, log::level_t::info, "this = %", this );
}


nodeoze::mpack::rpc::connection::~connection()
{
	mlog( marker::msgpack, log::level_t::info, "this = %", this );
}


nodeoze::buffer
nodeoze::mpack::rpc::connection::deflate( const nodeoze::any &message )
{
	return mpack::deflate( message );
}


std::error_code
nodeoze::mpack::rpc::connection::process( const buffer &buf )
{
	mlog( marker::msgpack, log::level_t::info, "received % bytes", buf.size() );

	auto err = m_parser.process( buf.const_data(), buf.size() );
	ncheck_error( !err, exit, "unable to process msgpack encoded rpc message" );

	mlog( marker::msgpack, log::level_t::info, "after parsing: %", m_root );
	
	nodeoze::rpc::manager::shared().dispatch( m_root, [=]( const any &message, bool close )
	{
		nodeoze::connection::send( mpack::deflate( message ) )
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