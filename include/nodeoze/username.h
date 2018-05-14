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

#ifndef _nodeoze_username_h
#define _nodeoze_username_h

#include <nodeoze/string.h>
#include <nodeoze/unicode.h>

namespace nodeoze {

class username
{
public:

	username()
	{
	}

	username( const std::string &name )
	{
		assign( name, std::string() );
	}
	
	username( const std::string &name, const std::string &domain )
	{
		assign( name, domain );
	}

	~username()
	{
	}

	inline std::string
	to_string() const
	{
		std::string fqdn( m_name );

		if ( ( m_domains.size() > 0 ) && ( m_domains[ 0 ].size() > 1 ) )
		{
			fqdn += "@" + m_domains[ 0 ];
		}

		return fqdn;
	}

	inline const std::string&
	name() const
	{
		return m_name;
	}

	inline const std::vector< std::string >&
	domains() const
	{
		return m_domains;
	}

	inline username&
	operator=( const std::string &rhs )
	{
		assign( rhs, std::string() );
		return *this;
	}

	inline username&
	operator=( const username &rhs )
	{
		m_name		= rhs.m_name;
		m_domains	= rhs.m_domains;

		return *this;
	}

	template < class F >
	inline bool
	find( F functor )
	{
		auto found = false;

		for ( auto &domain : m_domains )
		{
			found = functor( domain );

			if ( found )
			{
				break;
			}
		}

		return found;
	}

	// USER@DOMAIN

	inline std::string
	to_upn( const std::string &domain ) const
	{
		std::string val( m_name );

		if ( domain.size() > 1 )
		{
			val += std::string( "@" ) + domain;
		}

		return val;
	}

	// DOMAIN\USER

	inline std::string
	to_dln( const std::string &domain ) const
	{
		std::string val;

		if ( domain.size() > 1 )
		{
			val = domain + "\\" + m_name;
		}
		else
		{
			val = m_name;
		}

		return val;
	}

private:

	void
	assign( const std::string &name, const std::string &domain );

	std::string					m_name;
	std::vector< std::string >	m_domains;
};

}


#endif
