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
 
#include <nodeoze/http.h>
#include <nodeoze/machine.h>
#include <nodeoze/runloop.h>
#include <nodeoze/rpc.h>
#include <nodeoze/mime.h>
#include <nodeoze/tls.h>
#include <nodeoze/ws.h>
#include <nodeoze/base64.h>
#include <nodeoze/string.h>
#include <nodeoze/proxy.h>
#include <nodeoze/macros.h>
#include <http_parser.h>
#include <algorithm>
#include <fstream>
#include <regex>
#include <assert.h>
#include <stdarg.h>
#if defined(WIN32)
#	include <shlwapi.h>
#	include <tchar.h>
#	include <errno.h>
#	define strcasecmp _stricmp
#else
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <sys/stat.h>
#	include <sys/errno.h>
#	define TCHAR char
#	define TEXT( X ) X
#endif

#if 0

#if defined(WIN32)

#	include <winspool.h>
#	include <limits.h>
#	include <sys/types.h>
#	include <string.h>
#	define strcasecmp _stricmp

#endif

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark method implementation
#endif

const std::string&
http::to_string( http::method_t val )
{
	switch ( val )
	{
		case method_t::delet:
		{
			static const std::string s( "DELETE" );
			return s;
		}
		break;
		
		case method_t::get:
		{
			static const std::string s( "GET" );
			return s;
		}
		break;
		
		case method_t::head:
		{
			static const std::string s( "HEAD" );
			return s;
		}
		break;
		
		case method_t::post:
		{
			static const std::string s( "POST" );
			return s;
		}
		break;
		
		case method_t::put:
		{
			static const std::string s( "PUT" );
			return s;
		}
		break;
		
		case method_t::connect:
		{
			static const std::string s( "CONNECT" );
			return s;
		}
		break;
		
		case method_t::options:
		{
			static const std::string s( "OPTIONS" );
			return s;
		}
		break;
		
		case method_t::trace:
		{
			static const std::string s( "TRACE" );
			return s;
		}
		break;
		
		case method_t::copy:
		{
			static const std::string s( "COPY" );
			return s;
		}
		break;
		
		case method_t::lock:
		{
			static const std::string s( "LOCK" );
			return s;
		}
		break;
		
		case method_t::mkcol:
		{
			static const std::string s( "MKCOL" );
			return s;
		}
		break;
		
		case method_t::move:
		{
			static const std::string s( "MOVE" );
			return s;
		}
		break;
		
		case method_t::propfind:
		{
			static const std::string s( "PROPFIND" );
			return s;
		}
		break;
		
		case method_t::proppatch:
		{
			static const std::string s( "PROPPATCH" );
			return s;
		}
		break;
		
		case method_t::search:
		{
			static const std::string s( "SEARCH" );
			return s;
		}
		break;
		
		case method_t::unlock:
		{
			static const std::string s( "UNLOCK" );
			return s;
		}
		break;
		
		case method_t::report:
		{
			static const std::string s( "REPORT" );
			return s;
		}
		break;
		
		case method_t::mkactivity:
		{
			static const std::string s( "MKACTIVITY" );
			return s;
		}
		break;
		
		case method_t::checkout:
		{
			static const std::string s( "CHECKOUT" );
			return s;
		}
		break;
		
		case method_t::merge:
		{
			static const std::string s( "MERGE" );
			return s;
		}
		break;
		
		case method_t::msearch:
		{
			static const std::string s( "MSEARCH" );
			return s;
		}
		break;
		
		case method_t::notify:
		{
			static const std::string s( "NOTIFY" );
			return s;
		}
		break;
		
		case method_t::subscribe:
		{
			static const std::string s( "SUBSCRIBE" );
			return s;
		}
		break;
		
		case method_t::unsubscribe:
		{
			static const std::string s( "UNSUBSCRIBE" );
			return s;
		}
		break;
		
		case method_t::patch:
		{
			static const std::string s( "PATCH" );
			return s;
		}
		break;
		
		case method_t::purge:
		{
			static const std::string s( "PURGE" );
			return s;
		}
		break;

		default:
		{
			static const std::string s( "UNKNOWN" );
			return s;
		}
		break;
	}
}

#if defined( __APPLE__ )
#	pragma mark code implementation
#endif

const std::string&
http::to_string( http::code_t val )
{
	switch ( val )
	{
		case code_t::cont:
		{
			static std::string s( "Continue" );
			return s;
		}
		break;

		case code_t::switching_protocols:
		{
			static std::string s( "Switching Protocols" );
			return s;
		}
		break;

		case code_t::ok:
		{
			static std::string s( "OK" );
			return s;
		}
		break;

		case code_t::created:
		{
			static std::string s( "Created" );
			return s;
		}
		break;

		case code_t::accepted:
		{
			static std::string s( "Accepted" );
			return s;
		}
		break;

		case code_t::not_authoritative:
		{
			static std::string s( "Non-Authoritative Information" );
			return s;
		}
		break;

		case code_t::no_content:
		{
			static std::string s( "No Content" );
			return s;
		}
		break;

		case code_t::reset_content:
		{
			static std::string s( "Reset Content" );
			return s;
		}
		break;

		case code_t::partial_content:
		{
			static std::string s( "Error" );
			return s;
		}
		break;


		case code_t::multiple_choices:
		{
			static std::string s( "Multiple Choices" );
			return s;
		}
		break;

		case code_t::moved_permanently:
		{
			static std::string s( "Moved Permanently" );
			return s;
		}
		break;

		case code_t::moved_temporarily:
		{
			static std::string s( "Moved Temporarily" );
			return s;
		}
		break;

		case code_t::see_other:
		{
			static std::string s( "See Other" );
			return s;
		}
		break;

		case code_t::not_modified:
		{
			static std::string s( "Not Modified" );
			return s;
		}
		break;

		case code_t::use_proxy:
		{
			static std::string s( "Use Proxy" );
			return s;
		}
		break;

		case code_t::bad_request:
		{
			static std::string s( "Bad Request" );
			return s;
		}
		break;

		case code_t::unauthorized:
		{
			static std::string s( "Authorization Required" );
			return s;
		}
		break;

		case code_t::payment_required:
		{
			static std::string s( "Payment Required" );
			return s;
		}
		break;

		case code_t::forbidden:
		{
			static std::string s( "Forbidden" );
			return s;
		}
		break;

		case code_t::not_found:
		{
			static std::string s( "Not Found" );
			return s;
		}
		break;

		case code_t::method_not_allowed:
		{
			static std::string s( "Not Allowed" );
			return s;
		}
		break;

		case code_t::not_acceptable:
		{
			static std::string s( "Not Acceptable" );
			return s;
		}
		break;

		case code_t::proxy_authentication:
		{
			static std::string s( "Proxy Authentication" );
			return s;
		}
		break;

		case code_t::request_timeout:
		{
			static std::string s( "Request Timeout" );
			return s;
		}
		break;

		case code_t::conflict:
		{
			static std::string s( "Conflict" );
			return s;
		}
		break;

		case code_t::gone:
		{
			static std::string s( "Gone" );
			return s;
		}
		break;

		case code_t::length_required:
		{
			static std::string s( "Length Required" );
			return s;
		}
		break;

		case code_t::precondition:
		{
			static std::string s( "Precondition" );
			return s;
		}
		break;

		case code_t::request_too_large:
		{
			static std::string s( "Too Large" );
			return s;
		}
		break;

		case code_t::uri_too_long:
		{
			static std::string s( "Too Long" );
			return s;
		}
		break;

		case code_t::unsupported_media_type:
		{
			static std::string s( "Unsupported Media Type" );
			return s;
		}
		break;

		case code_t::requested_range:
		{
			static std::string s( "Requested Range" );
			return s;
		}
		break;

		case code_t::expectation_failed:
		{
			static std::string s( "Failed" );
			return s;
		}
		break;

		case code_t::upgrade_required:
		{
			static std::string s( "Upgrade Required" );
			return s;
		}
		break;

		case code_t::server_error:
		{
			static std::string s( "Server Error" );
			return s;
		}
		break;

		case code_t::not_implemented:
		{
			static std::string s( "Not Implemented" );
			return s;
		}
		break;

		case code_t::bad_gateway:
		{
			static std::string s( "Bad Gateway" );
			return s;
		}
		break;

		case code_t::service_unavailable:
		{
			static std::string s( "Service Unavailable" );
			return s;
		}
		break;

		case code_t::gateway_timeout:
		{
			static std::string s( "Gateway Timeout" );
			return s;
		}
		break;

		case code_t::not_supported:
		{
			static std::string s( "Not Supported" );
			return s;
		}
		break;

		case code_t::authorized_cancelled:
		{
			static std::string s( "Authorized Cancelled" );
			return s;
		}
		break;

		case code_t::pki_error:
		{
			static std::string s( "PKI Error" );
			return s;
		}
		break;

		case code_t::webif_disabled:
		{
			static std::string s( "Webif Disabled" );
			return s;
		}
		break;

		default:
		{
			static std::string s( "Unknown" );
			return s;
		}
		break;
	}
}

#if defined( __APPLE__ )
#	pragma mark http::message implementation
#endif

http::message::message()
{
}


http::message::message( method_t method, const uri &resource )
:
	m_method( method ),
	m_resource( resource ),
	m_keep_alive( true ),
	m_type( type_t::request )
{
	add_header_field( "Host", resource.host() );
	add_header_field( "User-Agent", "nodeoze/2 " + machine::self().description() );
	//add_header_field( "User-Agent", "curl/7.54.0" );

	add_header_field( "Connection", "keep-alive" );
	
	if ( proxy::manager::shared().authorization().size() > 0 )
	{
		add_proxy_authorization_header_fields();
	}
}


http::message::message( const message &request, code_t code )
:
	m_code( code ),
	m_major( request.m_major ),
	m_minor( request.m_minor ),
	m_keep_alive( request.m_keep_alive ),
	m_type( type_t::response )
{
	time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
#if defined( WIN32 )
	localtime_s( &tstruct, &now );
#else
    tstruct = *localtime( &now );
#endif
    strftime( buf, sizeof( buf ), "%a, %d %b %Y %I:%M:%S %Z", &tstruct );
	
	add_header_field( "Server", "nodeoze/2.5 OpenSSL/1.0.2j" );
	add_header_field( "Date", buf );

	if ( m_keep_alive )
	{
		add_header_field( "Connection", "Keep-Alive" );
	}
}


http::message::message( const message &rhs )
:
	m_major( rhs.m_major ),
	m_minor( rhs.m_minor ),
	m_header( rhs.m_header ),
	m_content_type( rhs.m_content_type ),
	m_content_length( rhs.m_content_length ),
	m_upgrade( rhs.m_upgrade ),
	m_ws_key( rhs.m_ws_key ),
	m_keep_alive( rhs.m_keep_alive ),
	m_authorization( rhs.m_authorization ),
	m_username( rhs.m_username ),
	m_password( rhs.m_password ),
	m_from( rhs.m_from ),
	m_type( rhs.m_type )
{
}


http::message::message( message &&rhs )
:
	m_major( rhs.m_major ),
	m_minor( rhs.m_minor ),
	m_header( rhs.m_header ),
	m_content_type( rhs.m_content_type ),
	m_content_length( rhs.m_content_length ),
	m_upgrade( rhs.m_upgrade ),
	m_ws_key( rhs.m_ws_key ),
	m_keep_alive( rhs.m_keep_alive ),
	m_authorization( rhs.m_authorization ),
	m_username( rhs.m_username ),
	m_password( rhs.m_password ),
	m_from( rhs.m_from ),
	m_type( rhs.m_type )
{
}

	
http::message::~message()
{
}


http::message&
http::message::operator=( const http::message &rhs )
{
	m_major				= rhs.m_major;
	m_minor				=  rhs.m_minor;
	m_header			= rhs.m_header;
	m_content_type		= rhs.m_content_type;
	m_content_length	= rhs.m_content_length;
	m_upgrade			= rhs.m_upgrade;
	m_ws_key			= rhs.m_ws_key;
	m_keep_alive		= rhs.m_keep_alive;
	m_authorization		= rhs.m_authorization;
	m_username			= rhs.m_username;
	m_password			= rhs.m_password;
	m_from				= rhs.m_from;
	m_type				= rhs.m_type;

	return *this;
}


http::message&
http::message::operator=( http::message &&rhs )
{
	m_major				= rhs.m_major;
	m_minor				=  rhs.m_minor;
	m_header			= rhs.m_header;
	m_content_type		= rhs.m_content_type;
	m_content_length	= rhs.m_content_length;
	m_upgrade			= rhs.m_upgrade;
	m_ws_key			= rhs.m_ws_key;
	m_keep_alive		= rhs.m_keep_alive;
	m_authorization		= rhs.m_authorization;
	m_username			= rhs.m_username;
	m_password			= rhs.m_password;
	m_from				= rhs.m_from;
	m_type				= rhs.m_type;

	return *this;
}


void
http::message::preflight()
{
	if ( ( content_type() != mime::null ) && ( m_body.size() > 0 ) )
	{
		std::ostringstream os;
	
		os << content_type();
		add_header_field( "Content-Type", os.str() );
		add_header_field( "Content-Length", ( int ) m_body.size() );
	}
	else
	{
		add_header_field( "Content-Length", 0 );
	}
	
	if ( header().find( "Accept" ) == header().end() )
	{
		add_header_field( "Accept", "*/*" );
	}
}


void
http::message::prologue( std::ostream &os ) const
{
	switch ( m_type )
	{
		case message::type_t::request:
		{
			os << m_method << " ";
		
			if ( m_method == http::method_t::connect )
			{
				os << m_resource.host() << ":" << m_resource.port();
			}
			else if ( proxy::manager::shared().is_http() && ( m_resource.scheme() == "http" ) )
			{
				os << m_resource.to_string();
			}
			else
			{
				os << m_resource.path();

				if ( !m_resource.query().empty() )
				{
					os << "?" << m_resource.query();
				}
			}

			os << " HTTP/1.1" << http::endl;
		}
		break;
		
		case message::type_t::response:
		{
			os << "HTTP/" << m_major << "." << m_minor << " ";
		
			os << m_code << " " << http::to_string( m_code );
		
			os << http::endl;
		}
		break;
	}
}


void
http::message::add_basic_authentication( const std::string &username, const std::string &password )
{
	add_header_field( "Authorization", "Basic " + codec::base64::encode( nodeoze::buffer{ username + ":" + password } ) );
}


void
http::message::header( std::ostream &os ) const
{
	for ( auto &it : m_header )
	{
		os << it.first << ": " << it.second << http::endl;
	}
			
	os << http::endl;
}


void
http::message::add_header_fields( const header_type &heeder )
{
	for ( auto it : heeder )
	{
		add_header_field( it.first, it.second );
	}
}

	
void
http::message::add_header_field( const std::string &key, const std::string &val )
{
	m_header[ key ] = val;
	
	if ( string::case_insensitive_compare( key, "Content-Length" ) )
	{
		m_content_length = atoi( val.c_str() );
	}
	else if ( string::case_insensitive_compare( key, "Content-Type" ) )
	{
		m_content_type = val;
	}
	else if ( string::case_insensitive_compare( key, "Host" ) )
    {
		m_host = val;
    }
    else if ( string::case_insensitive_compare( key, "Expect" ) )
    {
		m_expect = val;
    }
	else if ( string::case_insensitive_compare( key, "User-Agent" ) )
	{
		m_user_agent = val;
	}
	else if ( string::case_insensitive_compare( key, "Authorization" ) )
	{
		std::string base64Encoded;
		std::string decoded;
		size_t      pos;

		m_authorization = val;
		
        base64Encoded	= m_authorization.substr( m_authorization.find( ' ' ) + 1 );

        decoded			= codec::base64::decode( base64Encoded ).to_string();
		pos				= decoded.find( ':' );

		if ( pos != std::size_t( -1 ) )
		{
			m_username  = decoded.substr( 0, pos );
			m_password  = decoded.substr( pos + 1 );
        }
	}
	else if ( string::case_insensitive_compare( key, "Upgrade" ) )
	{
		m_upgrade = val;
	}
	else if ( string::case_insensitive_compare( key, "Sec-WebSocket-Key" ) )
	{
		m_ws_key = val;
	}
}


bool
http::message::validate_ws_request( std::string &key ) const
{
	std::string val;
	bool		ok = false;
	
	val = find( "Upgrade" );
	ncheck_error_quiet( val.size() > 0, exit );
	ncheck_error_quiet( string::case_insensitive_compare( val, "websocket" ), exit );
	
	val = find( "Connection" );
	ncheck_error_quiet( val.size() > 0, exit );
	ncheck_error_quiet( val.find( "Upgrade" ) != std::string::npos, exit );
	
	val = find( "Sec-WebSocket-Version" );
	ncheck_error_quiet( val.size() > 0, exit );
	ncheck_error_quiet( val == "13", exit );
			
	key = find( "Sec-WebSocket-Key" );
	ncheck_error_quiet( key.size() > 0, exit );
	
	ok = true;
	
exit:

	return ok;
}


std::string
http::message::find( const std::string &key ) const
{
	std::string val;
	auto		it = m_header.find( key );

	if ( it != m_header.end() )
	{
		val = it->second;
	}

	return val;
}


void
http::message::remove_header_field( const std::string &key )
{
	m_header.erase( key );
}


void
http::message::write( const uint8_t *buf, size_t len )
{
	m_body.append( buf, len );
}


promise< void >
http::message::body( std::ostream &os ) const
{
	auto ret = promise< void >();
	
	if ( m_body.size() > 0 )
	{
		os.write( reinterpret_cast< const char* >( m_body.const_data() ), m_body.size() );
	}
	
	ret.resolve();
	
	return ret;
}


void
http::message::on_begin()
{
	m_header_field.clear();
	m_header_value.clear();
	m_resource.clear();
	m_parse_state = NONE;
}

	
void
http::message::on_uri( const char *buf, std::size_t len )
{
	std::string tmp( buf, len );
	
	m_resource.assign( tmp );
}

	
void
http::message::on_header_field( const char *buf, std::size_t len )
{
	if ( m_parse_state != FIELD )
	{
		if ( ( m_header_field.size() > 0 ) && ( m_header_value.size() > 0 ) )
		{
			add_header_field( m_header_field, m_header_value );
		}
		
		m_header_field.assign( buf, buf + len );
		m_parse_state = FIELD;
	}
	else
	{
		m_header_field.append( buf, buf + len );
	};
}

	
void
http::message::on_header_value( const char *buf, std::size_t len )
{
	if ( m_parse_state != VALUE )
	{
		m_header_value.assign( buf, buf + len );
		m_parse_state = VALUE;
	}
	else
	{
		m_header_value.append( buf, buf + len );
	}
}

	
void
http::message::on_headers()
{
	if ( ( m_header_field.size() > 0 ) && ( m_header_value.size() > 0 ) )
	{
		add_header_field( m_header_field, m_header_value );
	}
}

	
void
http::message::on_body( buffer &buf )
{
	m_body.append( buf );
}

	
void
http::message::on_end()
{
}


std::shared_ptr< http::message >
http::message::create() const
{
	return std::make_shared< http::message >();
}


#if defined( __APPLE__ )
#	pragma mark http::connection implementation
#endif

const std::size_t http::connection::default_buffer_size = 32767;

static connection::factory
factory( { "http", "https" }, []( const uri &resource )
{
	return ( resource.scheme() == "https" ) ? true : false;
},
[]( const uri &resource )
{
	return std::make_shared< http::connection >( resource );
},
[]( const uri &resource, ip::tcp::socket socket )
{
	nunused( resource );
	return std::make_shared< http::connection >( std::move( socket ) );
} );


http::connection::connection()
{
}


http::connection::connection( const nodeoze::uri &resource )
:
	nodeoze::connection( resource ),
	m_okay( true ),
	m_settings( new http_parser_settings ),
	m_parser( new http_parser ),
	m_in_message_type( message::type_t::response )
{
	memset( m_settings.get(), 0, sizeof( http_parser_settings ) );
	memset( m_parser.get(), 0, sizeof( http_parser ) );
	
	m_settings->on_message_begin 	= message_will_begin;
	m_settings->on_url				= uri_was_received;
	m_settings->on_header_field		= header_field_was_received;
	m_settings->on_header_value		= header_value_was_received;
	m_settings->on_headers_complete	= headers_were_received;
	m_settings->on_body				= body_was_received;
	m_settings->on_message_complete	= message_was_received;
	
	http_parser_init( m_parser.get(), HTTP_RESPONSE );
	m_parser->data = this;
	
	setup_buffer();
}


http::connection::connection( ip::tcp::socket sock )
:
	nodeoze::connection( std::move( sock ) ),
	m_okay( true ),
	m_settings( new http_parser_settings ),
	m_parser( new http_parser ),
	m_in_message_type( message::type_t::request )
{
	memset( m_settings.get(), 0, sizeof( http_parser_settings ) );
	memset( m_parser.get(), 0, sizeof( http_parser ) );
	
	m_settings->on_message_begin 	= server::message_will_begin;
	m_settings->on_url				= server::uri_was_received;
	m_settings->on_header_field		= server::header_field_was_received;
	m_settings->on_header_value		= server::header_value_was_received;
	m_settings->on_headers_complete	= server::headers_were_received;
	m_settings->on_body				= server::body_was_received;
	m_settings->on_message_complete	= server::message_was_received;
	
	http_parser_init( m_parser.get(), HTTP_REQUEST );
	m_parser->data = this;
	
	setup_buffer();
}


http::connection::~connection()
{
	mlog( marker::http, log::level_t::info, "" );
}


int
http::connection::method() const
{
	return m_parser->method;
}


int
http::connection::status_code() const
{
	return m_parser->status_code;
}


int
http::connection::http_major() const
{
	return m_parser->http_major;
}


int
http::connection::http_minor() const
{
	return m_parser->http_minor;
}


promise< http::message::ptr >
http::connection::send_request( message::ptr &request )
{
	bool	is_https_request	= request->resource().scheme() == "https";
	bool	is_http_proxy		= proxy::manager::shared().is_http();
	auto	ret					= promise< http::message::ptr >();
	
	if ( m_connected )
	{
		if ( is_https_request && is_http_proxy )
		{
			mlog( marker::http, log::level_t::info, "pushing tls filter for %...", request->resource().to_string() );
			m_socket.push( tls::filter::create( role_t::client ) );
		}
	
		really_send_request( request, ret );
	}
	else
	{
		auto resource = request->resource();

		mlog( marker::http, log::level_t::info, "connecting to %...", resource.to_string() );
		
		connect()
		.then( [=]() mutable
		{
			if ( is_http_proxy && is_https_request )
			{
				auto tmp = std::make_shared< http::message >( method_t::connect, request->resource() );
				tmp->set_link( request );
				request = tmp;
			}
			
			mlog( marker::http, log::level_t::info, "connected to %", resource.to_string() );
			
			if ( is_https_request && !is_http_proxy )
			{
				mlog( marker::http, log::level_t::info, "pushing tls filter for %...", request->resource().to_string() );
				m_socket.push( tls::filter::create( role_t::client ) );
			}
		
			really_send_request( request, ret );
		},
		[=]( auto err ) mutable
		{
			mlog( marker::http, log::level_t::info, "connect failed % (%)", err, err.message() );
			ret.reject( err );
		} );
	}
	
	return ret;
}
	
	
void
http::connection::really_send_request( message::ptr &message, promise< message::ptr > ret )
{
	auto os = std::make_shared< ostream >( this );
	
	message->preflight();
	message->prologue( *os );
	message->header( *os );
	
	message->body( *os )
	.then( [=]() mutable
	{
		mlog( marker::http, log::level_t::info, "sending %", std::string( pbase(), pptr() - pbase() ) );
		
		os->flush();

		m_out_message = message;
	},
	[=]( std::error_code err ) mutable
	{
		nunused( err );
	} );
	
	m_ret = ret;
}

	
promise< void >
http::connection::send_response( message &message )
{
	auto os		= std::make_shared< ostream >( this );
	auto ret	= promise< void >();
	
	message.preflight();
	
	message.prologue( *os );
	message.header( *os );
	message.body( *os )
	.then( [=]() mutable
	{
		os->flush();
		ret.resolve();
	},
	[=]( std::error_code err ) mutable
	{
		ret.reject( err );
	} );
	
	return ret;
}


nodeoze::uri
http::connection::destination() const
{
	bool is_http_proxy = proxy::manager::shared().is_http();
	
	if ( is_http_proxy )
	{
		return proxy::manager::shared().resource();
	}
	else
	{
		return nodeoze::connection::destination();
	}
}


std::streamsize
http::connection::xsputn( const char* s, std::streamsize n )
{
	return std::streambuf::xsputn( s, n );
}


int
http::connection::flush()
{
	int num = static_cast< int >( pptr() - pbase() );
	
	if ( num > 0 )
	{
		m_buffer.size( num );

		nodeoze::connection::send( std::move( m_buffer ) )
		.then( [=]() mutable
		{
		},
		[=]( auto err ) mutable
		{
			mlog( marker::http, log::level_t::error, "unable to write % bytes % (%)", num, err, err.message() );
		} );
		
		setup_buffer();
	}
	
	return num;
}


int
http::connection::overflow( int c )
{
	if ( c != EOF )
	{
		if ( flush() != EOF )
		{
			c = sputc( c );
		}
		else
		{
			c = EOF;
		}
	}
		
	return c;
}

	
int
http::connection::sync()
{
	return ( flush() != EOF ) ? 0 : -1;
}


std::error_code
http::connection::process( const buffer &buf )
{
	auto err = std::error_code();
	
	if ( m_web_socket )
	{
		server::shared().set_active_connection( this );
		
		m_web_socket_handler( buf, [=]( buffer buf, bool close ) mutable
		{
			nodeoze::connection::send( std::move( buf ) )
			.then( [=]() mutable
			{
				if ( close )
				{
					this->close();
				}
			},
			[=]( std::error_code err ) mutable
			{
				nlog( log::level_t::error, "unable to send % (%)", err, err.message() );
			} );
		} );
		
		server::shared().set_active_connection( nullptr );
	}
	else
	{
		if ( m_in_message_type == message::type_t::request )
		{
			server::shared().set_active_connection( this );
		}
		
		if ( m_reset_parser )
		{
			http_parser_init( m_parser.get(), HTTP_BOTH );
			m_reset_parser = false;
		}
		
		std::streamsize processed = http_parser_execute( m_parser.get(), m_settings.get(), reinterpret_cast< const char* >( buf.const_data() ), buf.size() );
		
		if ( !m_reset_parser && ( processed != std::streamsize( buf.size() ) ) )
		{
			nlog( log::level_t::info, "http_parser_execute() failed: bytes read = %, processed = %, state = %", buf.size(), processed, m_parser.get()->state );
			err = make_error_code( std::errc::bad_message );
		}
		
		if ( m_in_message_type == message::type_t::request )
		{
			server::shared().set_active_connection( nullptr );
		}
	}

	return err;
}


void
http::connection::close()
{
	if ( m_in_message_type == message::type_t::response )
	{
		maybe_invoke_reply( make_error_code( std::errc::not_connected ), m_in_message );
	}
	
	nodeoze::connection::close();
}


bool
http::connection::upgrade_to_tls( const message &request, server::response_f reply )
{
	http::message response( request, http::code_t::switching_protocols );

	nlog( log::level_t::debug, "upgrading http connection to tls" );

	response.add_header_field( "Connection", "Upgrade" );
	response.add_header_field( "Upgrade", "TLS/1.0, HTTP/1.1" );

	reply( response, false );
	
	m_socket.push( tls::filter::create( role_t::server ) );

	return true;
}


bool
http::connection::upgrade_to_websocket( ws::filter::type_t type, websocket_handler_f handler )
{
	auto ok = true;
	
	m_web_socket_handler	= handler;
	m_web_socket			= true;
	
	if ( m_in_message_type == message::type_t::response )
	{
		m_socket.push( ws::filter::create_client( m_resource, type ) );
	}
	else
	{
		assert( m_in_message );
		
		auto filter = ws::filter::create_server( m_in_message, type );
		ncheck_error_action( filter, ok = false, exit, "unable to create ws filter" );
		m_socket.push( filter );
	}
	
exit:
	
	return ok;
}
	

int
http::connection::message_will_begin( http_parser *parser )
{
	auto self = reinterpret_cast< connection* >( parser->data );
	assert( self );
	
	self->m_parser->upgrade = 0;
	
	if ( self->m_out_message != nullptr )
	{
		self->m_in_message = self->m_out_message->create();
	}
	else
	{
		self->m_in_message = std::make_shared< http::message >();
	}
	
	self->m_in_message->set_from( self->m_socket.peer() );
	self->m_in_message->set_type( self->m_in_message_type );
	self->m_in_message->on_begin();
	self->m_redirect.clear();
	
	return 0;
}


int
http::connection::uri_was_received( http_parser *parser, const char *buf, size_t len )
{
	nunused( parser );
	nunused( buf );
	nunused( len );
	
	assert( 0 );
	return 0;
}


int
http::connection::header_field_was_received( http_parser *parser, const char *buf, size_t len )
{
	auto self = reinterpret_cast< connection* >( parser->data );
	assert( self );
	assert( self->m_in_message );
	
	self->m_in_message->on_header_field( buf, len );

	return 0;
}


int
http::connection::header_value_was_received( http_parser *parser, const char *buf, size_t len )
{
	auto self = reinterpret_cast< connection* >( parser->data );
	assert( self );
	assert( self->m_in_message );
	
	self->m_in_message->on_header_value( buf, len );

	return 0;
}

	
int
http::connection::headers_were_received( http_parser *parser )
{
	auto self = reinterpret_cast< connection* >( parser->data );
	assert( self );
	assert( self->m_in_message );
	int ret;
	
	self->m_in_message->on_headers();

	self->m_in_message->set_major( parser->http_major );
	self->m_in_message->set_minor( parser->http_minor );
	
	self->m_in_message->set_code( static_cast< http::code_t >( parser->status_code ) );
	ret = !self->m_out_message->link() && self->m_in_message->has_body() ? 0 : 1;
	
	return ret;
}

	
int
http::connection::body_was_received( http_parser *parser, const char *data, size_t len )
{
	auto self = reinterpret_cast< connection* >( parser->data );
	assert( self );
	
//	buffer buf( ( void* ) data, len, len, []( std::uint8_t *data ) { nunused( data ); } );
//	buffer buf( data, len ); // TODO: check with Scott -- is it ok to make a copy here?

	buffer buf( ( void* ) data, len, buffer::policy::exclusive, nullptr, nullptr );
	
	if ( self->m_on_body_handler )
	{
		self->m_on_body_handler( buf ); // regarding the above TODO: -- buf is passed as non-const reference here
	}
	else
	{
		self->m_in_message->on_body( buf ); // and here
	}
	
	return 0;
}

	
int
http::connection::message_was_received( http_parser *parser )
{
	auto self = reinterpret_cast< connection* >( parser->data );
	assert( self );
	int ret = 0;
	
	mlog( marker::http, log::level_t::info, "response code %", self->m_in_message->code() );
	
#if defined( DEBUG )

	for ( auto it = self->m_in_message->header().begin(); it != self->m_in_message->header().end(); it++ )
	{
		mlog( marker::http, log::level_t::info, "%: %", it->first, it->second );
	}
	
#endif
	
	if ( ( self->m_in_message->code() == http::code_t::moved_permanently ) || ( self->m_in_message->code() == http::code_t::moved_temporarily ) )
	{
		auto location = self->m_in_message->find( "Location" );
		
		if ( ( location.size() > 0 ) && ( self->m_out_message->can_redirect() ) )
		{
			mlog( marker::http, log::level_t::info, "redirecting HTTP request to %", location );
							
			self->m_out_message->num_redirects()++;
			self->m_out_message->set_resource( location );
			self->nodeoze::connection::close();
			
			auto saved = self->m_ret;
			
			self->send_request( self->m_out_message )
			.then( [=]( message::ptr response ) mutable
			{
				saved.resolve( response );
			},
			[=]( auto err ) mutable
			{
				saved.reject( err );
			} );
			
			self->m_reset_parser = true;
		}
		else
		{
			self->maybe_invoke_reply( make_error_code( self->m_in_message->code() ), self->m_in_message );
		}
	}
	else if ( ( self->m_in_message->code() == http::code_t::proxy_authentication ) && proxy::manager::shared().auth_challenge() )
	{
		self->m_out_message->add_proxy_authorization_header_fields();
		
		auto saved = self->m_ret;
		
		self->send_request( self->m_out_message )
		.then( [=]( message::ptr response ) mutable
		{
			saved.resolve( response );
		},
		[=]( auto err ) mutable
		{
			saved.reject( err );
		} );
		
		self->m_reset_parser = true;
	}
	else if ( self->m_out_message->link() && ( self->m_in_message->code() == http::code_t::ok ) )
	{
		auto saved = self->m_ret;
		
		self->send_request( self->m_out_message->link() )
		.then( [=]( message::ptr response ) mutable
		{
			saved.resolve( response );
		},
		[=]( auto err ) mutable
		{
			saved.reject( err );
		} );
		
		self->m_reset_parser = true;
	}
	else
	{
		self->m_in_message->on_end();
		self->maybe_invoke_reply( make_error_code( self->m_in_message->code() ), self->m_in_message );
	}
	
	return ret;
}


void
http::connection::setup_buffer()
{
	assert( m_buffer.size() == 0 );
	m_buffer.size( default_buffer_size );
	setp( reinterpret_cast< char* >( m_buffer.mutable_data() ), reinterpret_cast< char* >( m_buffer.mutable_data() + m_buffer.size() ) );
}


void
http::connection::maybe_invoke_reply( std::error_code error, http::message::ptr &response )
{
	if ( !m_ret.is_finished() )
	{
		// We are going to create a temporary here that copies reply context
		//
		// This makes sure that we don't lose context if connection goes away as a result
		// of calling handler
		
		auto saved	= m_ret;
		
		if ( response && ( !error || ( ( error.category() == http::error_category() ) && ( static_cast< code_t >( error.value() ) >= http::code_t::ok ) && ( static_cast< code_t >( error.value() ) <= http::code_t::partial_content ) ) ) )
		{
			saved.resolve( response );
		}
		else
		{
			saved.reject( error );
		}
	
		runloop::shared().dispatch( [=]() mutable
		{
			// it's safe to let it go here...
			
			// reply = nullptr;
		} );
	}
}


#if defined( __APPLE__ )
#	pragma mark http::loopback implementation
#endif

http::loopback::loopback()
{
	mlog( marker::http, log::level_t::info, "opening loopback connection" );


}


http::loopback::~loopback()
{
	mlog( marker::http, log::level_t::info, "closing loopback connection" );
}


promise< void >
http::loopback::connect()
{
	auto ret = promise< void >();

	assert( 0 );

	ret.resolve();

	return ret;
}

	
promise< void >
http::loopback::send( buffer buf )
{
	auto ret = promise< void >();

	if ( m_dest )
	{
		server::shared().set_active_connection( this );
		m_dest->process( buf );
		ret.resolve();
	}
	else
	{
		ret.reject( make_error_code( std::errc::not_connected ) );
	}

	return ret;
}


bool
http::loopback::is_colocated()
{
	return true;
}
	

std::error_code
http::loopback::process( const buffer &buf )
{
	nunused( buf );

	auto ret = std::error_code();

	assert( 0 );
	
	return ret;
}


#if defined( __APPLE__ )
#	pragma mark http::server implementation
#endif

NODEOZE_DEFINE_SINGLETON( http::server )

http::server*
http::server::create()
{
	return new http::server;
}

http::server::server()
{
}


http::server::~server()
{
}


nodeoze::http::connection::ptr
nodeoze::http::server::adopt( ip::tcp::socket sock, std::function< void () > callback )
{
	auto conn = std::make_shared< http::connection >( std::move( sock ) );
	
	notification::shared().subscribe( notification::local, make_oid( conn ), connection::close_event, [=]( const any &info ) mutable
	{
		nunused( info );
	
		callback();
		auto ok = remove( conn.get() );
		assert( ok );
		return false;
	} );
	
	m_connections[ make_oid( conn ) ] = conn;
	conn->recv();
	
	return conn;
}


void
http::server::bind_request( method_t method, const std::string &path, const std::string &type, request_f r )
{
	bind( method, std::unique_ptr< server::binding >( new server::binding( path, type, r ) ) );
}


void
http::server::bind_request( method_t method, const std::string &path, const std::string &type, request_body_was_received_f rbwr, request_f r )
{
	bind( method, std::unique_ptr< server::binding >( new server::binding( path, type, rbwr, r ) ) );
}


void
http::server::bind_request( method_t method, const std::string &path, const std::string &type, request_will_begin_f rwb, request_body_was_received_f rbwr, request_f r )
{
	bind( method, std::unique_ptr< server::binding >( new server::binding( path, type, rwb, rbwr, r ) ) );
}


void
http::server::bind( method_t method, binding::ptr binding )
{
	auto it = m_bindings.find( method );
	
	if ( it == m_bindings.end() )
	{
		mlog( marker::http, log::level_t::info, "creating new binding list for %", method );
		m_bindings[ method ] = binding::list();
		it = m_bindings.find( method );
	}
	
	mlog( marker::http, log::level_t::info, "binding % to %", method, binding->m_path );
	
	it->second.emplace_back( std::move( binding ) );
	
	it->second.sort( []( const binding::ptr &lhs, const binding::ptr &rhs ) mutable
    {
        return ( lhs->m_path.size() > rhs->m_path.size() );
    } );
}


void
http::server::bind_upgrade( method_t method, const std::string &path, const std::string &type, upgrade_f u )
{
	bind( method, std::unique_ptr< server::binding >( new server::binding( path, type, u ) ) );
}


int
http::server::message_will_begin( http_parser *parser )
{
	auto conn = reinterpret_cast< connection* >( parser->data );
	assert( conn );
	
	http::connection::message_will_begin( parser );
	conn->set_data( nullptr );
	
	return 0;
}


int
http::server::uri_was_received( http_parser *parser, const char *buf, size_t len )
{
	auto conn = reinterpret_cast< connection* >( parser->data );
	assert( conn );
	assert( conn->m_in_message );
	
	conn->m_in_message->on_uri( buf, len );
	
	return 0;
}


int
http::server::header_field_was_received( http_parser *parser, const char *buf, size_t len )
{
	return http::connection::header_field_was_received( parser, buf, len );
}


int
http::server::header_value_was_received( http_parser *parser, const char *buf, size_t len )
{
	return http::connection::header_value_was_received( parser, buf, len );
}


int
http::server::headers_were_received( http_parser *parser )
{
	auto conn = reinterpret_cast< connection* >( parser->data );
	assert( conn );
	assert( conn->m_in_message );
	int ret = -1;
	
	conn->m_in_message->on_headers();
	conn->m_in_message->set_major( parser->http_major );
	conn->m_in_message->set_minor( parser->http_minor );
	
	switch ( parser->method )
	{
		case HTTP_DELETE:
		{
			conn->m_in_message->set_method( method_t::delet );
		}
		break;
				
		case HTTP_GET:
		{
			conn->m_in_message->set_method( method_t::get );
		}
		break;
				
		case HTTP_HEAD:
		{
			conn->m_in_message->set_method( method_t::head );
		}
		break;
				
		case HTTP_POST:
		{
			conn->m_in_message->set_method( method_t::post );
		}
		break;
				
		case HTTP_PUT:
		{
			conn->m_in_message->set_method( method_t::put );
		}
		break;
		
		case HTTP_OPTIONS:
		{
			conn->m_in_message->set_method( method_t::options );
		}
		break;
	}

	auto binding = shared().resolve( conn->m_in_message );

	if ( !binding )
	{
		http::message response( *conn->m_in_message, http::code_t::not_found );
		
		response.add_header_field( "Connection", "Close" );
		response.add_header_field( "Content-Type", "text/html" );
		response << "<html>Error 404: Content Not Found</html>";
		response.add_header_field( "Content-Length", static_cast< int >( response.body().size() ) );
		
		conn->send_response( response );
		
		goto exit;
	}
	
	if ( binding->m_rwb )
	{
		binding->m_rwb( conn->m_in_message );
	}

	if ( !conn->m_in_message )
	{
		http::message response( *conn->m_in_message, http::code_t::not_found );
		
		response.add_header_field( "Connection", "Close" );
		response.add_header_field( "Content-Type", "text/html" );
		response << "<html>Error 404: Content Not Found</html>";
		response.add_header_field( "Content-Length", static_cast< int >( response.body().size() ) );
		
		conn->send_response( response );
		
		goto exit;
	}

	if ( conn->m_in_message->expect() == "100-continue" )
	{
		http::message response( *conn->m_in_message, http::code_t::cont );
		
		conn->send_response( response );
	}
	
	conn->set_data( binding );

	ret = 0;

exit:

	return ret;
}


int
http::server::body_was_received( http_parser *parser, const char *data, size_t len )
{
	auto conn = reinterpret_cast< connection* >( parser->data );
	assert( conn );
	assert( conn->m_in_message );
	auto id			= conn->id();
	auto binding	= reinterpret_cast< server::binding* >( conn->data() );
// TODO: same as earlier
//	auto buf		= buffer( ( void* ) data, len, len, []( std::uint8_t *data ) { nunused( data ); } );
//	auto buf		= buffer( ( void* ) data, len );

	auto buf = buffer( ( void* ) data, len, buffer::policy::exclusive, nullptr, nullptr );

	assert( binding );
	
	return binding->m_rbwr( *conn->m_in_message, buf, [=]( http::message &response, bool close ) mutable
	{
		connection::find< http::connection >( id, [&]( connection &conn ) mutable
		{
			conn.send_response( response )
			.then( [=]() mutable
			{
				if ( close )
				{
					connection::find< http::connection >( id, [&]( http::connection &conn ) mutable
					{
						conn.close();
					} );
				}
			},
			[=]( auto err ) mutable
			{
				nunused( err );
			
				if ( close )
				{
					connection::find< http::connection >( id, [&]( http::connection &conn ) mutable
					{
						conn.close();
					} );
				}
			} );
		} );
	} );
}

	
int
http::server::message_was_received( http_parser *parser )
{
	auto conn = reinterpret_cast< connection* >( parser->data );
	assert( conn );
	assert( conn->m_in_message );
	auto id			= conn->id();
	auto binding	= reinterpret_cast< server::binding* >( conn->data() );
	assert( binding );
	int ret;
	
#if defined( DEBUG )
	
	for ( auto it = conn->m_in_message->header().begin(); it != conn->m_in_message->header().end(); it++ )
	{
		mlog( marker::http, log::level_t::info, "%: %", it->first, it->second );
	}
	
#endif
	
	if ( binding->m_u )
	{
		binding->m_u( *conn );

		shared().remove( conn );

		return -1;
	}
	else if ( binding->m_r )
	{
		conn->m_in_message->on_end();
	
		ret = binding->m_r( *conn->m_in_message, [=]( http::message &response, bool close ) mutable
		{
			mlog( marker::http, log::level_t::info, "sending back response with code % (close is %)", response.code(), close );

			connection::find< http::connection >( id, [&]( http::connection &conn ) mutable
			{
				conn.send_response( response )
				.then( [=]() mutable
				{
					if ( close )
					{
						connection::find< http::connection >( id, [&]( http::connection &conn ) mutable
						{
							mlog( marker::http, log::level_t::info, "closing connection" );

							conn.close();
						} );
					}
				},
				[=]( auto err ) mutable
				{
					mlog( marker::http, log::level_t::info, "error sending back response % (%)", err.value(), err.message() );
					
					if ( close )
					{
						connection::find< http::connection >( id, [&]( http::connection &conn ) mutable
						{
							mlog( marker::http, log::level_t::info, "closing connection" );

							conn.close();
						} );
					}
				} );
#if promise_has_finally
#endif
			} );
		} );
	}
	else
	{
		ret = -1;
	}
	
	return ret;
}


http::server::binding*
http::server::resolve( const message::ptr &request )
{
	binding *binding = nullptr;
	
	mlog( marker::http, log::level_t::info, "looking up bindings for % %", request->method(), request->resource().path() );
	
	auto it1 = m_bindings.find( request->method() );
	
	if ( it1 != m_bindings.end() )
	{
		std::string accept;

		auto it2 = request->header().find( "Content-Type" );

		if ( it2 != request->header().end() )
		{
			accept = it2->second;
		}

		for ( auto it3 = it1->second.begin(); it3 != it1->second.end(); it3++ )
		{
			mlog( marker::http, log::level_t::info, "looking at %", ( *it3 )->m_path );
			
			try
			{
				std::regex	regex1( regexify( ( *it3 )->m_path ) );
				std::regex	regex2( regexify( ( *it3 )->m_type ) );

				if ( ( ( *it3 )->m_type == "*" && std::regex_search( request->resource().path(), regex1 ) ) ||
					( std::regex_search( request->resource().path(), regex1 ) && std::regex_search( accept, regex2 ) ) )
				{
					mlog( marker::http, log::level_t::info, "found binding % (%)", ( *it3 )->m_path, ( *it3 )->m_type );
					binding = ( *it3 ).get();
					break;
				}
			}
			catch ( std::exception &exc )
			{
				nlog( log::level_t::error, "caught exception in regex: %", exc.what() );
			}
		}
	}
	
	if ( !binding )
	{
		mlog( marker::http, log::level_t::error, "unable to find binding for method % -> %", request->method(), request->resource().to_string() );
		goto exit;
	}
	
exit:

	return binding;
}


bool
http::server::remove( http::connection *conn )
{
	bool ok = false;
	
	auto it = m_connections.find( make_oid( conn ) );
	
	if ( it != m_connections.end() )
	{
		m_connections.erase( it );
		ok = true;
	}
	
	return ok;
}


std::string
http::server::regexify( const std::string &in )
{
	std::string out( in );

	replace( out, "\\", "\\\\" );
    replace( out, "^", "\\^");
	replace( out, ".", "\\.");
	replace( out, "$", "\\$");
	replace( out, "|", "\\|");
    replace( out, "(", "\\(");
    replace( out, ")", "\\)");
    replace( out, "[", "\\[");
    replace( out, "]", "\\]");
    replace( out, "*", "\\*");
    replace( out, "+", "\\+");
    replace( out, "?", "\\?");
    replace( out, "/", "\\/");
	replace( out, "\\?", ".");
    replace( out, "\\*", ".*");

	return out;
}


void
http::server::replace( std::string& str, const std::string& oldStr, const std::string& newStr)
{
	size_t pos = 0;

	while ( ( pos = str.find( oldStr, pos ) ) != std::string::npos )
	{
		str.replace(pos, oldStr.length(), newStr);
		pos += newStr.length();
	}
}


std::ostream&
http::operator<<( std::ostream &os, const nodeoze::http::message &obj )
{
	if ( obj.type() == message::type_t::request )
	{
		os << obj.method() << " " << obj.resource() << std::endl;
	}
	
	for ( auto &it : obj.header() )
	{
		os << it.first << ": " << it.second << std::endl;
	}
	
	if ( obj.find( "Content-Type" ) == mime::application::json )
	{
		os << obj.body().to_string() << std::endl;
	}
	
	return os;
}


namespace nodeoze {

namespace http {

class category : public std::error_category
{
public:

	virtual const char*
	name() const noexcept override
    {
		return "http";
    }
	
    virtual std::string
	message( int value ) const override
    {
		return to_string( static_cast< code_t >( value ) );
    }
};

const std::error_category&
error_category()
{
	static auto instance = new category;
    return *instance;
}

}

}

#endif