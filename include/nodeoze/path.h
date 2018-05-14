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
 
#ifndef _nodeoze_path_h
#define _nodeoze_path_h

#include <nodeoze/string.h>
#include <nodeoze/unicode.h>
#include <string>
#include <vector>

namespace nodeoze {

class path
{
public:

	typedef std::vector< std::string > components;
	typedef void ( path::*safe_bool_type )();
	
	static path
	self();
	
	static path
	home();
	
	static path
	app_data();

	static path
	system_drive();

	static path
	system();
	
	static path
	tmp();

	static std::string
	quote( const path &p )
	{
		auto s = p.to_string();

		s.insert( 0, "\"" );
		s.append( "\"" );

		return s;
	}
	
	inline
	path()
	{
	}
	
	inline
	path( const char *s )
	:
		m_path( s )
	{
	}

	inline
	path( const std::string &s )
	:
		m_path( s )
	{
	}
	
	inline
	path( std::string &&s )
	:
		m_path( s )
	{
	}

	inline
	path( const path &rhs )
	:
		m_path( rhs.m_path )
	{
	}
	
	inline
	path( path &&rhs )
	:
		m_path( std::move( rhs.m_path ) )
	{
	}
	
	inline path&
	operator=( const std::string &rhs )
	{
		m_path = rhs;
		return *this;
	}
	
	inline path&
	operator=( std::string &&rhs )
	{
		m_path = rhs;
		return *this;
	}
	
	inline path&
	operator=( const path &rhs )
	{
		m_path = rhs.m_path;
		return *this;
	}
	
	inline path&
	operator=( path &&rhs )
	{
		m_path = std::move( rhs.m_path );
		return *this;
	}
	
	inline path&
	operator+=( const path &rhs )
	{
		append( rhs );
		return *this;
	}
	
	inline bool
	operator==( const std::string &rhs ) const
	{
		return ( m_path == rhs );
	}
	
	inline bool
	operator==( const path &rhs ) const
	{
		return ( m_path == rhs.m_path );
	}
	
	inline bool
	operator!=( const std::string &rhs ) const
	{
		return ( m_path != rhs );
	}
	
	inline bool
	operator!=( const path &rhs ) const
	{
		return ( m_path != rhs.m_path );
	}
	
	path
	parent_folder() const;
	
	std::string
	extension() const;
	
	std::string
	filename() const;
	
	std::vector< path >
	children() const;
	
	path&
	append( const path &p );
	
	components
	split() const;

	inline std::size_t
	size() const
	{
		return m_path.size();
	}
	
	inline const std::string&
	to_string() const
	{
		return m_path;
	}
	
	inline explicit operator bool () const
	{
		return ( m_path.empty() ) ? false : true;
	}

protected:

	std::string m_path;
};

inline path
operator+( const path& lhs, const path& rhs )
{
	path p( lhs );
	
	p.append( rhs );
	
	return p;
}

inline std::ostream&
operator<<( std::ostream &os, const nodeoze::path &path )
{
	os << path.to_string();
	return os;
}

}

#endif
