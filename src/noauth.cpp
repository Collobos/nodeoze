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

#include <nodeoze/noauth.h>
#include <nodeoze/nlog.h>
#include <nodeoze/nhttp.h>
#include <nodeoze/nany.h>
#include <nodeoze/njson.h>
#include <chrono>

using namespace nodeoze;
using namespace std::chrono;

oauth::oauth( const std::string &client_id, const std::string &client_secret, const std::string &auth_server_uri )
:
	m_client_id( client_id ),
	m_client_secret( client_secret ),
	m_auth_server_uri( auth_server_uri )
{
	m_token.refresh_token	= "";
	m_token.expire_time		= system_clock::now() - seconds( 1 );
}


oauth::oauth( const std::string &client_id, const std::string &client_secret, const std::string &auth_server_uri, const std::string &refresh_token )
:
	m_client_id( client_id ),
	m_client_secret( client_secret ),
	m_auth_server_uri( auth_server_uri )
{
	m_token.refresh_token	= refresh_token;
	m_token.expire_time		= system_clock::now() - seconds( 1 );
}


oauth::oauth( const std::string &client_id, const std::string &client_secret, const std::string &auth_server_uri, const std::string &refresh_token, const std::string &access_token, const std::chrono::system_clock::time_point &expire_time )
:
	m_client_id( client_id ),
	m_client_secret( client_secret ),
	m_auth_server_uri( auth_server_uri )
{
	m_token.refresh_token	= refresh_token;
	m_token.access_token	= access_token;
	m_token.expire_time		= expire_time;
}


void
oauth::access_token( access_token_reply_f reply )
{
	if ( m_token.expire_time < system_clock::now() )
	{
		m_update_queue.push( reply );

		if ( m_update_queue.size() == 1 )
		{
			auto request = std::make_shared< http::message >( http::method_t::post, m_auth_server_uri );

			request->add_header_field( "Content-Type", std::string( "application/x-www-form-urlencoded" ) );

			*request << "client_id=" << m_client_id << "&client_secret=" << m_client_secret << "&refresh_token="
		         << m_token.refresh_token << "&grant_type=" << "refresh_token";
				 
			auto conn = connection::create< http::connection >( request->resource() );

			conn->send_request( request )
			.then( [=]( http::message::ptr response ) mutable
			{
				m_token.expire_time = system_clock::now();
	
				if ( response->code() == http::code_t::ok )
				{
					auto root = json::inflate( response->body().to_string() );
						
					if ( !root.is_null() && root.is_member( "access_token" ) && root[ "access_token" ].is_string() )
					{
						m_token.access_token = root[ "access_token" ].to_string();
			
						std::uint32_t seconds_until_expire = root[ "expires_in" ].to_uint32();
						m_token.expire_time = system_clock::now() + seconds( seconds_until_expire );
					}
					else
					{
						nlog( log::level_t::error, "bad json format: %", response->body().to_string() );
					}
				}
				else
				{
					nlog( log::level_t::error, "http error trying to refresh access token: %", response->code() );
				}
		
				while ( !m_update_queue.empty() )
				{
					m_update_queue.top()( err_t::ok, true, m_token );
					m_update_queue.pop();
				}
			},
			[=]( std::error_code err ) mutable
			{
				nunused( err );
			} );
		}
	}
	else
	{
		reply( err_t::ok, false, m_token );
	}
}
