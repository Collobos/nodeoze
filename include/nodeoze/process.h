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
 
#ifndef _nodeoze_process_h
#define _nodeoze_process_h

#include <nodeoze/promise.h>
#include <nodeoze/scoped_operation.h>
#include <nodeoze/filesystem.h>
#include <functional>
#include <cstdint>
#include <string>
#include <sys/types.h>
#if defined( WIN32 )
#include <WinSock2.h>
#endif

namespace nodeoze {

class any;
class buffer;

class process
{
public:

#if defined( __APPLE__ ) || defined( __linux__ )
	typedef ::pid_t pid_t;
#elif defined( WIN32 )
	typedef ::DWORD pid_t;
#elif defined( __linux__ )
#endif 
	
	typedef std::function< std::string () >							input_f;
	typedef std::function< void ( const nodeoze::buffer &buf ) >	output_f;
	typedef std::vector< std::string >								env_t;
	
	/*
	 * YO: make this stream friendly
	 */

	static promise< std::pair< std::int64_t, int > >
	create( const filesystem::path &exe, const std::vector< std::string > &args, const env_t &env, const filesystem::path &working_directory, pid_t &pid, input_f stdin_handler = nullptr, output_f stdout_handler = nullptr, output_f stderr_handler = nullptr );

	static process&
	self();
	
	inline
	process( const process &rhs )
	:
		m_owner( rhs.m_owner ),
		m_pid( rhs.m_pid )
	{
	}
	
	inline
	process( process &&rhs )
	:
		m_owner( std::move( rhs.m_owner ) ),
		m_pid( rhs.m_pid )
	{
	}
	
	process( const nodeoze::any &root );
	
	inline bool
	operator==( const process &rhs ) const
	{
		return ( m_pid == rhs.m_pid );
	}
	
	inline process&
	operator=( const process &rhs )
	{
		m_owner	= rhs.m_owner;
		m_pid	= rhs.m_pid;
		
		return *this;
	}
	
	inline process&
	operator=( process &&rhs )
	{
		m_owner	= std::move( rhs.m_owner );
		m_pid	= rhs.m_pid;
		
		return *this;
	}
	
	inline const std::string&
	owner()
	{
		return m_owner;
	}

	inline std::uint64_t
	pid() const
	{
		return m_pid;
	}
	
	std::size_t
	rss() const;
	
	nodeoze::any
	to_any() const;
	
protected:

	inline
	process( const std::string &owner, std::uint64_t pid )
	:
		m_owner( owner ),
		m_pid( pid )
	{
	}

	std::string		m_owner;
	std::uint64_t	m_pid;
};


class always_running_process
{
public:

	static scoped_operation
	create( const filesystem::path &exe, const std::vector< std::string > &args, const process::env_t &env, const filesystem::path &working_directory, process::input_f stdin_handler = nullptr, process::output_f stdout_handler = nullptr, process::output_f stderr_handler = nullptr );
};

}

#endif
