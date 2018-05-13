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
 
#include <nodeoze/uri.h>
#include <nodeoze/connection.h>
#include <nodeoze/string.h>
#include <uriparser/Uri.h>
#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include <cctype>


typedef std::vector< std::string > strings;

static strings
split( const std::string &str, const char &delim )
{
	strings						tokens;
	std::string::const_iterator pos		= str.begin();
	std::string::const_iterator last	= str.begin();
	std::string					lastToken;

	while ( pos != str.end() )
	{
		last = pos;
		
        pos = find( pos, str.end(), delim );

		if ( pos != str.end() )
		{
			std::string token = std::string( last, pos );
			
			if ( token.length() > 0 )
			{
                tokens.push_back(token);
			}

			last = ++pos;
        }
    }

	lastToken = std::string( last, pos );
	
	if ( lastToken.length() > 0 )
	{
        tokens.push_back( lastToken );
	}

    return tokens;
}


using namespace nodeoze;


uri::uri()
{
}


uri::uri( const std::string &scheme, const std::string& host, std::uint16_t port )
:
	m_scheme( scheme ),
	m_host( host ),
	m_port( port )
{
}


uri::uri( const std::string &scheme, const std::string& host, std::uint16_t port, const std::string &path )
:
	m_scheme( scheme ),
	m_host( host ),
	m_port( port ),
	m_path( path )
{
}


uri::uri( const char *s )
:
	m_port( 0 )
{
	assign( s );
}


uri::uri( const std::string& s )
:
	m_port( 0 )
{
	assign( s );
}


uri::~uri()
{
}


std::string
uri::escape( const std::string &in )
{
    char        out[ 1024 ];
    const char  *term;
    std::string ret;

    term = uriEscapeA( in.c_str(), out, false, false );
    ret = ( term != NULL ) ? out : in;

    return ret;
}


bool
uri::assign( const std::string& s )
{
	UriParserStateA state;
	std::string		userinfo;
	UriUriA			uri;
	bool			ok		= false;
	
	clear();
	
	memset( &state, 0, sizeof( state ) );
	memset( &uri, 0, sizeof( uri ) );

	if ( s.size() > 0 )
	{
		state.uri = &uri;
		
		if ( uriParseUriA( &state, s.c_str() ) != URI_SUCCESS )
		{
			goto exit;
		}
		
		m_scheme.assign( uri.scheme.first, uri.scheme.afterLast - uri.scheme.first );
		m_scheme = decode( m_scheme );
		
		userinfo.assign( uri.userInfo.first, uri.userInfo.afterLast - uri.userInfo.first );
		
		m_username.clear();
		m_password.clear();
		
		if ( userinfo.size() > 0 )
		{
			auto vec = split( userinfo, ':' );
			
			m_username = vec[ 0 ];
			
			if ( vec.size() == 2 )
			{
				m_password = vec[ 1 ];
			}
		}
		
		m_host.assign( uri.hostText.first, uri.hostText.afterLast - uri.hostText.first );
		m_host = decode( m_host );

		if ( uri.portText.first )
		{
			std::string text;
			
			text.assign( uri.portText.first, uri.portText.afterLast - uri.portText.first );
			m_port = atoi( text.c_str() );
		}
		else if ( ( m_scheme == "http" ) || ( m_scheme == "ws" ) )
		{
			m_port = 80;
		}
		else if ( ( m_scheme == "https" ) || ( m_scheme == "wss" ) )
		{
			m_port = 443;
		}
		
		for ( UriPathSegmentA* path = uri.pathHead; path != NULL; path = path->next )
		{
			std::string temp;
			
			temp.assign( path->text.first, path->text.afterLast - path->text.first );
			temp = decode( temp );
			m_path += "/" + temp;
		}
		
		if ( m_path.size() == 0 )
		{
			m_path = "/";
		}
		
		m_query.assign( uri.query.first, uri.query.afterLast - uri.query.first );
		
		if ( m_query.size() > 0 )
		{
			strings params = split( query(), '&' );
			for ( auto it = params.begin(); it != params.end(); it++ )
			{
				strings param = split( *it, '=' );

				if ( param.size() == 2 )
				{
					m_parameters[ param[ 0 ] ] = param[ 1 ];
				}
			}
		}
	}
	
	ok = true;
	
exit:

	uriFreeUriMembersA( &uri );
	
	return ok;
}


void
uri::clear()
{
	m_scheme.clear();
	m_username.clear();
	m_password.clear();
	m_host.clear();
	m_port = 0;
	m_path.clear();
	m_query.clear();
	m_parameters.clear();
}


std::string
uri::encode( const std::string & str )
{
	const char	*last;
	char		*buf = new char[ ( str.size() * 3 ) + 1 ];
	std::string	ret;
	
	last = uriEscapeA( str.c_str(), buf, false, false );
	ret.assign( buf, last - buf );
	delete [] buf;
	
	return ret;
}


std::string
uri::decode( const std::string & str )
{
	std::string	dummy( str );
	std::string	ret;
	const char		*last;
	
	last = uriUnescapeInPlaceA( ( char* ) dummy.c_str() );
	ret.assign( dummy.c_str(), last );
	
	return ret;
}


std::string
uri::to_string() const
{
	std::string		ret;
	char			*str	= NULL;
	UriUriStructA	uri;
	
	memset( &uri, 0, sizeof( UriUriStructA ) );
	
	if ( *this )
	{
		std::string		port;
		std::string		userinfo;
		strings			components;
		strings			encoded_components;
		int				len;
		
		uri.scheme.first		= m_scheme.c_str();
		uri.scheme.afterLast	= m_scheme.c_str() + m_scheme.size();
		
		if ( m_username.size() > 0 )
		{
			userinfo = m_username;
			
			if ( m_password.size() > 0 )
			{
				userinfo.push_back( ':' );
				userinfo += m_password;
			}
		}
		
		if ( userinfo.size() > 0 )
		{
			uri.userInfo.first	= userinfo.c_str();
			uri.userInfo.afterLast = userinfo.c_str() + userinfo.size();
		}
		
		auto host = encode( m_host );
		
		uri.hostText.first		= host.c_str();
		uri.hostText.afterLast	= host.c_str() + host.size();
		
		if ( m_port != 0 )
		{
			port = std::to_string( m_port );

			uri.portText.first		= port.c_str();
			uri.portText.afterLast	= port.c_str() + port.size();
		}

		if ( m_path != "/" )
		{
			components = split( m_path, '/' );
		
			if ( components.size() > 0 )
			{
				UriPathSegmentA **path = &uri.pathHead;
				int index = 0;

				encoded_components.resize( components.size() );
			
				for ( auto it = components.begin(); it != components.end(); it++ )
				{
					encoded_components[ index ] = encode( *it );

					*path = ( UriPathSegmentA* ) malloc( sizeof ( UriPathSegmentA ) );
					( *path )->text.first = encoded_components[ index ].c_str();
					( *path )->text.afterLast = encoded_components[ index ].c_str() + encoded_components[ index ].size();
					( *path )->next = NULL;
				
					path = &( *path )->next;
				
					index++;
				}
			}
		}

		if ( !m_query.empty() )
		{
			uri.query.first			= m_query.c_str();
			uri.query.afterLast		= m_query.c_str() + m_query.size();
		}

		if ( uriToStringCharsRequiredA( &uri, &len ) != URI_SUCCESS )
		{
			goto exit;
		}
		
		len++;

		str = new char[ len ];
		
		if ( !str )
		{
			goto exit;
		}
		
		if ( uriToStringA( str, &uri, len, NULL ) != URI_SUCCESS )
		{
			goto exit;
		}
		
		ret.assign( str );
	}
	
exit:

	uriFreeUriMembersA( &uri );
	
	if ( str )
	{
		delete [] str;
	}
	
	if ( ret == "://" )
	{
		ret = "";
	}
	
	return ret;
}


bool
uri::equals( const uri &rhs ) const
{
	return ( ( m_scheme == rhs.m_scheme ) &&
			 ( m_username == rhs.m_username ) &&
			 ( m_password == rhs.m_password ) &&
	         ( m_host == rhs.m_host ) &&
			 ( m_port == rhs.m_port ) &&
			 ( m_path == rhs.m_path ) &&
			 ( m_query == rhs.m_query ) );
}


bool
uri::wants_security() const
{
	return connection::factory::wants_security( *this );
}
