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
 
#ifndef _nodeoze_uri_h
#define _nodeoze_uri_h

#include <cstdint>
#include <string>
#include <unordered_map>

namespace nodeoze {

class uri
{
public:

	typedef void ( uri::*safe_bool_type )();
	
	uri();
	
	uri( const std::string &scheme, const std::string& host, std::uint16_t port );

	uri( const std::string &scheme, const std::string& host, std::uint16_t port, const std::string &path = std::string() );
	
	uri( const char *s );
	
	uri( const std::string& s );

	~uri();
	
	static std::string
    escape( const std::string &s );

	inline const std::string&
	scheme() const
	{
		return m_scheme;
	}
	
	inline void
	set_scheme( const std::string &val )
	{
		m_scheme = val;
	}
	
	inline const std::string&
	username() const
	{
		return m_username;
	}
	
	inline void
	set_username( const std::string &val )
	{
		m_username = val;
	}
	
	inline const std::string&
	password() const
	{
		return m_password;
	}
	
	inline void
	set_password( const std::string &val )
	{
		m_password = val;
	}

	inline const std::string&
	host() const
	{
		return m_host;
	}
	
	inline void
	set_host( const std::string &val )
	{
		m_host = val;
	}

	inline int
	port() const
	{
		return m_port;
	}

	inline void
	set_port( int val )
	{
		m_port = val;
	}

	inline const std::string&
	path() const
	{
		return m_path;
	}

	inline void
	set_path( const std::string &val )
	{
		m_path = val;
	}

	inline const std::string&
	query() const
	{
		return m_query;
	}
	
	inline void
	set_query( const std::string &val )
	{
		m_query = val;
	}
	
	inline uri&
	add_query( const std::string &val )
	{
		if ( m_query.size() > 0 )
		{
			m_query += "&" + val;
		}
		else
		{
			m_query += val;
		}
		
		return *this;
	}
	
	inline const std::unordered_map< std::string, std::string >&
	parameters() const
	{
		return m_parameters;
	}

	bool
	assign( const std::string &s );
	
	inline uri&
	operator=( const std::string &s )
	{
		assign( s );
		return *this;
	}
	
	inline bool
	operator==( const uri &uri )
	{
		return equals( uri );
	}
	
	inline bool
	operator!=( const uri &uri )
	{
		return !equals( uri );
	}
	
	void
	clear();
	
	static std::string
	encode( const std::string &str );
	
	static std::string
	decode( const std::string& str );

	std::string
	to_string() const;
	
	bool
	equals( const uri &rhs ) const;
	
	bool
	wants_security() const;
	
	operator safe_bool_type () const
	{
		return ( ( m_scheme.size() > 0 ) || ( m_host.size() > 0 ) || ( m_path.size() > 0 ) ) ? &uri::safe_bool_func : 0;
	}
	
private:

	inline void
	safe_bool_func()
	{
	}

	std::string										m_scheme;
	std::string										m_username;
	std::string										m_password;
	std::string										m_host;
	int												m_port		= 0;
	std::string										m_path;
	std::string										m_query;
	std::unordered_map< std::string, std::string >	m_parameters;
};


inline bool
operator==( const nodeoze::uri &u1, const nodeoze::uri &u2 )
{
	return ( u1.equals( u2 ) );
}


inline bool
operator!=( const nodeoze::uri &u1, const nodeoze::uri &u2 )
{
	return ( !u1.equals( u2 ) );
}

inline std::ostream&
operator<<( std::ostream &os, const nodeoze::uri &resource )
{
	return os << resource.to_string();
}

}

namespace std {

template <>
struct hash< nodeoze::uri >
{
	typedef nodeoze::uri	argument_type;
	typedef std::size_t		result_type;
 
	result_type operator()( const argument_type &v ) const
	{
		return std::hash< std::string >()( v.to_string() );
    }
};

}

#endif
