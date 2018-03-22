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

#include <nodeoze/nnotification.h>
#include <nodeoze/nscoped_operation.h>
#include <nodeoze/njson.h>
#include <nodeoze/nhttp.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/noid.h>

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark notification implementation
#endif

const std::string	notification::subscribe_event = "notification_subscribe_event";
const uri			notification::local( "notification://local" );

NODEOZE_DEFINE_SINGLETON( notification )

notification*
notification::create()
{
	return new notification;
}


notification::notification()
{
	http::server::shared().bind_request( http::method_t::get, "/notification", "*", [=]( http::message &request, http::server::response_f reply )
	{
		auto it = request.header().find( "Upgrade" );
	  
		if ( ( it != request.header().end() ) && string::case_insensitive_compare( it->second, "websocket" ) )
		{
			http::server::shared().active_connection()->upgrade_to_websocket( ws::filter::type_t::text, [=]( const buffer &buf, http::connection::websocket_reply_f reply )
			{
				static any			root;
				static json::parser parser( root );
				
				auto err = parser.process( buf.data(), buf.size() );
				ncheck_error( !err, exit, "unable to parse JSON encoded rpc message" );
				
				rpc::manager::shared().dispatch( root, [=]( const any &out, bool close )
				{
					reply( json::deflate_to_string( out ), close );
				} );
				
			exit:
			
				return;
			} );
		}
		else
		{
			http::message response( request, http::code_t::not_found );
			response.add_header_field( "Connection", "Close" );
			response.add_header_field( "Content-Length", 0 );
			reply( response, true );
		}
	  
		return 0;
	} );
	
	rpc::manager::shared().bind_request( "/notification/subscribe", 3, [=]( const any &params, rpc::manager::server_reply_f reply )
	{
		auto		remote_oid	= params[ 0 ].to_string();
		auto		source_oid	= params[ 1 ].to_uint64();
		auto		event		= params[ 2 ].to_string();
		any result;
		
		subscribe_proxy( http::server::shared().active_connection(), remote_oid, source_oid, event );
		
		reply( result, false );
	} );
	
	rpc::manager::shared().bind_request( "/notification/unsubscribe", 1, [=]( const any &params, rpc::manager::server_reply_f reply )
	{
		auto		remote_oid = params[ 0 ].to_string();
		any result;
		
		unsubscribe_proxy( remote_oid );
		
		reply( result, false );
	} );
	
	rpc::manager::shared().bind_notification( "/notification/publish", 4, [=]( const any &params )
	{
		auto subscription_oid	= params[ 0 ].to_uint64();
		auto source_oid			= params[ 1 ].to_uint64();
		auto event				= params[ 2 ].to_string();
		auto info				= params[ 3 ];
		
		publish_local( subscription_oid, source_oid, event, info );
	} );
	
	rpc::manager::shared().bind_notification( "/notification/notify", 3, [=]( const any &params )
	{
		auto subscription_oid	= params[ 0 ].to_string();
		auto event				= params[ 1 ].to_string();
		auto info				= params[ 2 ];
		
		notify( subscription_oid, event, info );
	} );
}


notification::~notification()
{
}


void
notification::subscribe( const uri &resource, std::uint64_t source_oid, const std::string &event, handler_f handler, subscribe_reply_f reply )
{
	ncheck_error_action( resource.scheme() == "notification", reply ? reply( scoped_operation() ) : ( void ) 0, exit, "bad resource: %", resource );
	
	if ( resource.host() == "local" )
	{
		subscribe_local( source_oid, event, handler, reply );
	}
	else
	{
		subscribe_remote( resource, source_oid, event, handler, reply );
	}
	
exit:
	
	return;
}


void
notification::publish( const uri &resource, std::uint64_t subscription_oid, std::uint64_t source_oid, const std::string &event, any info )
{
	ncheck_error_quiet( resource.scheme() == "notification", exit );
	
	if ( resource.host() == "local" )
	{
		// Don't dispatch if there are no subscribers

		if ( m_local_subscribers.find( event ) != m_local_subscribers.end() )
		{
			runloop::shared().dispatch( [=,info{ std::move( info ) }]() mutable
			{
				publish_local( subscription_oid, source_oid, event, info );
			} );
		}
	}
	else
	{
		publish_remote( resource, subscription_oid, source_oid, event, info );
	}
	
exit:

	return;
}


void
notification::subscribe_local( std::uint64_t source_oid, const std::string &event, handler_f handler, subscribe_reply_f reply )
{
	local_subscriber::ptr subscriber;
	
	auto it = m_local_subscribers.find( event );
		
	if ( it == m_local_subscribers.end() )
	{
		m_local_subscribers[ event ] = local_subscriber::list();
		it = m_local_subscribers.find( event );
	}
		
	subscriber = std::make_shared< local_subscriber >( source_oid, event, handler );
	
	it->second.push_back( subscriber );
	
	if ( reply != nullptr )
	{
		scoped_operation ret;
		
		ret = create_subscription( [=]( void *v ) mutable
		{
			nunused( v );
	
			subscriber->m_extant = false;
		} );
		
		reply( ret );
	}
		
	// Don't publish if the subscribed event was our own event
	
	if ( event != subscribe_event )
	{
		publish( local, 0, 1, subscribe_event, any( { { "subscription_oid", make_oid( subscriber ) }, { "source_oid", source_oid }, { "event", event } } ) );
	}
}


void
notification::subscribe_remote( const uri &resource, std::uint64_t source_oid, const std::string &event, handler_f handler, subscribe_reply_f reply )
{
	uri rewrite( resource );
		
	rewrite.set_scheme( "jsonrpc+wss" );
	rewrite.set_path( "/notification" );
		
	auto server		= connection::create< rpc::connection >( rewrite );
	auto subscriber	= std::make_shared< remote_subscriber >( rewrite, server, source_oid, event, handler );
		
	server->send_request( "/notification/subscribe", { subscriber->m_oid, source_oid, event } )
	.then( [=]( any result ) mutable
	{
		nunused( result );
		
		scoped_operation ret;
		
		m_remote_subscribers[ subscriber->m_oid ] = subscriber;
			
		ret = create_subscription( [=]( void *v ) mutable
		{
			nunused( v );
	
			server->send_request( "/notification/unsubscribe", { subscriber->m_oid } )
			.then( [=]( any result ) mutable
			{
				nunused( result );
			},
			[=]( std::error_code err ) mutable
			{
				nunused( err );
			} );
			
			auto it = m_remote_subscribers.find( subscriber->m_oid );
				
			if ( it != m_remote_subscribers.end() )
			{
				m_remote_subscribers.erase( it );
			}
		} );
		
		reply( ret );
	},
	[=]( std::error_code err ) mutable
	{
		nunused( err );
	} );
}


void
notification::subscribe_proxy( http::connection::ptr client, const std::string &remote_oid, std::uint64_t source_oid, const std::string &event )
{
	auto subscriber = std::make_shared< proxy_subscriber >( remote_oid );
	
	subscribe( local, source_oid, event, [=]( const any &info ) mutable
	{
		assert( client );
		
		rpc::manager::shared().send_notification( "/notification/notify", { subscriber->m_remote_oid, event, info }, [=]( const any &message )
		{
			client->send( json::deflate_to_string( message ) )
			.then( [=]() mutable
			{
			},
			[=]( std::error_code err ) mutable
			{
				nunused( err );
			} );
		} );
		
		return true;
	},
	
	[=]( scoped_operation s ) mutable
	{
		subscriber->m_local_subscription = s;
	} );
	
	subscribe( local, make_oid( client ), connection::close_event, [=]( const any &info ) mutable
	{
		nunused( info );

		unsubscribe_proxy( subscriber->m_remote_oid );
		return false;
	},
	
	[=]( scoped_operation s )
	{
		subscriber->m_close_subscription = s;
	} );
	
	m_proxy_subscribers[ remote_oid ] = subscriber;
}


void
notification::unsubscribe_proxy( const std::string &remote_oid )
{
	auto it = m_proxy_subscribers.find( remote_oid );
	
	ncheck_error_quiet( it != m_proxy_subscribers.end(), exit );
	
	m_proxy_subscribers.erase( it );
	
exit:

	return;
}


void
notification::publish_local( std::uint64_t subscription_oid, std::uint64_t source_oid, const std::string &event, const any &info )
{
	auto it1 = m_local_subscribers.find( event );
			
	if ( it1 != m_local_subscribers.end() )
	{
		if ( subscription_oid != 0 )
		{
			for ( auto it2 = it1->second.begin(); it2 != it1->second.end(); it2++ )
			{
				if ( make_oid( ( *it2 ) ) == subscription_oid )
				{
					bool extant = ( *it2 )->m_extant;

					if ( extant )
					{
						extant = ( *it2 )->m_handler( info );
					}
					
					if ( !extant )
					{
						it1->second.erase( it2 );
					}
					
					break;
				}
			}
		}
		else
		{
			auto it2 = it1->second.begin();
			
			while ( it2 != it1->second.end() )
			{
				bool extant = ( *it2 )->m_extant;
				
				if ( extant )
				{
					if ( ( ( *it2 )->m_source_oid == 0 ) || ( ( *it2 )->m_source_oid == source_oid ) )
					{
						if ( ( *it2 )->m_handler )
						{
							extant = ( *it2 )->m_handler( info );
						}
						else
						{
							extant = false;
						}
					}
				}
				
				if ( !extant )
				{
					it2 = it1->second.erase( it2 );
				}
				else
				{
					it2++;
				}
			}
		}
	}
}


void
notification::publish_remote( const uri &resource, std::uint64_t subscription_oid, std::uint64_t source_oid, const std::string &event, const any &info )
{
	uri rewrite( resource );
		
	rewrite.set_scheme( "jsonrpc+wss" );
	rewrite.set_path( "/notification" );
	
	auto server = connection::create< rpc::connection>( rewrite );
		
	server->send_notification( "/notification/publish", { subscription_oid, source_oid, event, info } );
}


void
notification::notify( const std::string &subscription_oid, const std::string &event, const any &info )
{
	nunused( event );
	
	auto it = m_remote_subscribers.find( subscription_oid );
	
	ncheck_error_quiet( it != m_remote_subscribers.end(), exit );
				
	it->second->m_handler( info );
	
exit:

	return;
}
