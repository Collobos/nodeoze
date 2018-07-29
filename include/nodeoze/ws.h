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
 
#ifndef _nodeoze_ws_h
#define _nodeoze_ws_h

#include <nodeoze/stream2.h>
#include <nodeoze/uri.h>
#include <nodeoze/buffer.h>
#include <nodeoze/types.h>
#include <queue>

namespace nodeoze {

namespace http {

class message;

}

namespace ws {

class client 
{
public:

	class options
	{
	public:

		options( std::vector< std::string > sub_protocols )
		:
			m_sub_protocols( std::move( sub_protocols ) )
		{
		}

	private:

		std::vector< std::string > m_sub_protocols;
	};

	using ptr = std::shared_ptr< client >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );

	virtual ~client() = 0;
};

class server
{
public:

	class options
	{
	public:

		options()
		{
		}

	private:

		std::vector< std::string >	m_extensions;
	};

	using ptr = std::shared_ptr< server >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );

	virtual ~server();
};

class filter : public stream::filter
{
public:

	enum class type_t
	{
		text	= 0,
		binary	= 1
	};

	static filter*
	create_client( const uri &resource, type_t type = type_t::text );

	static filter*
	create_server( type_t type = type_t::text );

	static filter*
	create_server( std::shared_ptr< http::message > request, type_t type = type_t::text );
	
	filter();
	
	virtual ~filter();

	virtual void
	ping( buffer &out_buf ) = 0;
	
	virtual void
	close( buffer &out_buf ) = 0;
};

}

}

#endif
