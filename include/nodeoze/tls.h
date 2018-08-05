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

enum class errc
{
	ok						= 0,
	invalid_argument		= 1,
	unsupported_argument	= 2,
	invalid_state 			= 3,
	lookup_error			= 4,
	internal_error 			= 5,
	invalid_key_length		= 6,
	invalid_iv_length		= 7,
	prng_unseeded			= 8,
	policy_violation		= 9,
	algorithm_not_found		= 10,
	no_provider_found		= 11,
	provider_not_found		= 12,
	invalid_algorithm_name	= 13,
	encoding_error			= 14,
	decoding_error			= 15,
	integrity_failure		= 16,
	invalid_oid				= 17,
	stream_io_error			= 18,
	self_test_failure		= 19,
	not_implemented 		= 20,
	unknown                 = 21
};

std::error_code
create_self_signed_cert( const std::string &common_name, const std::string &country, const std::string &organization, const std::string &email, std::string &pem_key, std::string &pem_cert );

class private_key
{
public:

	private_key( std::string pem )
	:
		m_pem( pem )
	{
	}

	private_key( filesystem::path path )
	{
	}

	inline const std::string&
	pem() const
	{
		return m_pem;
	}

private:

	std::string m_pem;
};

class certs
{
public:

	certs( std::string pem )
	:
		m_pem( std::move( pem ) )
	{
	}

	certs( filesystem::path path )
	{
	}

	inline const std::string&
	pem() const
	{
		return m_pem;
	}

private:

	std::string m_pem;
};

class client : public stream::dual
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
};

class server : public stream::dual
{
public:

	class options
	{
	public:

		options()
		{
		}

		options( const private_key &key, const certs &certs )
		:
			m_key( key.pem() ),
			m_certs( certs.pem() )
		{
		}

		inline const std::string&
		key() const
		{
			return m_key;
		}

		inline const std::string&
		certs() const
		{
			return m_certs;
		}

	private:

		std::string				m_key;
		std::string				m_certs;
	};

	using ptr = std::shared_ptr< server >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );
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

const std::error_category&
error_category();
	
inline std::error_code
make_error_code( errc val )
{
	return std::error_code( static_cast< int >( val ), error_category() );
}

}

}

#endif
