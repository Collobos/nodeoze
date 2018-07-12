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

#include <nodeoze/tls.h>
#include <nodeoze/runloop.h>
#include <nodeoze/random.h>
#include <nodeoze/macros.h>
#include <nodeoze/test.h>
#include <botan_all.h>
#include <assert.h>
#include <algorithm>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <queue>

using namespace nodeoze;

class botan_adapter : public Botan::TLS::Callbacks, public Botan::Credentials_Manager
{
public:

	inline void
	tls_emit_data(const uint8_t data[], size_t size) override
	{
		// send data to tls server, e.g., using BSD sockets or boost asio
	}

	inline void
	tls_record_received(uint64_t seq_no, const uint8_t data[], size_t size) override
	{
         // process full TLS record received by tls server, e.g.,
         // by passing it to the application
	}

	inline void
	tls_alert(Botan::TLS::Alert alert) override
	{
		// handle a tls alert received from the tls server
	}

	inline bool
	tls_session_established(const Botan::TLS::Session& session) override
	{
         // the session with the tls server was established
         // return false to prevent the session from being cached, true to
         // cache the session in the configured session manager
         return false;
	}

	inline std::vector<Botan::Certificate_Store*>
	trusted_certificate_authorities(
         const std::string& type,
         const std::string& context) override
	{
         // return a list of certificates of CAs we trust for tls server certificates,
         // e.g., all the certificates in the local directory "cas"
		return { new Botan::Certificate_Store_In_Memory("cas") };
	}

	inline std::vector<Botan::X509_Certificate>
	cert_chain(
         const std::vector<std::string>& cert_key_types,
         const std::string& type,
         const std::string& context) override
	{
         // when using tls client authentication (optional), return
         // a certificate chain being sent to the tls server,
         // else an empty list
         return std::vector<Botan::X509_Certificate>();
	}

	Botan::Private_Key*
	private_key_for(const Botan::X509_Certificate& cert,
         const std::string& type,
         const std::string& context) override
	{
         // when returning a chain in cert_chain(), return the private key
         // associated with the leaf certificate here
		return nullptr;
	}

};

#if defined( __APPLE__ )
#	pragma mark tls::client implementation
#endif

class tls_client : public tls::client, public botan_adapter
{
public:

	tls_client( options o )
	:
		m_session_mgr( m_rng ),
		m_impl( *this, m_session_mgr, *this, m_policy, m_rng, Botan::TLS::Server_Information("botan.randombit.net", 443), Botan::TLS::Protocol_Version::TLS_V12 )
	{
	}

	virtual ~tls_client()
	{
	}

	inline void
	tls_emit_data( const uint8_t data[], size_t size ) override
	{
		push( buffer( data, size ) );
	}

	inline promise< void >
	really_write( buffer buf ) override
	{
		auto ret = promise< void >();

		m_impl.send( buf.const_data(), buf.size() );

		return ret;
	}

	virtual void
	really_read() override
	{
	}

	virtual void
	really_pause() override
	{
	}

private:

	Botan::AutoSeeded_RNG					m_rng;
	Botan::TLS::Session_Manager_In_Memory	m_session_mgr;
	Botan::TLS::Strict_Policy				m_policy;
	Botan::TLS::Client						m_impl;
};

tls::client::ptr
tls::client::create( options options )
{
	std::error_code err;

	auto ret = create( std::move( options ), err );

	if ( err )
	{
		throw std::system_error( err );
	}

	return ret;
}

tls::client::ptr
tls::client::create( options options, std::error_code &err )
{
	auto ret = tls::client::ptr();

	try
	{
		ret = std::make_shared< tls_client >( std::move( options ) );
	}
	catch( Botan::PRNG_Unseeded &exc )
	{
		std::cerr << "caught unseeded exc" << std::endl;
	}
	catch ( std::exception &exc )
	{
		std::cerr << "caught exception: " << exc.what() << std::endl;
	}

	return ret;
}

tls::client::~client()
{
}

#if defined( __APPLE__ )
#	pragma mark tls::server implementation
#endif

class tls_server : public tls::server, public botan_adapter
{
public:

	tls_server( options o )
	:
		m_session_mgr( m_rng ),
		m_impl( *this, m_session_mgr, *this, m_policy, m_rng )
	{
	}

	virtual ~tls_server()
	{
	}

	inline void
	tls_emit_data( const uint8_t data[], size_t size ) override
	{
		push( buffer( data, size ) );
	}

	virtual promise< void >
	really_write( buffer buf ) override
	{
		auto ret = promise< void >();

		m_impl.send( buf.const_data(), buf.size() );

		return ret;
	}

	virtual void
	really_read() override
	{
	}

	virtual void
	really_pause() override
	{
	}

private:

	Botan::AutoSeeded_RNG					m_rng;
	Botan::TLS::Session_Manager_In_Memory	m_session_mgr;
	Botan::TLS::Strict_Policy				m_policy;
	Botan::TLS::Server						m_impl;
};

tls::server::ptr
tls::server::create( options options )
{
	std::error_code err;

	auto ret = create( std::move( options ), err );

	if ( err )
	{
		throw std::system_error( err );
	}

	return ret;
}


tls::server::ptr
tls::server::create( options options, std::error_code &err )
{
	return std::make_shared< tls_server >( std::move( options ) );
}

tls::server::~server()
{
}