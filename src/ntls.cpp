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

#include <nodeoze/ntls.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nlog.h>
#include <nodeoze/nrandom.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nerror.h>
#include <nodeoze/ntest.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif
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


#if defined( __APPLE__ )
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

class tls_filter_impl : public tls::filter
{
public:

	ndefine_global_static( pending_write_list_name, std::string, "m_pending_write_list" );
	ndefine_global_static( pending_read_list_name, std::string, "m_pending_read_list" );

	static inline bool
	is_buffer_valid( const std::string &name, buffer *buf )
	{
		auto valid = ( ( buf != nullptr ) && ( buf->data() != nullptr ) && ( buf->size() > 0 ) );
		
		if ( !valid )
		{
			auto data = reinterpret_cast< void* >( buf ? buf->data() : nullptr );
			auto size = buf ? buf->size() : 0u;
		}
		
		return valid;
	}
	
	static inline void
	was_consumed( std::queue< buffer > &queue, buffer &buf, std::size_t used )
	{
		if ( buf.size() == used )
		{
			queue.pop();
		}
		else if ( used > 0 )
		{
			buf.rotate( 0, used, buf.size() );
			buf.size( buf.size() - used );
		}
	}

	friend std::error_code
	tls::use_server_cert( const std::vector< nodeoze::buffer > &chain, const nodeoze::buffer &key );

	tls_filter_impl( role_t r );
	
	virtual ~tls_filter_impl();

	virtual const std::string&
	name() const
	{
		static std::string s( "tls" );
		return s;
	}
	
	virtual stream::state_t
	state() const
	{
		return stream::state_t::connected;
	}
	
	virtual stream::state_t
	send( buffer &in_buf, buffer &out_buf );
		
	virtual stream::state_t
	recv( std::vector< buffer > &in_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs );
	
	virtual void
	reset();
	
private:
	
	bool
	make_self_signed_cert( X509 **x509p, EVP_PKEY **pkeyp, int bits, int serial, int days );
	
	int
	add( X509 *cert, int nid, const char *value );
	
	static void
	callback(int p, int n, void *arg);
	
	static std::vector< X509* >&
	certs()
	{
		static auto global = new std::vector< X509* >();
		return *global;
	}
	
	std::vector< std::uint8_t >		m_send_data;
	std::vector< std::uint8_t >		m_recv_data;
	static EVP_PKEY					*m_pkey;
	
protected:

	void
	process( buffer &out_send_buf, std::vector< buffer > &out_recv_bufs );

	std::streamsize
	data_to_write( buffer &in_buf );

	std::streamsize
	data_to_read( buffer &in_buf, std::vector< buffer > &out_bufs );

	void
	grab_pending_data( buffer &buf );
	
	
	
	void
	handle_error( int result);
	
	void
	setup();
	
	void
	teardown();

	role_t					m_role;
	std::queue< buffer >	m_pending_write_list;
	std::queue< buffer >	m_pending_read_list;
	bool					m_read_required;
	std::error_code			m_err;
	SSL						*m_ssl;
	SSL_CTX					*m_ssl_context;
};


std::error_code
tls::use_server_cert( const std::vector< nodeoze::buffer > &chain, const nodeoze::buffer &key )
{
	const std::uint8_t		*data;
	std::vector< X509* >	certs;
	X509					*cert;
	EVP_PKEY				*pkey;
	RSA						*rsa;
	std::error_code			err;
	
	ncheck_error_action( chain.size() >= 1, err = make_error_code( std::errc::invalid_argument ), exit, "no certs in chain" );
	
	data = chain[ 0 ].data();
	cert = d2i_X509( nullptr, &data, chain[ 0 ].size() );
	ncheck_error_action( cert, err = make_error_code( std::errc::invalid_argument ), exit, "d2i_x509() failed" );
	
	certs.push_back( cert );
	
	for ( auto i = 1u; i < chain.size(); i++ )
	{
		data = chain[ i ].data();
		cert = d2i_X509( nullptr, &data, chain[ i ].size() );
		ncheck_error_action( cert, err = make_error_code( std::errc::invalid_argument ), exit, "d2i_x509() failed" );
		certs.push_back( cert );
	}
	
	data = key.data();
	
	rsa = d2i_RSAPrivateKey( nullptr, &data, key.size() );
	ncheck_error_action( rsa, err = make_error_code( std::errc::invalid_argument ), exit, "d2i_RSAPrivateKey() failed" );
	
	pkey = EVP_PKEY_new();
	ncheck_error_action( pkey, err = make_error_code( std::errc::invalid_argument ), exit, "EVP_PKEY_new() failed" );
	
	EVP_PKEY_assign_RSA( pkey, rsa );
	
	tls_filter_impl::certs()	= certs;
	tls_filter_impl::m_pkey		= pkey;
	
exit:

	return err;
}


EVP_PKEY				*tls_filter_impl::m_pkey		= nullptr;


nodeoze::tls::filter*
tls::filter::create( role_t r )
{
	return new tls_filter_impl( r );
}


tls::filter::~filter()
{
}


tls_filter_impl::tls_filter_impl( role_t r )
:
	m_role( r ),
	m_read_required( false ),
	m_ssl( nullptr ),
	m_ssl_context( nullptr )
{
	setup();
}


tls_filter_impl::~tls_filter_impl()
{
	nlog( log::level_t::debug, "" );
	
	teardown();
}


stream::state_t
tls_filter_impl::send( buffer &in_buf, buffer &out_buf )
{
	if ( in_buf.size() > 0 )
	{
		std::vector< buffer > dummy;
		
		nlog( log::level_t::debug, "pushing buffer (% bytes)", in_buf.size() );
	
		m_pending_write_list.emplace( std::move( in_buf ) );
	
		process( out_buf, dummy );
	}
	
	return stream::state_t::connected;
}


stream::state_t
tls_filter_impl::recv( std::vector< buffer > &in_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs )
{
	for ( auto &buf : in_bufs )
	{
		if ( buf.size() > 0 )
		{
			m_pending_read_list.emplace( std::move( buf ) );
		}
	}
	
	process( out_send_buf, out_recv_bufs );
	
	return stream::state_t::connected;
}


void
tls_filter_impl::reset()
{
	teardown();
	setup();
}


void
tls_filter_impl::process( buffer &out_send_buf, std::vector< buffer > &out_recv_bufs )
{
	out_send_buf.clear();
	out_recv_bufs.clear();

	while ( ( !m_read_required && ( m_pending_write_list.size() > 0 ) ) || ( m_pending_read_list.size() > 0 ) )
	{
		if ( SSL_in_init( m_ssl ) )
		{
		}

		if ( m_pending_read_list.size() > 0 )
		{
			auto &in_buf = m_pending_read_list.front();
			
			if ( is_buffer_valid( pending_read_list_name(), &in_buf ) )
			{
				auto	used	= data_to_read( in_buf, out_recv_bufs );
				
				if ( used > 0 )
				{
					was_consumed( m_pending_read_list, in_buf, static_cast< std::size_t >( used ) );
				}
				else if ( used < 0 )
				{
					break;
				}
			}
			else
			{
				m_pending_read_list.pop();
			}
		}

		if ( !m_read_required && ( m_pending_write_list.size() > 0 ) )
		{
			auto &in_buf	= m_pending_write_list.front();
			
			if ( is_buffer_valid( pending_write_list_name(), &in_buf ) )
			{
				auto used		= data_to_write( in_buf );
				
				if ( used > 0 )
				{
					was_consumed( m_pending_write_list, in_buf, static_cast< std::size_t >( used ) );
				}
				else if ( used < 0 )
				{
					break;
				}
			}
			else
			{
				m_pending_write_list.pop();
			}
		}

		if ( BIO_ctrl_pending( m_ssl->wbio ) )
		{
			grab_pending_data( out_send_buf );
		}
	}
}


std::streamsize
tls_filter_impl::data_to_write( buffer &buf )
{
	std::streamsize bytes_used	= 0;
	std::streamsize result		= SSL_write( m_ssl, buf.data(), static_cast< int >( buf.size() ) );

	if ( result < 0 )
	{
		handle_error( ( int ) result );
	}
	else
	{
		bytes_used = result;
	}

	if ( SSL_want_read( m_ssl ) )
	{
		m_read_required = true;
	}

	return bytes_used;
}


std::streamsize
tls_filter_impl::data_to_read( buffer &in_buf, std::vector< buffer > &out_bufs )
{
	std::size_t			bytes_used	= BIO_write( m_ssl->rbio, in_buf.data(), ( int ) in_buf.size() );
	int					bytes_out	= 0;
	const std::size_t	buf_size	= 8192;
	buffer				out_buf;
   
	m_read_required = false;
	
	do
	{
		if ( out_buf.space() < buf_size )
		{
			out_buf.capacity( out_buf.capacity() + ( buf_size - out_buf.space() ) );
		}
	
		assert( out_buf.space() > 0 );
		
		bytes_out = SSL_read( m_ssl, ( void* ) ( out_buf.data() + out_buf.size() ), static_cast< int >( out_buf.space() ) );

		if ( bytes_out > 0 )
		{
			out_buf.size( out_buf.size() + bytes_out );
		}
		else if ( bytes_out < 0 )
		{
			handle_error( bytes_out );
		}
	}
	while ( bytes_out > 0 );
	
	if ( out_buf.size() )
	{
		out_bufs.emplace_back( std::move( out_buf ) );
	}

	return bytes_used;
}


void
tls_filter_impl::grab_pending_data( buffer &buf )
{
	std::size_t pending;

	while ( ( pending = BIO_ctrl_pending( m_ssl->wbio ) ) > 0 )
	{
		if ( buf.space() < pending )
		{
			buf.capacity( buf.capacity() + ( pending - buf.space() ) );
		}
		
		assert( buf.space() > 0 );
		
		int bytes_to_grab = BIO_read( m_ssl->wbio, ( void* ) ( buf.data() + buf.size() ), static_cast< int >( buf.space() ) );

		if ( bytes_to_grab > 0 )
		{
			buf.size( buf.size() + bytes_to_grab );
		}
		else
		{
			if ( !BIO_should_retry( m_ssl->wbio ) )
			{
				handle_error( bytes_to_grab );
			}
		}
	}
}


void
tls_filter_impl::handle_error( int result )
{
	if ( result <= 0 )
	{
		int error = SSL_get_error(m_ssl, result);

		switch ( error )
		{
			case SSL_ERROR_ZERO_RETURN:
			case SSL_ERROR_NONE:
			case SSL_ERROR_WANT_READ:
			{
			}
			break;

			default :
			{
				char buffer[256];

				while ( error != 0 )
				{
					ERR_error_string_n( error, buffer, sizeof( buffer ) );

					nlog( log::level_t::error, "tls error: % - %", error, buffer );

					error = ( int ) ERR_get_error();
				}
				
				//m_error = true;
			}
			break;
		}
	}
}


bool
tls_filter_impl::make_self_signed_cert( X509 **x509p, EVP_PKEY **pkeyp, int bits, int serial, int days )
{
	X509		*x;
	EVP_PKEY	*pk;
	RSA			*rsa;
	X509_NAME	*name=NULL;
	bool		ok = false;
	
	if ( ( pkeyp == NULL ) || ( *pkeyp == NULL ) )
	{
		if ( ( pk = EVP_PKEY_new() ) == NULL )
		{
			nlog( log::level_t::error, "EVP_PKEY_new() failed" );
			goto exit;
		}
	}
	else
	{
		pk = *pkeyp;
	}

	if ( ( x509p == NULL ) || ( *x509p == NULL ) )
	{
		if ( ( x = X509_new() ) == NULL )
		{
			nlog( log::level_t::error, "X509_new() failed" );
			goto exit;
		}
	}
	else
	{
		x = *x509p;
	}

	rsa = RSA_generate_key( bits, RSA_F4, callback, NULL );
	
	if ( !EVP_PKEY_assign_RSA( pk, rsa ) )
	{
		nlog( log::level_t::error, "EVP_PKEY_assign_RSA() failed" );
		goto exit;
	}

	rsa = NULL;

	X509_set_version( x,2 );
	ASN1_INTEGER_set( X509_get_serialNumber(x),serial);
	X509_gmtime_adj( X509_get_notBefore(x),0);
	X509_gmtime_adj( X509_get_notAfter(x),(long)60*60*24*days);
	X509_set_pubkey( x,pk);

	name = X509_get_subject_name(x);

	if ( !X509_NAME_add_entry_by_txt(name,"C", MBSTRING_ASC, ( const unsigned char* ) "US", -1, -1, 0) )
	{
		nlog( log::level_t::error, "X509_NAME_add_entry_by_txt() failed" );
		goto exit;
	}
	
	if ( !X509_NAME_add_entry_by_txt(name,"CN", MBSTRING_ASC, ( const unsigned char* ) "OpenSSL Group", -1, -1, 0 ) )
	{
		nlog( log::level_t::error, "X509_NAME_add_entry_by_txt() failed" );
		goto exit;
	}

	// Its self signed so set the issuer name to be the same as the subject.
	
	X509_set_issuer_name(x,name);

	// Add various extensions: standard extensions
	
	if ( !add( x, NID_basic_constraints, "critical,CA:TRUE" ) )
	{
		goto exit;
	}
	
	if ( !add( x, NID_key_usage, "critical,keyCertSign,cRLSign" ) )
	{
		goto exit;
	}

	if ( !add( x, NID_subject_key_identifier, "hash" ) )
	{
		goto exit;
	}

	if ( !X509_sign( x, pk, EVP_md5() ) )
	{
		goto exit;
	}

	*x509p = x;
	*pkeyp = pk;
	
	ok = true;
	
exit:

	return ok;
}


int
tls_filter_impl::add( X509 *cert, int nid, const char *value )
{
	X509_EXTENSION *ex;
	X509V3_CTX		ctx;
	int				ret;
	
	X509V3_set_ctx_nodb(&ctx);
	X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
	
	ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, ( char* ) value);
	
	if ( !ex )
	{
		nlog( log::level_t::error, "X509V3_EXT_conf_nid() failed" );
		ret = 0;
		goto exit;
	}

	X509_add_ext(cert,ex,-1);
	X509_EXTENSION_free(ex);
	
	ret = 1;
	
exit:

	return ret;
}


void
tls_filter_impl::callback(int p, int n, void *arg)
{
	nunused( p );
	nunused( n );
	nunused( arg );
}


void
tls_filter_impl::setup()
{
	static bool init = false;
	
	if ( !init )
	{
		CRYPTO_malloc_init();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
		SSL_library_init();

		CRYPTO_cleanup_all_ex_data();
		
		init = true;
	}
	
	m_read_required = false;
	m_err			= std::error_code();
	m_ssl_context	= nullptr;
	m_ssl			= nullptr;

	switch ( m_role )
	{
		case role_t::server:
		{
			if ( certs().empty() )
			{
				X509			*cert = nullptr;
				random< int > r;

#if !defined( OPENSSL_NO_ENGINE )
				ENGINE_load_builtin_engines();
#endif
				auto ok = make_self_signed_cert( &cert, &m_pkey, 2048, r.generate(), 3650 );
				certs().push_back( cert );
				
				if ( !ok )
				{
					nlog( log::level_t::error, "unable to make certificate" );
				}
				
#ifndef OPENSSL_NO_ENGINE
				ENGINE_cleanup();
#endif
			}

			m_ssl_context = SSL_CTX_new( TLSv1_server_method() );
			SSL_CTX_set_options( m_ssl_context, SSL_OP_NO_SSLv2 );
			SSL_CTX_use_PrivateKey( m_ssl_context, m_pkey );
			SSL_CTX_use_certificate( m_ssl_context, certs()[ 0 ] );
			
			for ( auto i = 1u; i < certs().size(); i++ )
			{
				SSL_CTX_add_extra_chain_cert( m_ssl_context, certs()[ i ] );
				CRYPTO_add( &certs()[ i ]->references, 1, CRYPTO_LOCK_X509 );
			}
	
			m_ssl = SSL_new( m_ssl_context );
			SSL_set_bio( m_ssl, BIO_new( BIO_s_mem() ), BIO_new( BIO_s_mem() ) );
			SSL_set_accept_state( m_ssl );
		}
		break;

		case role_t::client:
		{
			m_ssl_context = SSL_CTX_new( TLSv1_client_method() );
			SSL_CTX_set_options( m_ssl_context, SSL_OP_NO_SSLv2 );
	
			m_ssl = SSL_new( m_ssl_context );
			SSL_set_bio( m_ssl, BIO_new( BIO_s_mem() ), BIO_new( BIO_s_mem() ) );
			SSL_set_connect_state( m_ssl );
		}
		break;
	}
	
}


void
tls_filter_impl::teardown()
{
	if ( m_ssl )
	{
		SSL_free( m_ssl );
		m_ssl = nullptr;
	}
	
	if ( m_ssl_context )
	{
		SSL_CTX_free( m_ssl_context );
		m_ssl_context = nullptr;
	}
	
	while ( m_pending_write_list.size() )
	{
		m_pending_write_list.pop();
	}
	
	while ( m_pending_read_list.size() )
	{
		m_pending_read_list.pop();
	}
}


#if defined( __APPLE__ )
#	pragma clang diagnostic pop
#endif

TEST_CASE( "nodeoze/smoke/tls" )
{
	SUBCASE( "was_consumed" )
	{
		std::queue< buffer > buffers;
		auto checker = []( const buffer &buf, std::size_t size )
		{
			CHECK( buf.size() == size );
			
			for ( auto i = 0u; i < buf.size(); i++ )
			{
				CHECK( buf[ i ] == ( i + ( 10 - size ) ) );
			}
		};
		
		buffers.emplace( buffer( 10 ) );
		
		for ( auto i = 0; i < 10; i++ )
		{
			buffers.front().push_back( i );
		}
		
		CHECK( buffers.size() == 1 );
		CHECK( buffers.front().size() == 10 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 9 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 8 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 7 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 6 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 5 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 4 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 3 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 2 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		checker( buffers.front(), 1 );
		tls_filter_impl::was_consumed( buffers, buffers.front(), 1 );
		CHECK( buffers.size() == 0 );
	}
	
	SUBCASE( "buffer integrity" )
	{
		CHECK( !tls_filter_impl::is_buffer_valid( tls_filter_impl::pending_write_list_name(), nullptr ) );
		buffer b;
		CHECK( !tls_filter_impl::is_buffer_valid( tls_filter_impl::pending_write_list_name(), &b ) );
		b.size( 10 );
		CHECK( tls_filter_impl::is_buffer_valid( tls_filter_impl::pending_write_list_name(), &b ) );
	}
}
