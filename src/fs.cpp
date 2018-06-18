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

#include <nodeoze/fs.h>
#include <nodeoze/unicode.h>
#include <nodeoze/macros.h>
#include <nodeoze/log.h>
#include <nodeoze/test.h>
#include <fstream>
#include "error_libuv.h"
#include <uv.h>

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark fs::writer implementation
#endif

class fs_writer : public fs::writer
{
public:

	fs_writer( options options )
	:
		m_options( std::move( options ) )
	{
		assert( !m_options.path().empty() );

		memset( &m_open_req, 0, sizeof( m_open_req ) );
		memset( &m_write_req, 0, sizeof( m_write_req ) );
	}

	~fs_writer()
	{
	}

	inline int
	flags() const
	{
		auto ret = O_WRONLY;

		if ( m_options.create() )
		{
			ret |= O_CREAT;
		}

		if ( m_options.truncate() )
		{
			ret |= O_TRUNC;
		}

		return ret;
	}

	std::error_code
	open()
	{
		assert( !m_options.path().empty() );

		auto err	= std::error_code();
		auto ret	= uv_fs_open( uv_default_loop(), &m_open_req, m_options.path().c_str(), flags(), 0644, nullptr );

		if ( ret < 0 )
		{
			err = std::error_code( ret, libuv::error_category() );
		}

		m_open_req.ptr = this;

		return err;
	}

protected:

	virtual promise< void >
	really_write( buffer b )
	{
		m_write_promise = promise< void >();

		uv_buf_t iov = { reinterpret_cast< char* >( b.rdata() ), b.size() };

		uv_fs_write( uv_default_loop(), &m_write_req, m_open_req.result, &iov, 1, -1, on_write );

		m_write_req.ptr = this;

		return m_write_promise;
	}

	static void
	on_write( uv_fs_t *req )
	{
		assert( req );

		auto self = reinterpret_cast< fs_writer* >( req->ptr );

		assert( self );

		if ( req->result > 0 )
		{
			self->m_write_promise.resolve();
		}
		else if ( req->result == 0 )
		{
			fprintf( stderr, "result is 0\n" );
		}
		else
		{
			auto err = std::error_code( req->result, libuv::error_category() );

			self->m_write_promise.reject( err, reject_context );

			self->emit( "error", err );
		}
	}

	options				m_options;
	uv_fs_t				m_open_req;
	uv_fs_t				m_write_req;
	promise< void >		m_write_promise;
};

fs::writer::ptr
fs::writer::create( options options )
{
	std::error_code err;

	auto ret = create( std::move( options ), err );

	if ( err )
	{
	}

	return ret;
}

fs::writer::ptr
fs::writer::create( options options, std::error_code &err )
{
	auto ret = std::make_shared< fs_writer >( std::move( options ) );

	err = ret->open();

	return ret;
}

fs::writer::~writer()
{
}

#if defined( __APPLE__ )
#	pragma mark fs::reader implementation
#endif

class fs_reader : public fs::reader
{
public:

	fs_reader( options options )
	:
		m_options( options ),
		m_read_buf( 16384 ),
		m_paused( true )
	{
		assert( !m_options.path().empty() );

		memset( &m_open_req, 0, sizeof( m_open_req ) );
		memset( &m_read_req, 0, sizeof( m_open_req ) );
	}

	inline std::error_code
	open()
	{
		assert( !m_options.path().empty() );
		auto err = std::error_code();
		auto ret = uv_fs_open( uv_default_loop(), &m_open_req, m_options.path().c_str(), O_RDONLY, 0644, nullptr );

		if ( ret < 0 )
		{
			err = std::error_code( ret, libuv::error_category() );
		}

		m_open_req.ptr = this;

		return err;
	}

	virtual ~fs_reader()
	{
		// uv_fs_req_cleanup( &m_open_req );
		// uv_fs_req_cleanup( &m_read_req );
	}

protected:

	virtual void
	really_read()
	{
		m_paused = false;

		really_really_read();
	}

	virtual void
	really_pause()
	{
		m_paused = true;
	}

	inline void
	really_really_read()
	{
		uv_buf_t iov = { reinterpret_cast< char* >( m_read_buf.rdata() ), m_read_buf.size() };
		uv_fs_read( uv_default_loop(), &m_read_req, m_open_req.result, &iov, 1, -1, on_read );
		m_read_req.ptr = this;
	}

private:

	static void
	on_read( uv_fs_t *req )
	{
		assert( req );

		auto self = reinterpret_cast< fs_reader* >( req->ptr );

		assert( self );

		if ( req->result > 0 )
		{
			self->m_read_buf.size( req->result );

			self->push( self->m_read_buf );
			//iov.len = req->result;
			// uv_fs_write(uv_default_loop(), &write_req, 1, &iov, 1, -1, on_write);

			if ( !self->m_paused )
			{
				self->really_really_read();
			}
		}
		else if ( req->result == 0 )
		{
			uv_fs_close( uv_default_loop(), &self->m_close_req, self->m_open_req.result, nullptr );

			self->emit( "end" );
		}
		else
		{
			self->emit( "error", std::error_code( req->result, libuv::error_category() ) );
		}
	}

	options				m_options;
	uv_fs_t				m_open_req;
	uv_fs_t				m_read_req;
	uv_fs_t				m_close_req;
	buffer				m_read_buf;
	bool				m_paused;
	// *buf = uv_buf_init((char*) malloc(suggested_size), suggested_size);
	// iov = uv_buf_init(buffer, sizeof(buffer));k
};

fs::reader::ptr
fs::reader::create( options options )
{
	std::error_code err;

	auto ret = create( std::move( options ), err );

	if ( err )
	{
	}

	return ret;
}

fs::reader::ptr
fs::reader::create( options options, std::error_code &err )
{
	auto ret = std::make_shared< fs_reader >( std::move( options ) );

	err = ret->open();

	return ret;
}

fs::reader::~reader()
{
}

TEST_CASE( "nodeoze/smoke/fs")
{
	filesystem::path in( "/tmp/in.txt" );
	filesystem::path out( "/tmp/out.txt" );
	std::error_code err;
	auto done = false;

	std::ofstream ofs( in.c_str() );

	for ( auto i = 0u; i < 1000000; i++ )
	{
		ofs << "all work and no play make jack a dull boy";
	}

	ofs.close();

	auto rstream = fs::reader::create( in, err );
	REQUIRE( !err );
	REQUIRE( rstream );

	rstream->on( "error", [&]( std::error_code err ) mutable
	{
		REQUIRE( !err );
	} );

	auto wstream = fs::writer::create( fs::writer::options( out ).create( true ).truncate( true ), err );
	REQUIRE( !err );
	REQUIRE( wstream );

	wstream->on( "error", [&]( std::error_code err ) mutable
	{
		fprintf( stderr, "err: %d\n", err.value() );
		fprintf( stderr, "err: %s\n", err.message().c_str() );
		REQUIRE( !err );
		done = true;
	} );

	rstream->pipe( wstream );

	wstream->on( "finish", [&]() mutable
	{
		done = true;
	} );

	while ( !done )
	{
		runloop::shared().run( runloop::mode_t::once );
	}
}