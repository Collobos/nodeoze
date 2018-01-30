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

#include <nodeoze/nprocess.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nbuffer.h>
#include <nodeoze/nthread.h>
#include <nodeoze/ntest.h>
#include "nerror_libuv.h"
#include <uv.h>

extern char** environ;

using namespace nodeoze;

struct pipe_t : public uv_pipe_t
{
};

struct process_s : public uv_process_s
{
	process::input_f							stdin_handler;
	process::output_f							stdout_handler;
	nodeoze::buffer								stdout_buf;
	process::output_f							stderr_handler;
	nodeoze::buffer								stderr_buf;
	uv_process_options_t						options;
	uv_stdio_container_t						stdio[3];
	pipe_t										*in;
	pipe_t										*out;
	pipe_t										*err;
	promise< std::pair< std::int64_t, int > >	ret;
	
	process_s()
	{
		memset( &options, 0, sizeof( options ) );
		memset( &stdio, 0, sizeof( stdio ) );
		
		in	= new pipe_t;
		uv_pipe_init( uv_default_loop(), in, 0 );
		in->data = this;
		
		out = new pipe_t;
		uv_pipe_init( uv_default_loop(), out, 0 );
		out->data = this;

		err = new pipe_t;
		uv_pipe_init( uv_default_loop(), err, 0 );
		err->data = this;
		
		options.stdio					= stdio;
		options.stdio[ 0 ].flags		= static_cast< uv_stdio_flags >( UV_CREATE_PIPE | UV_READABLE_PIPE );
		options.stdio[ 0 ].data.stream	= reinterpret_cast< uv_stream_t* >( in );
		options.stdio[ 1 ].flags		= static_cast< uv_stdio_flags >( UV_CREATE_PIPE | UV_WRITABLE_PIPE );
		options.stdio[ 1 ].data.stream	= reinterpret_cast< uv_stream_t* >( out );
		options.stdio[ 2 ].flags		= static_cast< uv_stdio_flags >( UV_CREATE_PIPE | UV_WRITABLE_PIPE );
		options.stdio[ 2 ].data.stream	= reinterpret_cast< uv_stream_t* >( err );
		options.stdio_count				= 3;
	}
	
	~process_s()
	{
		uv_close( reinterpret_cast< uv_handle_t* >( in ), []( auto handle )
		{
			delete handle;
		} );
		
		uv_close( reinterpret_cast< uv_handle_t* >( out ), []( auto handle )
		{
			delete handle;
		} );

		uv_close( reinterpret_cast< uv_handle_t* >( err ), []( auto handle )
		{
			delete handle;
		} );
	}
};


void
on_alloc_stdout( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
{
	auto context = reinterpret_cast< process_s* >( handle->data );
	
	context->stdout_buf.capacity( suggested_size );
	
	buf->base	= reinterpret_cast< char* >( context->stdout_buf.data() );
	buf->len	= suggested_size;
}


void
on_alloc_stderr( uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf )
{
	auto context = reinterpret_cast< process_s* >( handle->data );
	
	context->stderr_buf.capacity( suggested_size );
	
	buf->base	= reinterpret_cast< char* >( context->stderr_buf.data() );
	buf->len	= suggested_size;
}


static void
on_write( uv_write_t *req, int status )
{
	if ( status != 0 )
	{
		auto err = std::error_code( status, libuv::error_category() );
		nlog( log::level_t::error, "error writing to child % (%)", err, err.message() );
	}
	
	delete req;
}


static void
on_read_stdout( uv_stream_t* tcp, ssize_t nread, const uv_buf_t* rdbuf )
{
	nunused( rdbuf );
	
	auto context = reinterpret_cast< process_s* >( tcp->data );
	
	if ( nread > 0 )
	{
		context->stdout_buf.size( nread );
	
		if ( context->stdout_handler )
		{
			context->stdout_handler( context->stdout_buf );
		}
	}
}


static void
on_read_stderr( uv_stream_t* tcp, ssize_t nread, const uv_buf_t* rdbuf )
{
	nunused( rdbuf );
	
	auto context = reinterpret_cast< process_s* >( tcp->data );
	
	if ( nread > 0 )
	{
		context->stderr_buf.size( nread );
	
		if ( context->stderr_handler )
		{
			context->stderr_handler( context->stderr_buf );
		}
	}
}


static void
exit_cb( uv_process_t* process, std::int64_t exit_status, int term_signal )
{
	auto context = reinterpret_cast< process_s* >( process );
	
	context->ret.resolve( std::make_pair( exit_status, term_signal ) );
	
	uv_close( reinterpret_cast< uv_handle_t* >( process ), []( auto handle )
	{
		auto context = reinterpret_cast< process_s* >( handle );
		delete context;
	} );
}


std::size_t
process::rss() const
{
	std::size_t rss;

	uv_resident_set_memory( &rss );

	return rss;
}


promise< std::pair< std::int64_t, int > >
process::create( const path &exe, const std::vector< std::string > &args, const env_t &env, const path &working_directory, pid_t &pid, input_f stdin_handler, output_f stdout_handler, output_f stderr_handler )
{
	auto context	= std::unique_ptr< process_s, std::function< void ( process_s* )> >( new process_s(), []( auto handle )
	{
		uv_close( reinterpret_cast< uv_handle_t* >( handle ), []( auto handle )
		{
			auto context = reinterpret_cast< process_s* >( handle );
			delete context;
		} );
	} );
#if defined( WIN32 )
	auto env_vec	= std::vector< std::string >();
#endif
	auto c_args		= std::vector< const char* >();
	auto c_env		= std::vector< const char* >();
	auto ret		= promise< std::pair< std::int64_t, int > >();
	
	context->stdin_handler	= stdin_handler;
	context->stdout_handler	= stdout_handler;
	context->stderr_handler	= stderr_handler;
	context->ret			= ret;
	
	mlog( marker::process, log::level_t::info, "exe: %", exe.to_string() );

	c_args.push_back( exe.to_string().c_str() );
	
	for ( auto &arg : args )
	{
		mlog( marker::process, log::level_t::info, "arg: %", arg );
		c_args.push_back( arg.c_str() );
	}
	
	c_args.push_back( nullptr );
	
	context->options.file		= exe.to_string().c_str();
	context->options.args		= const_cast< char** >( c_args.data() );

#if defined( WIN32 )
	context->options.flags		= UV_PROCESS_DETACHED;
#endif
	
	if ( !env.empty() )
	{
#if defined( WIN32 )
#	undef GetEnvironmentStrings

		auto strings	= GetEnvironmentStrings();
		auto prev		= 0;

		for ( auto i = 0; ; i++ )
		{
			if ( strings[ i ] == '\0' )
			{
				env_vec.emplace_back( strings + prev, strings + i );
				prev = i;

				if ( strings[ i + 1 ] == '\0' )
				{
					break;
				}
			}
		}

		FreeEnvironmentStringsA( strings );

		for ( auto &string : env_vec )
		{
			c_env.push_back( string.c_str() );
		}

#else
		for ( int i = 0; environ[ i ] != nullptr; i++ )
		{
			c_env.push_back( environ[ i ] );
		}
#endif
		
		for ( auto it = env.begin(); it != env.end(); it++ )
		{
			c_env.push_back( it->c_str() );
		}

		c_env.push_back( nullptr );
		
		context->options.env = const_cast< char** >( c_env.data() );
	}
	
	if ( working_directory )
	{
		mlog( marker::process, log::level_t::info, "working directory: %", working_directory.to_string() );
		context->options.cwd	= working_directory.to_string().c_str();
	}
	
	context->options.exit_cb	= exit_cb;

	auto err = uv_spawn( uv_default_loop(), context.get(), &context->options );
	ncheck_error_action_quiet( err == 0, ret.reject( std::error_code( err, libuv::error_category() ), reject_context ), exit );
	
	if ( stdin_handler )
	{
		auto str = stdin_handler();
		auto buf = uv_buf_init( const_cast< char* >( str.data() ), str.size() );
		auto req = new uv_write_t;
		auto err = std::error_code( uv_write( req, reinterpret_cast< uv_stream_t* >( context->in ), &buf, 1, on_write ), libuv::error_category() );
		if ( err )
		{
			nlog( log::level_t::error, "error writing to child % (%)", err, err.message() );
		}
	}
	
	err = uv_read_start( reinterpret_cast< uv_stream_t* >( context->out ), on_alloc_stdout, on_read_stdout );
	ncheck_error_action_quiet( err == 0, ret.reject( std::error_code( err, libuv::error_category() ), reject_context ), exit );
	
	err = uv_read_start( reinterpret_cast< uv_stream_t* >( context->err ), on_alloc_stderr, on_read_stderr );
	ncheck_error_action_quiet( err == 0, ret.reject( std::error_code( err, libuv::error_category() ), reject_context ), exit );

	pid = context->pid;
	
	context.release();
	
exit:

	return ret;
}

