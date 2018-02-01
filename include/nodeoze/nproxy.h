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
 
#ifndef _nodeoze_proxy_h
#define _nodeoze_proxy_h

#include <nodeoze/nsingleton.h>
#include <nodeoze/nendpoint.h>
#include <nodeoze/nbuffer.h>
#include <nodeoze/nstream.h>
#include <nodeoze/nuri.h>
#include <algorithm>
#include <vector>
#include <string>
#include <queue>

namespace nodeoze {

namespace proxy {

class manager
{
	NODEOZE_DECLARE_SINGLETON( manager )
	
public:

	static const std::string			was_changed_event;
	
	typedef std::function< bool () >	auth_challenge_f;

	static void
	on_auth_challenge( auth_challenge_f handler );

	static bool
	auth_challenge();

	manager();
	
	virtual ~manager();
	
	stream::filter*
	create_filter( const nodeoze::ip::endpoint &to );

	bool
	is_null() const;
	
	bool
	is_http() const;
	
	bool
	is_socks() const;
	
	inline uri&
	resource()
	{
		return m_resource;
	}
	
	inline const uri&
	resource() const
	{
		return m_resource;
	}
	
	void
	set_resource( const uri &resource );
	
	void
	clear();

	inline void
	bypass_add( const std::string &val )
	{
		auto it = std::find( m_bypass_list.begin(), m_bypass_list.end(), val );

		if ( it == m_bypass_list.end() )
		{
			m_bypass_list.push_back( val );
		}
	}

	inline const std::vector< std::string >&
	bypass_list() const
	{
		return m_bypass_list;
	}

	inline void
	bypass_remove( const std::string &val )
	{
		auto it = std::find( m_bypass_list.begin(), m_bypass_list.end(), val );

		if ( it != m_bypass_list.end() )
		{
			m_bypass_list.erase( it );
		}
	}

	bool
	bypass( const nodeoze::uri &uri );

	void
	encode_authorization( const std::string &username, const std::string &password );

	inline const std::string&
	authorization() const
	{
		return m_authorization;
	}

	void
	decode_authorization( std::string &username, std::string &password ) const;
	
protected:

	uri							m_resource;
	std::vector< std::string > 	m_bypass_list;
	std::string 				m_authorization;
	static auth_challenge_f		m_auth_challenge_handler;
};

}

}

#endif
