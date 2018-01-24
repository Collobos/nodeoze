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

	typedef std::function< nodeoze::err_t ( any &params ) >				server_preflight_f;
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
					auto status = err_t::ok;
					auto id		= message[ "id" ].to_uint32();
	
					mlog( marker::rpc, log::level_t::info, "<-- % (%)", message, id );
		
					if ( m_preflight_handler )
					{
						status = m_preflight_handler( message );
					}
					
					if ( status == nodeoze::err_t::ok )
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
										mlog( marker::rpc, log::level_t::info, "--> % (%) : %", message[ "method" ], id, root[ "error" ][ "code" ].to_err() );
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
								func( make_error( message, err_t::invalid_params ), false );
							}
						}
						else
						{
							func( make_error( message, err_t::method_not_found ), false );
						}
					}
					else
					{
						func( make_error( message, status ), false );
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
					nodeoze::err_t		error_code = nodeoze::err_t::ok;
					std::string			error_message;
			
					if ( !message[ "error" ].is_null() )
					{
						error_code		= message[ "error" ][ "code" ].to_err();
						error_message	= message[ "error" ][ "message" ].to_string();
					}
					
					mlog( marker::rpc, log::level_t::info, "<-- (%) : %", id, error_code );
					
					auto promise = it->second.second;
					
					m_reply_handlers.erase( it );
					
					if ( error_code == err_t::ok )
					{
						any result = message[ "result" ];
						
						promise.resolve( std::move( result ) );
					}
					else
					{
						promise.reject( make_error_code( error_code ), reject_context );
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
	
	static any
	make_error( std::error_code error );

	static any
	make_error( err_t err );
	
private:

	typedef std::pair< std::size_t, server_notification_f >								notification_target;
	typedef std::map< std::string, notification_target >								notification_handlers;
	typedef std::pair< std::size_t, server_request_f >									request_target;
	typedef std::map< std::string, request_target >										request_handlers;
	typedef std::map< std::uint64_t, std::pair< std::uint64_t, promise< any > > >		reply_handlers;
	
	typedef std::unordered_map< std::uint64_t, std::chrono::system_clock::time_point >	request_start_times;
	
	static any
	make_error( const any &message, err_t err );
	
	bool
	validate( const any &message, any &error );
	
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

#endif
