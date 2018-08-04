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


template<typename T>
const char* getClassName(T) {
  return typeid(T).name();
}

#define CATCH_BOTAN_EXCEPTION																    \
catch ( Botan::Invalid_Argument& ) { err = make_error_code( tls::errc::invalid_argument ); }    \
catch ( std::exception& exc ) { fprintf( stderr, "yo: %s %s\n", exc.what(), getClassName( exc ) ); err = make_error_code( tls::errc::unknown ); }

std::error_code
tls::create_self_signed_cert( const std::string &common_name, const std::string &country, const std::string &organization, const std::string &email, std::string &pem_key, std::string &pem_cert )
{
	Botan::X509_Cert_Options	opts;
	std::error_code				err;

	opts.common_name 	= common_name;
	opts.country		= country;
	opts.organization	= organization;
	opts.email			= email;

	try
	{
fprintf( stderr, "creating self signed cert...\n" );
		Botan::AutoSeeded_RNG	rng;
		Botan::RSA_PrivateKey	priv_key( rng, 2048 );
		Botan::X509_Certificate cert = Botan::X509::create_self_signed_cert( opts, priv_key, "SHA-256", rng );

		pem_key		= Botan::PKCS8::PEM_encode( priv_key, rng, std::string() );
		pem_cert	= cert.PEM_encode();
fprintf( stderr, "created self signed cert\n" );
	}
	CATCH_BOTAN_EXCEPTION

	return err;
}

template< class T >
class botan_adapter : public Botan::TLS::Callbacks, public Botan::Credentials_Manager
{
protected:

	botan_adapter( T *stream )
	:
		m_stream( stream ),
		m_session_mgr( m_rng )
	{
	}

	inline void
	tls_emit_data(const uint8_t data[], size_t size) override
	{
		// send data to tls server, e.g., using BSD sockets or boost asio
		fprintf( stderr, "%s: pushing data of size %d\n", name(), size );

		if ( !m_paused )
		{
fprintf( stderr, "%s: pushing data\n", name() );
			//m_stream->push( buffer( data, size ) );
		}
		else
		{
		fprintf( stderr, "%s: 0x%x queuing data of size %d 0x%x (%d)\n", name(), this, size, data, m_read_queue.size() );

			m_read_queue.emplace_back( data, size );
		}
	}

	inline void
	tls_record_received( uint64_t seq_no, const uint8_t data[], size_t size ) override
	{
		fprintf( stderr, "%s: record received\n", name() );
		 // process full TLS record received by tls server, e.g.,
		 // by passing it to the application
	}

	inline void
	tls_alert( Botan::TLS::Alert alert ) override
	{
		// handle a tls alert received from the tls server
	}

	inline bool
	tls_session_established(const Botan::TLS::Session& session) override
	{
		fprintf( stderr, "%s: session established\n", name() );
		 // the session with the tls server was established
		 // return false to prevent the session from being cached, true to
		 // cache the session in the configured session manager
		 return false;
	}

	inline virtual void
	tls_session_activated() override
	{
		fprintf( stderr, "%s: session activated\n", name() );
	}

	inline std::vector<Botan::Certificate_Store*>
	trusted_certificate_authorities( const std::string& type, const std::string& context ) override
	{
		fprintf( stderr, "%s: in trusted certificate authorities: %s\n", name(), type.c_str() );
		 // return a list of certificates of CAs we trust for tls server certificates,
		 // e.g., all the certificates in the local directory "cas"
		return { new Botan::Certificate_Store_In_Memory("cas") };
	}

	inline std::vector<Botan::X509_Certificate>
	cert_chain( const std::vector<std::string>& cert_key_types, const std::string& type, const std::string& context) override
	{
		for ( auto &type : cert_key_types )
		{
			fprintf( stderr, "cert key type: %s\n", type.c_str() );
		}

		fprintf( stderr, "cert chain: %s\n", type.c_str() );
		 // when using tls client authentication (optional), return
		 // a certificate chain being sent to the tls server,
		 // else an empty list

		 if ( m_creds.size() > 0 )
		 {
			 return m_creds[ 0 ].certs;
		 }
		 else
		 {
		 	return std::vector<Botan::X509_Certificate>();
		 }
	}

	Botan::Private_Key*
	private_key_for( const Botan::X509_Certificate& cert, const std::string& type, const std::string& context ) override
	{
		fprintf( stderr, "private key for\n" );
		 // when returning a chain in cert_chain(), return the private key
		 // associated with the leaf certificate here

		if ( m_creds.size() > 0 )
		{
			return m_creds[ 0 ].key.get();
		}
		else
		{
			return nullptr;
		}
	}

	inline virtual void
	tls_verify_cert_chain( const std::vector< Botan::X509_Certificate>& cert_chain, const std::vector< std::shared_ptr< const Botan::OCSP::Response>>& ocsp_responses, const std::vector< Botan::Certificate_Store* > &trusted_roots,  Botan::Usage_Type usage, const std::string &hostname, const Botan::TLS::Policy &policy ) override
	{
		nunused( cert_chain );
		nunused( ocsp_responses );
		nunused( trusted_roots );
		nunused( usage );
		nunused( policy );

		fprintf( stderr, "in tls verify cert chain: %s\n", hostname.c_str() );
	}

	virtual const char*
	name() const = 0;

/*
	inline void
	set_paused( bool val )
	{
		m_paused = val;

		if ( !m_paused )
		{
			for ( auto &buf : m_read_queue )
			{
				m_stream->push( buf );
			}

			m_read_queue.clear();
		}
	}
*/

	struct Certificate_Info
	{
		std::vector< Botan::X509_Certificate >	certs;
		std::shared_ptr< Botan::Private_Key >	key;
	};

	T															*m_stream;
	std::vector< Certificate_Info >								m_creds;
	std::vector< std::shared_ptr< Botan::Certificate_Store > >	m_certstores;
	Botan::AutoSeeded_RNG										m_rng;
	Botan::TLS::Session_Manager_In_Memory						m_session_mgr;

private:

	bool                                    					m_paused = true;
	deque< buffer >												m_read_queue;
};

#if defined( __APPLE__ )
#	pragma mark tls::client implementation
#endif

class my_policy : public Botan::TLS::Strict_Policy
{
public:

	virtual std::vector< std::string >
	allowed_signature_methods() const override
	{
		return { "RSA" };
	}
};

class tls_client : public tls::client, public botan_adapter< tls_client >
{
public:

	class top_stream : public stream::duplex
	{
	public:

		top_stream( tls_client *parent )
		:
			m_parent( parent )
		{
		}

		inline promise< void >
		really_write( buffer buf ) override
		{
			return m_parent->top_really_write( std::move( buf ) );
		}

		virtual void
		really_read() override
		{
			return m_parent->top_really_read();
		}

		virtual void
		really_pause() override
		{
			return m_parent->top_really_pause();
		}

	private:

		tls_client *m_parent;
	};

	class bottom_stream : public stream::duplex
	{
	public:

		bottom_stream( tls_client *parent )
		:
			m_parent( parent )
		{
		}

		~bottom_stream()
		{
		}

		inline promise< void >
		really_write( buffer buf ) override
		{
			return m_parent->bottom_really_write( std::move( buf ) );
		}

		virtual void
		really_read() override
		{
			return m_parent->bottom_really_read();
		}

		virtual void
		really_pause() override
		{
			return m_parent->bottom_really_pause();
		}

	private:

		tls_client *m_parent;
	};

	tls_client( options o )
	:
		botan_adapter( this ),
		m_impl( *this, m_session_mgr, *this, m_policy, m_rng, Botan::TLS::Server_Information( "nodeoze", 443 ), Botan::TLS::Protocol_Version::TLS_V12 )
	{
		for ( auto &cipher : m_policy.allowed_ciphers() )
		{
			fprintf( stderr, "allowed cipher: %s\n", cipher.c_str() );
		}

		for ( auto &cipher : m_policy.allowed_signature_methods() )
		{
			fprintf( stderr, "allowed sig method: %s\n", cipher.c_str() );
		}

		m_top		= std::make_shared< top_stream >( this );
		m_bottom	= std::make_shared< bottom_stream >( this );
	}

	virtual ~tls_client()
	{
	}

	virtual stream::duplex::ptr
	top() override
	{
		return m_top;
	}

	virtual stream::duplex::ptr
	bottom() override
	{
		return m_bottom;
	}

	inline promise< void >
	top_really_write( buffer buf )
	{
		auto ret = promise< void >();
		auto err = std::error_code();

fprintf( stderr, "CLIENT: top really write %d bytes\n", buf.size() );
		try
		{
			m_impl.received_data( buf.data(), buf.size() );
			ret.resolve();
		}
		CATCH_BOTAN_EXCEPTION

		if ( err )
		{
			ret.reject( err );
		}

		return ret;
	}

	void
	top_really_read()
	{
		//set_paused( false );
	}

	void
	top_really_pause()
	{
fprintf( stderr, "CLIENT: really pause\n" );

		//set_paused( true );
	}

	inline promise< void >
	bottom_really_write( buffer buf )
	{
		auto ret = promise< void >();
		auto err = std::error_code();

fprintf( stderr, "CLIENT: really write %d bytes\n", buf.size() );
		try
		{
			m_impl.received_data( buf.data(), buf.size() );
			ret.resolve();
		}
		CATCH_BOTAN_EXCEPTION

		if ( err )
		{
			ret.reject( err );
		}

		return ret;
	}

	inline void
	bottom_really_read()
	{
		//set_paused( false );
	}

	inline void
	bottom_really_pause()
	{
fprintf( stderr, "CLIENT: really pause\n" );

		//set_paused( true );
	}

	virtual const char*
	name() const override
	{
		return "CLIENT";
	}

private:

	my_policy								m_policy;
	Botan::TLS::Client						m_impl;
	std::shared_ptr< top_stream >			m_top;
	std::shared_ptr< bottom_stream >		m_bottom;
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
		err = std::error_code();
	}
	CATCH_BOTAN_EXCEPTION

	return ret;
}

#if defined( __APPLE__ )
#	pragma mark tls::server implementation
#endif

class tls_server : public tls::server, public botan_adapter< tls_server >
{
public:

	class top_stream : public stream::duplex
	{
	public:

		top_stream( tls_server *parent )
		:
			m_parent( parent )
		{
		}

		inline promise< void >
		really_write( buffer buf ) override
		{
			return m_parent->top_really_write( std::move( buf ) );
		}

		virtual void
		really_read() override
		{
			return m_parent->top_really_read();
		}

		virtual void
		really_pause() override
		{
			return m_parent->top_really_pause();
		}

	private:

		tls_server *m_parent;
	};

	class bottom_stream : public stream::duplex
	{
	public:

		bottom_stream( tls_server *parent )
		:
			m_parent( parent )
		{
		}

		~bottom_stream()
		{
		}

		inline promise< void >
		really_write( buffer buf ) override
		{
			return m_parent->bottom_really_write( std::move( buf ) );
		}

		virtual void
		really_read() override
		{
			return m_parent->bottom_really_read();
		}

		virtual void
		really_pause() override
		{
			return m_parent->bottom_really_pause();
		}

	private:

		tls_server *m_parent;
	};

	using ptr = std::shared_ptr< tls_server >;

	tls_server( options o )
	:
		botan_adapter( this ),
		m_options( std::move( o ) ),
		m_impl( *this, m_session_mgr, *this, m_policy, m_rng )
	{
		m_top		= std::make_shared< top_stream >( this );
		m_bottom	= std::make_shared< bottom_stream >( this );
	}

	virtual ~tls_server()
	{
	}

	virtual stream::duplex::ptr
	top() override
	{
		return m_top;
	}

	virtual stream::duplex::ptr
	bottom() override
	{
		return m_bottom;
	}

	std::error_code
	init()
	{
		auto err = std::error_code();

		if ( !m_options.key().empty() && !m_options.certs().empty() )
		{
			Botan::DataSource_Memory	ds_key( m_options.key() );
			Botan::DataSource_Memory	ds_certs( m_options.certs() );
			Certificate_Info			info;

fprintf( stderr, "loading key...\n" );
			info.key.reset( Botan::PKCS8::load_key( ds_key, m_rng ) );
fprintf( stderr, "loaded key\n" );

			while( !ds_certs.end_of_data() )
			{
				try
				{
fprintf( stderr, "emplacing certs\n" );
					info.certs.emplace_back( ds_certs );
fprintf( stderr, "emplaced cert\n" );
				}
				catch( std::exception& )
				{
				}
			}

			// TODO: attempt to validate chain ourselves
			m_creds.emplace_back( info );
		}
		else
		{
			err = make_error_code( tls::errc::invalid_argument );
		}

		return err;
	}

	inline promise< void >
	top_really_write( buffer buf )
	{
		auto ret = promise< void >();
		auto err = std::error_code();

fprintf( stderr, "SERVER: really write %d bytes\n", buf.size() );
		try
		{
			m_impl.received_data( buf.data(), buf.size() );
			ret.resolve();
		}
		CATCH_BOTAN_EXCEPTION

		if ( err )
		{
			ret.reject( err );
		}

		return ret;
	}

	inline void
	top_really_read()
	{
		//set_paused( false );
	}

	inline void
	top_really_pause()
	{
		//set_paused( true );
	}

	inline promise< void >
	bottom_really_write( buffer buf )
	{
		auto ret = promise< void >();
		auto err = std::error_code();

fprintf( stderr, "SERVER: really write %d bytes\n", buf.size() );
		try
		{
			m_impl.received_data( buf.data(), buf.size() );
			ret.resolve();
		}
		CATCH_BOTAN_EXCEPTION

		if ( err )
		{
			ret.reject( err );
		}

		return ret;
	}

	inline void
	bottom_really_read()
	{
		//set_paused( false );
	}

	inline void
	bottom_really_pause()
	{
		//set_paused( true );
	}

	virtual const char*
	name() const override
	{
		return "SERVER";
	}

private:

	options									m_options;
	Botan::TLS::Strict_Policy				m_policy;
	Botan::TLS::Server						m_impl;
	std::shared_ptr< top_stream >			m_top;
	std::shared_ptr< bottom_stream > 		m_bottom;
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
	auto ret = tls_server::ptr();

	try
	{
		ret = std::make_shared< tls_server >( std::move( options ) );
		err = ret->init();
	}
	CATCH_BOTAN_EXCEPTION

	if ( err )
	{
		ret.reset();
	}

	return ret;
}

namespace nodeoze {

namespace tls {

class category : public std::error_category
{
public:

	virtual const char*
	name() const noexcept override
	{
		return "tls";
	}
	
	virtual std::string
	message( int value ) const override
	{
		std::string ret;

		switch ( static_cast< errc >( value ) )
		{
			case errc::ok:
			{
				ret = "ok";
			}
			break;

			case errc::invalid_argument:
			{
				ret = "invalid argument";
			}
			break;

			case errc::unsupported_argument:
			{
				ret = "unsupported argument";
			}
			break;

			case errc::invalid_state:
			{
				ret = "invalid state";
			}
			break;

			case errc::lookup_error:
			{
				ret = "lookup error";
			}
			break;

			case errc::internal_error:
			{
				ret = "internal error";
			}
			break;

			case errc::invalid_key_length:
			{
				ret = "invalid key length";
			}
			break;

			case errc::invalid_iv_length:
			{
				ret = "invalid iv length";
			}
			break;

			case errc::prng_unseeded:
			{
				ret = "prng unseeded";
			}
			break;

			case errc::policy_violation:
			{
				ret = "policy violations";
			}
			break;

			case errc::algorithm_not_found:
			{
				ret = "algorithm not found";
			}
			break;

			case errc::no_provider_found:
			{
				ret = "no provider found";
			}
			break;

			case errc::provider_not_found:
			{
				ret = "provider not found";
			}
			break;

			case errc::invalid_algorithm_name:
			{
				ret = "invalid algorithm name";
			}
			break;

			case errc::encoding_error:
			{
				ret = "encoding error";
			}
			break;

			case errc::decoding_error:
			{
				ret = "decoding error";
			}
			break;

			case errc::integrity_failure:
			{
				ret = "integrity failure";
			}
			break;

			case errc::invalid_oid:
			{
				ret = "invalid oid";
			}
			break;

			case errc::stream_io_error:
			{
				ret = "stream io error";
			}
			break;

			case errc::self_test_failure:
			{
				ret = "self test failure";
			}
			break;

			case errc::not_implemented:
			{
				ret = "not implemented";
			}
			break;

			case errc::unknown:
			{
				ret = "unknown";
			}
			break;
		}

		return ret;
	}
};

const std::error_category&
error_category()
{
	static auto instance = new category;
	return *instance;
}

}

}
