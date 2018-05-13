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
 
#ifndef _nodeoze_msgpack_h
#define _nodeoze_msgpack_h

#include <nodeoze/any.h>
#include <nodeoze/rpc.h>


namespace nodeoze {

namespace mpack {

nodeoze::buffer
deflate( const any &root );

any
inflate( const std::string &s );

std::error_code
inflate( any &root, const std::string &s );

any
inflate( const buffer& buf );

std::error_code
inflate( any& root, const buffer& buf );

void
link();

class parser
{
public:

	typedef std::function< void ( any &root ) > object_f;

	parser( any &root );
	
	~parser();
	
	inline void
	on_root_object( object_f func )
	{
		m_on_root_object = func;
	}
	
	std::error_code
	process( const std::uint8_t *buf, std::size_t len );
	
	any&
	root();
	
	const any&
	root() const;
	
protected:

	object_f		m_on_root_object;
	nodeoze::any	&m_root;
};

namespace rpc {

class connection : public nodeoze::rpc::connection
{
public:

	typedef std::shared_ptr< connection > ptr;
	
	connection( const uri &resource );

	connection( ip::tcp::socket sock );

	connection( http::connection::ptr wrapped );
	
	virtual ~connection();

	virtual nodeoze::buffer
	deflate( const nodeoze::any &message );

protected:

	virtual std::error_code
	process( const buffer &buf );
	
	any		m_root;
	parser	m_parser;
};

}

}

}

#endif
