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

#include <nodeoze/rpc.h>
#include <nodeoze/tls.h>
#include <nodeoze/macros.h>

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark rpc::connection implementation
#endif

#if 0

rpc::connection::connection( const uri &resource )
:
	nodeoze::connection( resource )
{
	mlog( marker::rpc, log::level_t::info, "this = %", this );
}


rpc::connection::connection( ip::tcp::socket sock )
:
	nodeoze::connection( std::move( sock ) )
{
	mlog( marker::rpc, log::level_t::info, "this = %", this );
}


rpc::connection::connection( http::connection::ptr wrapped )
:
	nodeoze::connection( wrapped )
{
}


rpc::connection::~connection()
{
	mlog( marker::rpc, log::level_t::info, "this = %", this );
	close();
}


promise< void >
rpc::connection::send_notification( const std::string &method, const any::array_type &params )
{
	auto ret = promise< void >();
	
	rpc::manager::shared().send_notification( method, params, [&]( any &message )
	{
		connection::send( deflate( message ) )
		.then( [=]() mutable
		{
			ret.resolve();
		},
		[=]( auto err ) mutable
		{
			ret.reject( err );
		} );
	} );
	
	return ret;
}


promise< any >
rpc::connection::send_request( const std::string &method, const any::array_type &params )
{
	auto ret = promise< any >();
	
	rpc::manager::shared().send_request( method, params, make_oid( this ), ret, [&]( any &message ) mutable
	{
		auto buf = deflate( message );

		mlog( marker::msgpack, log::level_t::info, "sending % bytes of msgpack data", buf.size() );

		connection::send( std::move( buf ) )
		.then( [=]() mutable
		{
			// we should give promise to manager right now instead of in send_request
		},
		[=]( auto err ) mutable
		{
			ret.reject( err );
		} );
	} );
	
	return ret;
}


void
rpc::connection::close()
{
	mlog( marker::rpc, log::level_t::info, "closing rpc connection" );

	rpc::manager::shared().terminate_requests( make_oid( this ) );
	nodeoze::connection::close();
}

#endif


#if defined( __APPLE__ )
#	pragma mark rpc::manager implementation
#endif

NODEOZE_DEFINE_SINGLETON( rpc::manager )

rpc::manager*
rpc::manager::create()
{
	return new rpc::manager;
}


rpc::manager::manager()
{
}


rpc::manager::~manager()
{
}


void
rpc::manager::preflight( server_preflight_f func )
{
	m_preflight_handler = func;
}


void
rpc::manager::bind_notification( const std::string &method, std::size_t num_params, server_notification_f func )
{
	m_notification_handlers[ method ] = std::make_pair( num_params, func );
}


void
rpc::manager::unbind_notification( const std::string &method )
{
	auto it = m_notification_handlers.find( method );

	if ( it != m_notification_handlers.end() )
	{
		m_notification_handlers.erase( it );
	}
}


void
rpc::manager::bind_request( const std::string &method, std::size_t num_params, server_request_f func )
{
	m_request_handlers[ method ] = std::make_pair( num_params, func );
}


void
rpc::manager::unbind_request( const std::string &method )
{
	auto it = m_request_handlers.find( method );

	if ( it != m_request_handlers.end() )
	{
		m_request_handlers.erase( it );
	}
}


bool
rpc::manager::validate( const any &message, any &error )
{
	any	err;
	bool		ok = true;
      
	if ( !message.is_object() || !message.is_member( "jsonrpc" ) || ( message[ "jsonrpc" ].to_string() != "2.0" ) )
	{
		err[ "code" ]		= errc::invalid_request;
		err[ "message" ]	= "Invalid JSON-RPC request.";
		
		error[ "id" ]		= any::null();
		error[ "jsonrpc" ]	= "2.0";
		error[ "error" ]	= err;
		
		ok = false;
	}
	else if ( !message.is_member( "method" ) && !message.is_member( "id" ) )
	{
        err[ "code" ]		= errc::invalid_request;
        err[ "message" ]	= "Invalid JSON-RPC request.";
		
		error[ "id" ]		= any::null();
		error["jsonrpc" ]	= "2.0";
		error[ "error" ]	= err;
		
		ok = false;
	}
	else if ( message.is_member( "id" ) && !message[ "id" ].is_integer() )
	{
        err[ "code" ]		= errc::invalid_request;
        err[ "message" ]	= "Invalid JSON-RPC request.";
		
		error[ "id" ]		= any::null();
		error[ "jsonrpc" ]	= "2.0";
		error[ "error" ]	= err;
		
		ok = false;
	}
	else if ( message.is_member( "method" ) && !message["method"].is_string() )
	{
        err[ "code" ]		= errc::invalid_request;
        err[ "message" ]	= "Invalid JSON-RPC request.";
		
		error[ "id" ]		= any::null();
		error["jsonrpc" ]	= "2.0";
		error[ "error" ]	= err;
		
		ok = false;
	}
	
	return ok;
}


void
rpc::manager::terminate_requests( std::uint64_t oid )
{
	auto it = m_reply_handlers.begin();
	
	while ( it != m_reply_handlers.end() )
	{
		if ( it->second.first == oid )
		{
			auto handler = it->second.second;
			
			it = m_reply_handlers.erase( it );
			
			handler.reject( make_error_code( std::errc::interrupted ) );
		}
		else
		{
			it++;
		}
	}
}


class rpc_error_category : public std::error_category
{
public:

	virtual const char*
	name() const noexcept override
    {
		return "rpc";
    }
	
    virtual std::string
	message( int value ) const override
    {
		std::ostringstream os;
		
		switch ( static_cast< rpc::errc >( value ) )
        {
			case rpc::errc::ok:
			{
				os << "ok";
			}
			break;
			
			case rpc::errc::invalid_request:
			{
				os << "invalid request";
			}
			break;
			
			case rpc::errc::method_not_found:
			{
				os << "method not found";
			}
			break;
			
			case rpc::errc::invalid_params:
			{
				os << "invalid params";
			}
			break;
			
			case rpc::errc::internal_error:
			{
				os << "internal error";
			}
			break;
			
			case rpc::errc::parse_error:
			{
				os << "parse error";
			}
			break;
		}
		
		return os.str();
    }
};

const std::error_category&
nodeoze::rpc::error_category()
{
	static auto instance = new rpc_error_category;
    return *instance;
}

any
rpc::make_error( std::error_code err )
{
	any reply;

	if ( err.category() == rpc::error_category() )
	{
		reply[ "error" ][ "code" ]		= err.value();
		reply[ "error" ][ "message" ]	= err.message();
	}
	else if ( is_user_defined_error( err.value() ) )
	{
		reply[ "error" ][ "code" ]		= err.value();
		reply[ "error" ][ "message" ]	= err.message();
	}
	else
	{
		reply[ "error" ][ "code" ]		= errc::internal_error;
		reply[ "error" ][ "message" ]	= "unknown";
	}

	return reply;
}


any
rpc::make_error( const any &in, std::error_code err )
{
	any out;
					
	out[ "error" ]		= make_error( err );
	out[ "jsonrpc" ]	= "2.0";
	out[ "id" ]			= in[ "id" ];
	
	return out;
}
