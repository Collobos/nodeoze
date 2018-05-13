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
#define NOMINMAX
 
#include <nodeoze/proxy.h>
#include <nodeoze/notification.h>
#include "net_utils.h"
#include <nodeoze/http.h>
#include <nodeoze/any.h>
#include <nodeoze/buffer.h>
#include <nodeoze/base64.h>
#include <nodeoze/log.h>
#include <nodeoze/markers.h>
#include <limits>
#include <sstream>
#include <vector>

using namespace nodeoze;

class http_proxy : public stream::filter
{
public:

	http_proxy( bool secure );

	virtual ~http_proxy();
	
	virtual const std::string&
	name() const
	{
		static std::string s( "http-proxy" );
		return s;
	}
	
	virtual stream::state_t
	state() const
	{
		return stream::state_t::handshaking;
	}
	
	virtual stream::state_t
	send( buffer &in_buf, buffer &out_buf );
		
	virtual stream::state_t
	recv( std::vector< buffer > &in_recv_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs );
	
protected:

	void
	send_connect();

	std::uint16_t
	parse_server_handshake();
	
	std::string	m_handshake;
	bool		m_secure;
};


class socks5_proxy : public stream::filter
{
public:

	socks5_proxy( const ip::endpoint &to );
	
	virtual ~socks5_proxy();
	
	virtual const std::string&
	name() const
	{
		static std::string s( "socks5-proxy" );
		return s;
	}
	
	virtual stream::state_t
	state() const
	{
		return m_state;
	}
	
	virtual stream::state_t
	send( buffer &in_buf, buffer &out_buf );
		
	virtual stream::state_t
	recv( std::vector< buffer > &in_recv_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs );
	
	virtual void
	reset()
	{
	}
	
protected:

	enum state5_t
	{
		waiting_for_opening_response			= 0x1,
		waiting_for_authentication_response		= 0x2,
		waiting_for_connect_response			= 0x3,
		connected								= 0x4
	};
		
	void
	start_negotiate();
	
	buffer
	make_connect_buffer();
	
	buffer
	make_auth_buffer();
	
	ip::endpoint				m_to;
	bool						m_sent_greeting = false;
	buffer						m_recv_handshake;
	std::uint8_t				m_method;
	stream::state_t				m_state;
	buffer						m_send_queue;
	state5_t					m_state5	= waiting_for_opening_response;
	sockaddr_storage			m_dest;
};

#if defined( __APPLE__ )
#	pragma mark proxy implementation
#endif

const std::string					proxy::manager::was_changed_event = "proxy_manager_was_changed_event";
proxy::manager::auth_challenge_f	proxy::manager::m_auth_challenge_handler;

NODEOZE_DEFINE_SINGLETON( proxy::manager )

proxy::manager*
proxy::manager::create()
{
	return new proxy::manager;
}

proxy::manager::manager()
{
}


proxy::manager::~manager()
{
}


stream::filter*
proxy::manager::create_filter( const ip::endpoint &to )
{
	stream::filter *filter = nullptr;

	if ( m_resource.scheme() == "socks" )
	{
		filter = new socks5_proxy( to );
	}
	
	return filter;
}


void
proxy::manager::on_auth_challenge( auth_challenge_f handler )
{
	m_auth_challenge_handler = handler;
}


bool
proxy::manager::auth_challenge()
{
	bool ok = false;

	if ( m_auth_challenge_handler )
	{
		ok = m_auth_challenge_handler();
	}
		
	return ok; 
}


bool
proxy::manager::is_null() const
{
	return ( m_resource.scheme().size() == 0 );
}


bool
proxy::manager::is_http() const
{
	return ( m_resource.scheme() == "http" );
}


bool
proxy::manager::is_socks() const
{
	return ( m_resource.scheme() == "socks" );
}


void
proxy::manager::encode_authorization( const std::string &username, const std::string &password )
{
	m_authorization = codec::base64::encode( nodeoze::buffer{ username + ":" + password } );
}


void
proxy::manager::decode_authorization( std::string &username, std::string &password ) const
{
	auto tmp = codec::base64::decode( m_authorization ).to_string();

	if ( tmp.size() > 0 )
	{
		std::size_t spot = tmp.find( ':' );

		if ( spot != std::string::npos )
		{
			username = tmp.substr( 0, spot );
			password = tmp.substr( spot + 1, m_authorization.size() );
		}
	}
}


bool
proxy::manager::bypass( const class uri &uri )
{
	bool bypass = false;

	if ( ( uri.host() == "localhost" ) || ( uri.host() == "127.0.0.1" ) )
	{
		bypass = true;
	}

	return bypass;
}


/*
void
proxy::deflate( any &root ) const
{
	if ( m_uri.scheme().size() > 0 )
	{
		root[ "uri" ] = m_uri.to_string();
	}
		
	root[ "authorization" ]	= m_authorization;

	for ( auto it = m_bypass_list.begin(); it != m_bypass_list.end(); it++ )
	{
		root[ "bypass" ].append( *it );
	}
}


void
proxy::inflate( const any &root )
{
	if ( root[ "uri" ].is_string() )
	{
		m_uri = nodeoze::uri( root[ "uri" ].to_string() );
	}

	m_authorization	= root[ "authorization" ].to_string();

	for ( auto i = 0; i < root[ "bypass" ].size(); i++ )
	{
		m_bypass_list.push_back( root[ "bypass" ][ i ].to_string() );
	}
}
*/


void
proxy::manager::set_resource( const uri &resource )
{
	if ( m_resource != resource )
	{
		m_resource = resource;
		
		if ( m_resource.username().size() > 0 )
		{
			encode_authorization( m_resource.username(), m_resource.password() );
		}
		
		notification::shared().publish( notification::local, 0, make_oid( this ), was_changed_event );
	}
}


void
proxy::manager::clear()
{
	if ( m_resource.scheme() != "" )
	{
		m_resource.clear();
		notification::shared().publish( notification::local, 0, make_oid( this ), was_changed_event );
	}
}


#if defined( __APPLE__ )
#	pragma mark socks5_proxy implementation
#endif

socks5_proxy::socks5_proxy( const nodeoze::ip::endpoint &to )
:
	m_to( to )
{
	mlog( marker::proxy_socks, log::level_t::info, "to: %", to.to_string() );
	m_state = stream::state_t::handshaking;
}

	
socks5_proxy::~socks5_proxy()
{
	mlog( marker::proxy_socks, log::level_t::info, "" );
}


stream::state_t
socks5_proxy::send( buffer &in_buf, buffer &out_buf )
{
	mlog( marker::proxy_socks, log::level_t::info, "state = %: sending % bytes", static_cast< std::uint32_t >( m_state ), in_buf.size() );
	
	if ( m_state == stream::state_t::handshaking )
	{
		if ( !m_sent_greeting )
		{
			mlog( marker::proxy_socks, log::level_t::info, "sending greeting..." );
			
			if ( proxy::manager::shared().authorization().size() > 0 )
			{
				out_buf.size( 4 );
			
				out_buf.put( 0, 0x05 );
				out_buf.put( 1, 0x02 );
				out_buf.put( 2, 0x00 );
				out_buf.put( 3, 0x02 );
			}
			else
			{
				out_buf.size( 3 );
				
				out_buf.put( 0, 0x05 );
				out_buf.put( 1, 0x01 );
				out_buf.put( 2, 0x00 );
			}
			
			mlog( marker::proxy_socks, log::level_t::info, "greeting size: %", out_buf.size() );
			
			m_sent_greeting = true;
		}
		
		m_send_queue.append( in_buf );
		
		m_state5 = waiting_for_opening_response;
		m_state = stream::state_t::connected;
	}
	else if ( m_state5 != connected )
	{
		m_send_queue.append( in_buf );
	}
	else
	{
		out_buf = std::move( in_buf );
	}
	
	return m_state;
}

		
stream::state_t
socks5_proxy::recv( std::vector< buffer > &in_recv_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs )
{
	mlog( marker::proxy_socks, log::level_t::info, "state = %: received % buffers", static_cast< std::uint32_t >( m_state ), in_recv_bufs.size() );
	
	switch ( m_state5 )
	{
		case waiting_for_opening_response:
		{
			std::size_t bytes_available = in_recv_bufs[ 0 ].size();
			mlog( marker::proxy_socks, log::level_t::info, "bytes available: %", bytes_available );
			std::size_t handshake_left	= 2 - m_recv_handshake.size();
			mlog( marker::proxy_socks, log::level_t::info, "handshake left: %", handshake_left );
	
			m_recv_handshake.append( in_recv_bufs[ 0 ].const_data(), std::min( handshake_left, bytes_available ) );
			
			if ( m_recv_handshake.size() < 2 )
			{
				in_recv_bufs[ 0 ].rotate( 0, std::min( handshake_left, bytes_available ), in_recv_bufs[ 0 ].size() );
			}
			else
			{
				if ( m_recv_handshake[ 0 ] == 0x05 )
				{
					if ( m_recv_handshake[ 1 ] == 0x00 )
					{
						out_send_buf	= make_connect_buffer();
						out_recv_bufs.clear();
						
						m_state5 = waiting_for_connect_response;
						mlog( marker::proxy_socks, log::level_t::info, "new state: waiting for connect" );
					}
					else if ( m_recv_handshake[ 1 ] == 0x02 )
					{
						out_send_buf	= make_auth_buffer();
						out_recv_bufs.clear();
	
						m_state5 = waiting_for_authentication_response;
						mlog( marker::proxy_socks, log::level_t::info, "new state: waiting for authentication response...auth size: %", out_send_buf.size() );
					}
					else
					{
						mlog( marker::proxy_socks, log::level_t::info, "new state: error (%,%)", static_cast< std::uint32_t >( m_recv_handshake[ 0 ] ), static_cast< std::uint32_t >( m_recv_handshake[ 1 ] ) );
						m_state = stream::state_t::error;
					}
				}
				else
				{
					mlog( marker::proxy_socks, log::level_t::info, "new state: error (%,%)", static_cast< std::uint32_t >( m_recv_handshake[ 0 ] ), static_cast< std::uint32_t >( m_recv_handshake[ 1 ] ) );
					m_state = stream::state_t::error;
				}
					
				m_recv_handshake.clear();
			}
		}
		break;
		
		case waiting_for_authentication_response:
		{
			std::size_t bytes_available = in_recv_bufs[ 0 ].size();
			std::size_t handshake_left	= 2 - m_recv_handshake.size();
	
			m_recv_handshake.append( in_recv_bufs[ 0 ].const_data(), std::min( handshake_left, bytes_available ) );
			
			if ( m_recv_handshake.size() < 2 )
			{
				in_recv_bufs[ 0 ].rotate( 0, std::min( handshake_left, bytes_available ), in_recv_bufs[ 0 ].size() );
			}
			else
			{
				if ( ( m_recv_handshake[ 0 ] == 0x01 ) && ( m_recv_handshake[ 1 ] == 0x00 ) )
				{
					out_send_buf = make_connect_buffer();
					out_recv_bufs.clear();
					
					mlog( marker::proxy_socks, log::level_t::info, "new state: waiting for connect" );
					m_state5 = waiting_for_connect_response;
				}
				else
				{
					mlog( marker::proxy_socks, log::level_t::info, "new state: error (%,%)", static_cast< std::uint32_t >( m_recv_handshake[ 0 ] ), static_cast< std::uint32_t >( m_recv_handshake[ 1 ] ) );
					m_state = stream::state_t::error;
				}
				
				m_recv_handshake.clear();
			}
		}
		break;
				
		case waiting_for_connect_response:
		{
			std::size_t bytes_available = in_recv_bufs[ 0 ].size();
			mlog( marker::proxy_socks, log::level_t::info, "bytes available: %", bytes_available );
			std::size_t handshake_left	= 4 - m_recv_handshake.size();
			mlog( marker::proxy_socks, log::level_t::info, "handshake left: %", handshake_left );
	
			m_recv_handshake.append( in_recv_bufs[ 0 ].const_data(), std::min( handshake_left, bytes_available ) );
			
			if ( m_recv_handshake.size() < 4 )
			{
				in_recv_bufs[ 0 ].rotate( 0, std::min( handshake_left, bytes_available ), in_recv_bufs[ 0 ].size() );
			}
			else if ( ( m_recv_handshake[ 0 ] == 0x05 ) && ( m_recv_handshake[ 1 ] == 0x00 ) )
			{
				for ( auto i = 0; i < 4; i++ )
				{
					mlog( marker::proxy_socks, log::level_t::info, "handshake[%]: %", i, static_cast< std::uint32_t >( m_recv_handshake[ i ] ) );
				}
				
				switch ( m_recv_handshake[ 3 ] )
				{
					case 0x01:
					{
						in_recv_bufs[ 0 ].rotate( 0, 6, in_recv_bufs[ 0 ].size() );
						
						out_send_buf = std::move( m_send_queue );
						
						m_state5 = connected;
						m_state = stream::state_t::connected;
					}
					break;
							
					case 0x03:
					{
						auto len = in_recv_bufs[ 0 ][ 0 ];
							
						in_recv_bufs[ 0 ].rotate( 0, len + 1, in_recv_bufs[ 0 ].size() );
						
						out_send_buf = std::move( m_send_queue );
							
						m_state5 = connected;
						m_state = stream::state_t::connected;
					}
					break;
								
					case 0x04:
					{
						in_recv_bufs[ 0 ].rotate( 0, 18, in_recv_bufs[ 0 ].size() );
						
						out_send_buf = std::move( m_send_queue );
						
						m_state5 = connected;
						m_state = stream::state_t::connected;
					}
					break;
						
					default:
					{
						m_state = stream::state_t::error;
					}
				}
			}
		}
		break;
		
		default:
		{
			out_recv_bufs = std::move( in_recv_bufs );
		}
	}
	
	return m_state;
}


buffer
socks5_proxy::make_auth_buffer()
{
	std::string		username;
	std::string		password;
	std::uint8_t	ulen;
	std::uint8_t	plen;
//	buffer			buf;
	
	proxy::manager::shared().decode_authorization( username, password );
	
	ulen = username.size() > 255 ? 255 : ( std::uint8_t ) username.size();
	plen = password.size() > 255 ? 255 : ( std::uint8_t ) password.size();

	buffer buf( 3 + ulen + plen );
	std::size_t ibuf = 0;


//	buf.push_back( 0x01 );
//	buf.push_back( ulen );
	buf.put( ibuf++, 0x01 );
	buf.put( ibuf++, ulen );

	
//	for ( auto &c : username )
//	{
//		buf.push_back( c );
//	}
	for ( auto i = 0; i < ulen; ++i )
	{
		buf.put( ibuf++, username[i] );
	}
	
//	buf.push_back( plen );
	buf.put( ibuf++, ulen );

//	for ( auto &c : password )
//	{
//		buf.push_back( c );
//	}
	for ( auto i = 0; i < plen; ++i )
	{
		buf.put( ibuf++, password[i] );
	}
	
	return buf;
}

	
buffer
socks5_proxy::make_connect_buffer()
{
//	buffer buf;
	buffer buf( 10 );
	std::size_t ibuf = 0;

	sockaddr_storage addr;
	sockaddr_in		*v4;
			
	ip::endpoint_to_sockaddr( m_to, addr );
			
	v4 = reinterpret_cast< sockaddr_in* >( &addr );
/*			
	buf.push_back( 0x05 );
	buf.push_back( 0x01 );
	buf.push_back( 0x00 );
	buf.push_back( 0x01 );
	buf.push_back( ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 0 ] );
	buf.push_back( ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 1 ] );
	buf.push_back( ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 2 ] );
	buf.push_back( ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 3 ] );
	buf.push_back( ( ( std::uint8_t* ) &v4->sin_port )[ 0 ] );
	buf.push_back( ( ( std::uint8_t* ) &v4->sin_port )[ 1 ] );
*/	
	buf.put( ibuf++, 0x05 );
	buf.put( ibuf++, 0x01 );
	buf.put( ibuf++, 0x00 );
	buf.put( ibuf++, 0x01 );
	buf.put( ibuf++, ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 0 ] );
	buf.put( ibuf++, ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 1 ] );
	buf.put( ibuf++, ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 2 ] );
	buf.put( ibuf++, ( ( std::uint8_t* ) &v4->sin_addr.s_addr )[ 3 ] );
	buf.put( ibuf++, ( ( std::uint8_t* ) &v4->sin_port )[ 0 ] );
	buf.put( ibuf++, ( ( std::uint8_t* ) &v4->sin_port )[ 1 ] );
	
	return buf;
}

