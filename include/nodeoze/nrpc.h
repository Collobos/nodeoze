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

#ifndef _nodeoze_rpc_h
#define _nodeoze_rpc_h

#include <nodeoze/nconnection.h>
#include <nodeoze/nnotification.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nsingleton.h>
#include <nodeoze/nhttp.h>
#include <nodeoze/nws.h>
#include <nodeoze/npromise.h>
#include <nodeoze/nany.h>
#include <vector>
#include <atomic>

namespace nodeoze {

namespace rpc {

enum class errc : std::int32_t
{
	ok					= 0,
	invalid_request		= -32600,
	method_not_found	= -32601,
	invalid_params		= -32602,
	internal_error		= -32603,
	parse_error			= -32700
};

const std::error_category&
error_category();

inline std::error_code
make_error_code( const nodeoze::rpc::errc &val )
{
	return std::error_code( static_cast< int >( val ), nodeoze::rpc::error_category() );
}

inline bool
is_system_defined_error( std::int32_t val )
{
	return ( ( static_cast< errc >( val ) == errc::parse_error ) || ( ( static_cast< errc >( val )  >= errc::internal_error ) && ( static_cast< errc >( val ) <= errc::invalid_request ) ) );
}

inline bool
is_user_defined_error( std::int32_t val )
{
	return ( ( val >= -32099 ) && ( val <= -32000 ) );
}

any
make_error( std::error_code error );

any
make_error( const any &message, std::error_code err );
	
class connection : public nodeoze::connection
{
public:

	typedef std::shared_ptr< connection > ptr;
	
	connection( const uri &resource );

	connection( ip::tcp::socket sock );

	connection( http::connection::ptr wrapped );
	
	virtual ~connection();

	nodeoze::promise< void >
	send_notification( const std::string &method, const any::array_type &params );

	nodeoze::promise< nodeoze::any >
	send_request( const std::string &method, const any::array_type &params );
	
	virtual void
	close();
	
protected:

	virtual nodeoze::buffer
	deflate( const nodeoze::any &message ) = 0;
};

class manager
{
	NODEOZE_DECLARE_SINGLETON( manager )
	
public:

	typedef std::function< std::error_code ( any &params ) >			server_preflight_f;
	typedef std::function< void ( any &result, bool close ) >			server_reply_f;
	typedef std::function< void ( any &params ) >						server_notification_f;
	typedef std::function< void ( any &params, server_reply_f reply ) >	server_request_f;

	manager();

	~manager();

	void
	preflight( server_preflight_f func );
	
	void
	bind_notification( const std::string &method, std::size_t num_params, server_notification_f func );

	void
	unbind_notification( const std::string &method );
	
	void
	bind_request( const std::string &method, std::size_t num_params, server_request_f func );

	void
	unbind_request( const std::string &method );
	
	template < class T >
	inline void
	send_notification( const std::string &method, const any::array_type &args, T func )
	{
		any root;
	
		root[ "jsonrpc" ]	= "2.0";
		root[ "method" ]	= method;
		root[ "params" ]	= args;

		func( root );
	}
	
	template < class T >
	inline void
	send_request( const std::string &method, const any::array_type &args, std::uint64_t owner, promise< any > promise, T func )
	{
		any root;
		auto		id = ++m_id;
	
		root[ "jsonrpc" ]	= "2.0";
		root[ "method" ]	= method;
		root[ "params" ]	= args;
		root[ "id" ]		= id;
	
		m_reply_handlers[ id ] = std::make_pair( owner, promise );
		
		if ( marker::rpc_performance )
		{
			m_request_start_times[ id ] = std::chrono::system_clock::now();
			mlog( marker::rpc_performance, log::level_t::info, "client(%) %: start", id, method );
		}
		
		mlog( marker::rpc, log::level_t::info, "--> % (%)", root, id );
		
		func( root );
	}
	
	template < class T >
	inline void
	dispatch( any &message, T func )
	{
		any error;
		
		mlog( marker::rpc, log::level_t::info, "%", message );
		
		if ( validate( message, error ) )
		{
			if ( message.is_member( "method" ) )
			{
				auto method = message[ "method" ].to_string();
			
				if ( message.is_member( "id" ) )
				{
					auto err 	= std::error_code();
					auto id		= message[ "id" ].to_uint32();
	
					mlog( marker::rpc, log::level_t::info, "<-- % (%)", message, id );
		
					if ( m_preflight_handler )
					{
						err = m_preflight_handler( message );
					}
					
					if ( !err )
					{
						auto it = m_request_handlers.find( method );
								
						if ( it != m_request_handlers.end() )
						{
							auto &params = message[ "params" ];
							
							if ( it->second.first == params.size() )
							{
								it->second.second( params, [=]( any &root, bool close )
								{
									root[ "jsonrpc" ]	= "2.0";
									root[ "id" ]		= id;
									
									if ( root.is_member( "error" ) )
									{
										mlog( marker::rpc, log::level_t::info, "--> % (%) : %", message[ "method" ], id, root[ "error" ][ "code" ] );
									}
									else
									{
										mlog( marker::rpc, log::level_t::info, "--> % (%) : ok", message[ "method" ], id );
									}
									
									func( root, close );
								} );
							}
							else
							{
								func( make_error( message, make_error_code( errc::invalid_params ) ), false );
							}
						}
						else
						{
							func( make_error( message, make_error_code( errc::method_not_found ) ), false );
						}
					}
					else
					{
						func( make_error( message, err ), false );
					}
				}
				else
				{
					auto it = m_notification_handlers.find( method );
				
					if ( it != m_notification_handlers.end() )
					{
						auto &params = message[ "params" ];
						
						if ( it->second.first == params.size() )
						{
							mlog( marker::rpc, log::level_t::info, "--> %", message[ "method" ] );
							it->second.second( params );
						}
						else
						{
							nlog( log::level_t::error, "bad params for notification '%'", method );
						}
					}
					else
					{
						nlog( log::level_t::warning, "no handler for notification '%'", method );
					}
				}
			}
			else if ( message.is_member( "id" ) )
			{
				auto id = message[ "id" ].to_uint64();
				
				if ( marker::rpc_performance )
				{
					auto sit = m_request_start_times.find( id );
					if ( sit != m_request_start_times.end() )
					{
						auto start = sit->second;
						auto finish = std::chrono::system_clock::now();
						auto span = finish - start;
						std::chrono::milliseconds response_millis = std::chrono::duration_cast<std::chrono::milliseconds>( span );
						mlog( marker::rpc_performance, log::level_t::info, "client (%): response in % milliseconds", id, response_millis.count() );
					}
				}
				
				auto it = m_reply_handlers.find( id );
						
				if ( it != m_reply_handlers.end() )
				{
					std::error_code	err;
			
					if ( !message[ "error" ].is_null() )
					{
						auto code = message[ "error" ][ "code" ].to_int32();

						if ( is_system_defined_error( code ) )
						{
							err = make_error_code( static_cast< errc >( message[ "error" ][ "code" ].to_int32() ) );
						}
						else if ( is_user_defined_error( code ) && m_user_defined_error_category )
						{
							err = std::error_code( message[ "error" ][ "code" ].to_int32(), *m_user_defined_error_category );
						}
						else
						{
							err = make_error_code( errc::internal_error );
						}
					}
					
					mlog( marker::rpc, log::level_t::info, "<-- (%) : % (%)", id, err.value(), err.message() );
					
					auto promise = it->second.second;
					
					m_reply_handlers.erase( it );
					
					if ( !err )
					{
						any result = message[ "result" ];
						
						promise.resolve( std::move( result ) );
					}
					else
					{
						promise.reject( err, reject_context );
					}
				}
				else
				{
					mlog( marker::rpc, log::level_t::error, "unable to find handler for id %", id );
				}
			}
		}
		else
		{
			mlog( marker::rpc, log::level_t::error, "validation failed: %", message );
			func( error, false );
		}
	}
	
	void
	terminate_requests( oid_t oid );
	
	inline void
	set_user_defined_error_category( std::error_category *category )
	{
		m_user_defined_error_category = category;
	}

private:

	typedef std::pair< std::size_t, server_notification_f >								notification_target;
	typedef std::map< std::string, notification_target >								notification_handlers;
	typedef std::pair< std::size_t, server_request_f >									request_target;
	typedef std::map< std::string, request_target >										request_handlers;
	typedef std::map< std::uint64_t, std::pair< std::uint64_t, promise< any > > >		reply_handlers;
	
	typedef std::unordered_map< std::uint64_t, std::chrono::system_clock::time_point >	request_start_times;
	
	bool
	validate( const any &message, any &error );
	
	std::error_category								*m_user_defined_error_category	= nullptr;
	server_preflight_f								m_preflight_handler;
	notification_handlers							m_notification_handlers;
	request_handlers								m_request_handlers;
	reply_handlers									m_reply_handlers;
	any												m_root;
	std::int32_t									m_id = 0;
	request_start_times								m_request_start_times;
};

}

}

namespace std
{
	template<>
	struct is_error_code_enum< nodeoze::rpc::errc > : public std::true_type {};
}

#endif
