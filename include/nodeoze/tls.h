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
 
#ifndef _nodeoze_tls_h
#define _nodeoze_tls_h

#include <nodeoze/stream.h>
#include <nodeoze/stream2.h>
#include <nodeoze/buffer.h>
#include <nodeoze/types.h>

namespace nodeoze {

namespace tls {

class client : public stream::duplex
{
public:

	class options
	{
	public:

		options()
		{
		}
	};

	using ptr = std::shared_ptr< client >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );

	virtual ~client() = 0;
};

class server : public stream::duplex
{
public:

	class options
	{
	public:

		options()
		{
		}

		inline const buffer&
		key() const
		{
			return m_key;
		}

		inline options&
		key( buffer key )
		{
			m_key = std::move( key );
			return *this;
		}

		inline const buffer&
		cert() const
		{
			return m_cert;
		}

		inline options&
		cert( buffer cert ) 
		{
			m_cert = std::move( cert );
			return *this;
		}

		inline const std::vector< buffer >&
		intermediates() const
		{
			return m_intermediates;
		}

		inline options&
		intermediates( std::vector< buffer > intermediates )
		{
			m_intermediates = std::move( intermediates );
			return *this;
		}

	private:

		buffer					m_key;
		buffer					m_cert;
		std::vector< buffer >	m_intermediates;
	};

	using ptr = std::shared_ptr< server >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );

	virtual ~server() = 0;
};

extern std::error_code
use_server_cert( const std::vector< nodeoze::buffer > &chain, const nodeoze::buffer &key );

class filter : public stream::filter
{
public:

	static filter*
	create( role_t r );
	
	virtual ~filter();
};

}

}

#endif
