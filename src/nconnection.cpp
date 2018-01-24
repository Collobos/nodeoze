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
 
#include <nodeoze/nconnection.h>
#include <nodeoze/nhttp.h>
#include <nodeoze/nws.h>
#include <nodeoze/ntls.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nlog.h>
#include <nodeoze/nmachine.h>

using namespace nodeoze;

connection::factory::factories *connection::factory::m_factories	= nullptr;

connection::factory::factory( std::vector< std::string > schemes, secure_f secure, builder_f builder1, builder_with_socket_f builder2 )
{
	if ( m_factories == nullptr )
	{
		m_factories = new factories;
	}
	
	for ( auto it : schemes )
	{
		( *m_factories )[ it ] = std::make_tuple( secure, builder1, builder2 );
	}
};

bool
connection::factory::wants_security( const uri &resource )
{
	auto it = m_factories->find( resource.scheme() );
	return ( it != m_factories->end() ) ? std::get< 0 >( it->second )( resource ) : false;
}

		
connection::ptr
connection::factory::build( const uri &resource )
{
	connection::ptr ret;

	auto it = m_factories->find( resource.scheme() );
	ncheck_error( it != m_factories->end(), exit, "unable to build connection using scheme '%'", resource.scheme() );
	ret = std::get< 1 >( it->second )( resource );

exit:

	return ret;
}


connection::ptr
connection::factory::build( const uri &resource, ip::tcp::socket socket )
{
	auto it = m_factories->find( resource.scheme() );
	return ( it != m_factories->end() ) ? std::get< 2 >( it->second )( resource, std::move( socket ) ) : nullptr;
}


const std::string connection::close_event	= "connection_close_event";

std::unordered_map< std::string, connection* >&
instances()
{
	auto global = new std::unordered_map< std::string, connection* >();
	return *global;
}


connection::connection()
:
	m_connected( true ),
	m_connecting( false ),
	m_id( uuid().to_string() )
{
}


connection::connection( const uri &resource )
:
	m_connected( false ),
	m_connecting( false ),
	m_resource( resource ),
	m_id( uuid().to_string() )
{
	mlog( marker::connection, log::level_t::info, "this = %", this );
	instances()[ m_id ] = this;
}


connection::connection( ip::tcp::socket sock )
:
	m_connected( true ),
	m_connecting( false ),
	m_socket( std::move( sock ) ),
	m_id( uuid().to_string() )
{
	mlog( marker::connection, log::level_t::info, "this = %", this );
	instances()[ m_id ] = this;
	recv();
}


connection::connection( ptr wrapped )
:
	m_connected( true ),
	m_connecting( false ),
	m_wrapped( wrapped ),
	m_id( uuid().to_string() )
{
	mlog( marker::connection, log::level_t::info, "this = %", this );
	instances()[ m_id ] = this;
}


connection::connection( connection &&rhs )
:
	m_connected( rhs.m_connected ),
	m_socket( std::move( rhs.m_socket ) ),
	m_id( uuid().to_string() )
{
	mlog( marker::connection, log::level_t::info, "this = %", this );
	instances()[ m_id ] = this;
	recv();
}


connection::~connection()
{
	mlog( marker::connection, log::level_t::info, "this = %", this );
	close();
	
	auto it = instances().find( m_id );
	
	if ( it != instances().end() )
	{
		instances().erase( it );
	}
}


promise< void >
connection::connect()
{
	auto ret = promise< void >();
	
	if ( m_connected )
	{
		mlog( marker::connection, log::level_t::info, "already connected to %", destination().to_string() );
		ret.resolve();
	}
	else if ( !m_connecting )
	{
		auto dest = destination();
		
		mlog( marker::connection, log::level_t::info, "connect to destination %", dest.to_string() );
		
		if ( dest )
		{
			assert( m_id.size() > 0 );
			
			auto id = m_id;
			
			m_connecting = true;
			
			mlog( marker::connection, log::level_t::info, "resolving host % (%)", dest.host(), id );
			
			ip::address::resolve( dest.host() ).then( [=]( std::vector< ip::address > addresses ) mutable
			{
				auto found = find< connection >( id, [&]( connection &self ) mutable
				{
					nunused( self );

					std::deque< ip::address > dummy;
					
					for ( auto &address : addresses )
					{
						dummy.emplace_back( address );
					}
					
					handle_resolve( dest, dummy, ret );
				} );
				
				ncheck_error_action_quiet( found, ret.reject( make_error_code( std::errc::owner_dead ) ), exit );
				
			exit:
			
				return;
			},
			[=]( auto err ) mutable
			{
				ret.reject( err );
			} );
		}
		else
		{
			ret.reject( make_error_code( std::errc::invalid_argument ) );
		}
	}
	
	return ret;
}


ip::address
connection::subnet() const
{
	ip::address sub( ip::address::v4_any() );
	
	for ( auto &nif : machine::self().nifs() )
	{
		if ( nif )
		{
			if ( nif.address() == name().addr() )
			{
				sub = nif.address() & nif.netmask();
				break;
			}
		}
	}
	return sub;

}


void
connection::handle_resolve( const uri &resource, std::deque< ip::address > addrs, promise< void > ret )
{
	auto promises	= promise< ip::tcp::socket >::array_t();
	auto id			= m_id;
	
	assert( addrs.size() > 0 );
	
	for ( auto &addr : addrs )
	{
		auto endpoint	= ip::endpoint( addr, resource.port() );
		auto promise	= nodeoze::promise< ip::tcp::socket >();
		auto sock		= std::make_shared< ip::tcp::socket >();
		
		mlog( marker::connection, log::level_t::info, "connecting to %", endpoint );
		
		sock->connect( endpoint )
		.then( [=]() mutable
		{
			if ( !ret.is_finished() )
			{
				mlog( marker::connection, log::level_t::info, "connected to %", endpoint );
			}
			
			promise.resolve( std::move( *sock ) );
		},
		[=]( auto err ) mutable
		{
			if ( !ret.is_finished() )
			{
				mlog( marker::connection, log::level_t::info, "failed to connect to %", endpoint );
			}
	
			promise.reject( err );
		} );
		
		promises.emplace_back( promise );
	}
	
	m_connecting	= true;
	m_connected		= false;
	
	promise< ip::tcp::socket >::any( promises )
	.then( [=]( ip::tcp::socket sock ) mutable
	{
		mlog( marker::connection, log::level_t::info, "using connection to %", sock.peer() );
	
		if ( !find< connection >( id, [&]( connection &self ) mutable
		{
			nunused( self );
			
			m_connecting	= false;
			m_connected		= true;
			m_socket		= std::move( sock );

			for ( auto it = m_filters.begin(); it != m_filters.end(); it++ )
			{
				m_socket.push( it->release() );
			}
			
			ret.resolve();
			
			recv();
		} ) )
		{
			ret.reject( make_error_code( std::errc::owner_dead ) );
		}
	},
	[=]( auto err ) mutable
	{
		if ( !find< connection >( id, [&]( connection &self ) mutable
		{
			nunused( self );
			
			m_connecting	= false;
			m_connected		= false;
			
			ret.reject( err );
		} ) )
		{
			ret.reject( make_error_code( std::errc::owner_dead ) );
		}
	} );
}
		

promise< void >
connection::send( buffer buf )
{
	auto ret = promise< void >();

	mlog( marker::connection, log::level_t::info, "sending buffer of size: %", buf.size() );

	if ( m_wrapped ) 
	{
		ret = m_wrapped->send( std::move( buf ) );
	}
	else if ( m_connected )
	{
		ret = m_socket.send( std::move( buf ) );
	}
	else
	{
		mlog( marker::connection, log::level_t::info, "pushing buffer on send queue" );
		
		m_send_queue.push( std::make_pair( std::move( buf ), ret ) );
		
		if ( !m_connecting )
		{
			connect()
			.then( [=]() mutable
			{
				while ( !m_send_queue.empty() )
				{
					auto promise = m_send_queue.front().second;
					
					send( std::move( m_send_queue.front().first ) )
					.then( [=]() mutable
					{
						promise.resolve();
					},
					[=]( auto err ) mutable
					{
						promise.reject( err );
					} );
					
					m_send_queue.pop();
				}
			},
			[=]( auto err ) mutable
			{
				while ( !m_send_queue.empty() )
				{
					m_send_queue.front().second.reject( err );
					m_send_queue.pop();
				}
			} );
		}
	}
	
	return ret;
}


void
connection::peek( peek_reply_f reply )
{
	m_socket.peek( reply );
}


void
connection::recv()
{
	m_socket.recv( [=]( auto err, buffer &buf )
	{
		auto hold( shared_from_this() );
		
		mlog( marker::connection, log::level_t::info, "err % (%), buf size: %", err, err.message(), buf.size() );
		
		if ( !err )
		{
			if ( buf.size() > 0 )
			{
				err = process( buf );

				if ( err )
				{
					mlog( marker::connection, log::level_t::info, "closing connection (process() failed)" );
					close();
				}
			}
			else
			{
				mlog( marker::connection, log::level_t::info, "buf size is 0" );
			}
		}
		else
		{
			mlog( marker::connection, log::level_t::info, "closing connection % (%)", err, err.message() );
			close();
		}
	} );
}


void
connection::close()
{
	if ( m_connected )
	{
		mlog( marker::connection, log::level_t::info, "publishing close event" );
		notification::shared().publish( notification::local, 0, make_oid( this ), close_event );
		m_connected = false;
		m_socket.close();
	}
}


void
connection::unref()
{
}


nodeoze::uri
connection::destination() const
{
	return m_resource;
}

