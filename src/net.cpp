/*
 * Copyright (c) 2013-2018, Collobos Software Inc.
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
 
#include <nodeoze/net.h>
#include <nodeoze/test.h>
#include "net_utils.h"
#include "error_libuv.h"
#include <functional>
#include <queue>
#include <errno.h>
#include <uv.h>

using namespace nodeoze;

#if defined( WIN32 )

#include <win/stream-inl.h>

int
uv_tcp_open_stream(uv_tcp_t* handle, uv_os_sock_t sock)
{
	auto err = std::error_code( uv_tcp_open(handle, sock), libuv::error_category() );

	if ( !err )
	{
		uv_connection_init( reinterpret_cast< uv_stream_t* >( handle ) );
		handle->flags |= UV_HANDLE_BOUND | UV_HANDLE_READABLE | UV_HANDLE_WRITABLE;
	}

	return err.value();
}

#else

# define uv_tcp_open_stream uv_tcp_open

#endif

#if defined( __APPLE__ )
#	pragma mark net::tcp::socket implementation
#endif

class net_tcp_socket : public net::tcp::socket
{
public:

	net_tcp_socket()
	:
		m_options( ip::endpoint() ),
		m_handle( nullptr )
	{
		m_recv_buf.make_no_copy_on_write();
	}

	net_tcp_socket( options options )
	:
		m_options( std::move( options ) ),
		m_handle( nullptr )
	{
		m_recv_buf.make_no_copy_on_write();
	}

	virtual ~net_tcp_socket()
	{
	}

	std::error_code
	accept( uv_tcp_t *handle )
	{
		m_handle = new uv_tcp_t;
		uv_tcp_init( uv_default_loop(), m_handle );
		m_handle->data = this;
		auto err = std::error_code( uv_accept( reinterpret_cast< uv_stream_t* >( handle ), reinterpret_cast< uv_stream_t* >( m_handle ) ), libuv::error_category() );

		return err;
	}

	std::error_code
	start_connect()
	{
		assert( !m_handle );

		m_handle		= new uv_tcp_t;
		m_handle->data	= this;

		auto err = std::error_code( uv_tcp_init( uv_default_loop(), m_handle ), libuv::error_category() );

		if ( !err )
		{
			sockaddr_storage addr;
		
			ip::endpoint_to_sockaddr( m_options.endpoint(), addr );

			auto req = new uv_connect_t;

			req->handle	= reinterpret_cast< uv_stream_t* >( m_handle );
			err	= std::error_code( uv_tcp_connect( req, m_handle, reinterpret_cast< sockaddr* >( &addr ), reinterpret_cast< uv_connect_cb >( on_connect ) ), libuv::error_category() );

			if ( err )
			{
				delete req;
			}
		}
	
		return err;
	}
	
	virtual ip::endpoint
	name() const
	{
		sockaddr_storage	addr;
		int					len;
		ip::endpoint		ret;

		memset( &addr, 0, sizeof( addr ) );
		len = sizeof( sockaddr_storage );
		uv_tcp_getsockname( m_handle, reinterpret_cast< sockaddr* >( &addr ), &len );

		ip::sockaddr_to_endpoint( addr, ret );

		return ret;
	}

	virtual ip::endpoint
	peer() const
	{
		sockaddr_storage	addr;
		int					len = sizeof( addr );
		ip::endpoint		ret;
	
		memset( &addr, 0, sizeof( addr ) );
		uv_tcp_getpeername( reinterpret_cast< const uv_tcp_t* >( this ), reinterpret_cast< sockaddr* >( &addr ), &len );
		
		ip::sockaddr_to_endpoint( addr, ret );
		
		return ret;
	}

	virtual void
	close()
	{
		m_handle->data = nullptr;

		uv_close( reinterpret_cast< uv_handle_t* >( m_handle ), []( uv_handle_t *handle ) mutable
		{
			delete handle;
		} );
	}

	virtual promise< void >
	really_write( buffer buf )
	{
		auto ret = promise< void >();

		assert( m_handle );

		if ( m_handle )
		{
			if ( buf.size() > 0 )
			{
				auto req	= new write_t( m_handle, std::move( buf ), ret );
				auto err	= uv_write( req, reinterpret_cast< uv_stream_t* >( m_handle ), &req->m_uv_buf, 1, reinterpret_cast< uv_write_cb >( on_send ) );
				ncheck_error_action_quiet( err == 0, on_send( req, err ), exit );
			}
			else
			{
				ret.resolve();
			}
		}
		else
		{
			ret.reject( make_error_code( std::errc::invalid_argument ), reject_context );
		}
		
	exit:

		return ret;
	}

	virtual void
	really_read()
	{
		assert( m_handle );

		if ( m_handle )
		{
			auto err = std::error_code( uv_read_start( reinterpret_cast< uv_stream_t* >( m_handle ), reinterpret_cast< uv_alloc_cb >( on_alloc ), reinterpret_cast< uv_read_cb >( on_recv ) ), libuv::error_category() );
			ncheck_error_action_quiet( !err, emit( "error", err ), exit );
		}
		else
		{
			emit( "error", std::error_code( UV_EFAULT, libuv::error_category() ) );
		}
	
	exit:

		return;
	}

	virtual void
	really_pause()
	{
		assert( m_handle );

		if ( m_handle )
		{
			auto err = std::error_code( uv_read_stop( reinterpret_cast< uv_stream_t* >( m_handle ) ), libuv::error_category() );
			ncheck_error_action_quiet( !err, emit( "error", err ), exit );
		}
		else
		{
			emit( "error", std::error_code( UV_EFAULT, libuv::error_category() ) );
		}

	exit:

		return;
	}

	struct write_t : public uv_write_t
	{
		write_t( uv_tcp_t *handle, buffer buf, promise< void > ret )
		:
			m_buf( std::move( buf ) ),
			m_ret( ret )
		{
			this->handle	= reinterpret_cast< uv_stream_t* >( handle );
			m_uv_buf.base	= reinterpret_cast< char* >( m_buf.mutable_data() );
#if defined( WIN32 )
			m_uv_buf.len	= static_cast< ULONG >( m_buf.size() );
#else
			m_uv_buf.len	= m_buf.size();
#endif
		}
	
		uv_buf_t		m_uv_buf;
		buffer			m_buf;
		promise< void >	m_ret;
	};

	inline void
	set_keep_alive( bool val )
	{
		uv_tcp_keepalive( m_handle, val, 0 );
	}

	inline static void
	on_alloc( uv_tcp_t *handle, size_t size_hint, uv_buf_t *buf )
	{
		assert( handle );

		auto self = reinterpret_cast< net_tcp_socket* >( handle->data );
		ncheck_error_quiet( self, exit );
		ncheck_error_quiet( buf, exit );
		
		self->m_recv_buf.capacity( size_hint );
		
		buf->base	= reinterpret_cast< char* >( self->m_recv_buf.mutable_data() );
	#if defined( WIN32 )
		buf->len	= static_cast< ULONG >( size_hint );
	#else
		buf->len	= size_hint;
	#endif
		
	exit:

		return;
	}

	inline static void
	on_connect( uv_connect_t *req, int libuv_err )
	{
		auto err = std::error_code( libuv_err, libuv::error_category() );

		assert( req );

		if ( req )
		{
			auto stream = reinterpret_cast< uv_tcp_t* >( req->handle );
			assert( stream );

			if ( stream )
			{
				auto self = reinterpret_cast< net_tcp_socket* >( stream->data );
				assert( self );

				if ( self )
				{
					if ( !err )
					{
						self->emit( "connect" );
					}
					else
					{
						self->emit( "error", err );
					}
				}
			}

			delete req;
		}
	}

	inline static void
	on_send( write_t *req, int libuv_err )
	{
		auto err = std::error_code( libuv_err, libuv::error_category() );

		assert( req );
		if ( req )
		{
			auto stream = reinterpret_cast< uv_stream_t* >( req->handle );
			assert( stream );

			if ( stream )
			{
				auto self = reinterpret_cast< net_tcp_socket* >( stream->data );
				assert( self );

				if ( self )
				{
					if ( !err )
					{
						req->m_ret.resolve();
					}
					else
					{
						req->m_ret.reject( err, reject_context );
						self->emit( "error", err );
					}
				}
			}

			delete req;
		}
	}

	inline static void
	on_recv( uv_tcp_t *handle, std::int32_t nread, const uv_buf_t *buf )
	{
		assert( handle );
		nunused( buf );

		if ( handle )
		{
			auto self = reinterpret_cast< net_tcp_socket* >( handle->data );
			assert( self );

			if ( self )
			{
				if ( nread > 0 )
				{
					self->m_recv_buf.size( nread );
					self->push( self->m_recv_buf );
				}
				else if ( nread < 0 )
				{
					auto err = std::error_code( nread, libuv::error_category() );
					self->emit( "error", err );
					self->m_recv_buf.size( 0 );
				}
			}
		}
	}

	net::tcp::socket::options	m_options;
	buffer						m_recv_buf;
	uv_tcp_t					*m_handle;
};

net::tcp::socket::~socket()
{
}

net::tcp::socket::ptr
net::tcp::socket::create( net::tcp::socket::options options )
{
	auto err = std::error_code();
	auto ret = create( std::move( options ), err );

	if ( err )
	{
		// throw exception
	}

	return ret;
}

net::tcp::socket::ptr
net::tcp::socket::create( net::tcp::socket::options options, std::error_code &err )
{
	auto ret = std::make_shared< net_tcp_socket >( std::move( options ) );

	err = ret->start_connect();

	return ret;
}

#if defined( __APPLE__ )
#	pragma mark net_tcp_server implementation
#endif

class net_tcp_server : public net::tcp::server
{
public:

	net_tcp_server( options options )
	:
		m_options( std::move( options ) ),
		m_handle( nullptr )
	{
	}

	std::error_code
	listen()
	{
		m_handle = new uv_tcp_t;
		sockaddr_storage	addr;
		int					len;
		auto err = std::error_code( uv_tcp_init( uv_default_loop(), m_handle ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		assert( m_handle->type == UV_TCP );
		m_handle->data = this;
		
		ip::endpoint_to_sockaddr( m_options.endpoint(), addr );
		
		err = std::error_code( uv_tcp_bind( m_handle, reinterpret_cast< sockaddr* >( &addr ), 0 ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		
		len = sizeof( addr );
		err = std::error_code( uv_tcp_getsockname( m_handle, reinterpret_cast< sockaddr* >( &addr ), &len ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		
		ip::sockaddr_to_endpoint( addr, m_name );
		
		err = std::error_code( uv_listen( reinterpret_cast< uv_stream_t* >( m_handle ), static_cast< int >( m_options.qsize() ), reinterpret_cast< uv_connection_cb >( on_accept ) ), libuv::error_category() );
		ncheck_error_action( !err, emit( "error", err ), exit, "uv_listen() on % failed (%)", m_name.to_string(), err );

		runloop::shared().dispatch( [=]() mutable
		{
			emit( "listening" );
		} );
		
	exit:

		return err;
	}

	virtual const ip::endpoint&
	name() const
	{
		return m_name;
	}

	inline void
	close()
	{
		assert( m_handle->type == UV_TCP );

		uv_close( reinterpret_cast< uv_handle_t* >( m_handle ), []( uv_handle_t *handle )
		{
			delete handle;
		} );
	}

private:

	inline static void
	on_accept( uv_tcp_t *handle, int libuv_err )
	{
		assert( handle );

		auto err = std::error_code( libuv_err, libuv::error_category() );

		if ( handle )
		{
			auto self = reinterpret_cast< net_tcp_server* >( handle->data );
			assert( self );

			if ( self )
			{
				if ( !err )
				{
					auto client = std::make_shared< net_tcp_socket >();

					err = client->accept( handle );
				
					if ( !err )
					{
						self->emit( "connection", std::dynamic_pointer_cast< net::tcp::socket >( client ) );
					}
					else
					{
						self->emit( "error", err );
					}
				}
				else
				{
					self->emit( "error", err );
				}
			}
		}
	}

	options			m_options;
	uv_tcp_t		*m_handle;
	ip::endpoint	m_name;
};

net::tcp::server::~server()
{
}

net::tcp::server::ptr
net::tcp::server::create( options options )
{
	auto err = std::error_code();
	auto ret = create( std::move( options ), err );

	if ( err )
	{
	}

	return ret;
}


net::tcp::server::ptr
net::tcp::server::create( options options, std::error_code &err )
{
	auto ret = std::make_shared< net_tcp_server >( std::move( options ) );
	
	err = ret->listen();

	return ret;
}

#if defined( __APPLE__ )
#	pragma mark net_udp_socket implementation
#endif

class net_udp_socket : public net::udp::socket
{
public:

	net_udp_socket( options options )
	:
		m_options( std::move( options ) ),
		m_handle( nullptr )
	{
	}

	~net_udp_socket()
	{
	}

	std::error_code
	init()
	{
		struct sockaddr_storage addr;

		m_handle = new uv_udp_t;
		auto err = std::error_code( uv_udp_init( uv_default_loop(), m_handle ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
	
		m_handle->data = this;
	
		ip::endpoint_to_sockaddr( m_options.endpoint(), addr );
		
		err = std::error_code( uv_udp_bind( m_handle, reinterpret_cast< sockaddr* >( &addr ), UV_UDP_REUSEADDR ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		
		err = std::error_code( uv_udp_recv_start( m_handle, reinterpret_cast< uv_alloc_cb >( on_alloc ), reinterpret_cast< uv_udp_recv_cb >( on_recv ) ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		
	exit:
	
		return err;
	}

	inline ip::endpoint
	name() const
	{
		sockaddr_storage	addr;
		int					len;
		ip::endpoint		ret;
		
		memset( &addr, 0, sizeof( addr ) );
		len = sizeof( sockaddr_storage );
		uv_udp_getsockname( m_handle, reinterpret_cast< sockaddr* >( &addr ), &len );
		
		ip::sockaddr_to_endpoint( addr, ret );
		
		return ret;
	}
	
	inline std::error_code
	set_membership( const ip::endpoint &endpoint, const ip::address iface, multicast_membership_type membership )
	{
		auto err = std::error_code();
		
		err = std::error_code( uv_udp_set_membership( m_handle, endpoint.addr().to_string().c_str(), iface.to_string().c_str(), membership == multicast_membership_type::join ? UV_JOIN_GROUP : UV_LEAVE_GROUP ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		if ( membership == multicast_membership_type::join )
		{
			err = std::error_code( uv_udp_set_multicast_loop( m_handle, 1 ), libuv::error_category() );
			ncheck_error_quiet( !err, exit );
		}

	exit:

		return err;
	}
	
	inline std::error_code
	set_broadcast( bool val )
	{
		auto err = std::error_code( uv_udp_set_broadcast( m_handle, val ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );

	exit:

		return err;
	}

	inline void
	send( buffer buf, const ip::endpoint &to, promise< void > ret )
	{
		sockaddr_storage addr;
		
		memset( &addr, 0, sizeof( addr ) );
		
		ip::endpoint_to_sockaddr( to, addr );
		
		auto request	= new write_s( m_handle, buf, ret );
		auto err		= uv_udp_send( request, m_handle, &request->m_uv_buf, 1, reinterpret_cast< sockaddr* >( &addr ), reinterpret_cast< uv_udp_send_cb >( on_send ) );
		ncheck_error_action( err == 0, on_send( request, err ), exit, "uv_udp_send() to % failed (%)", to.to_string(), uv_strerror( err ) );
		
	exit:

		return;
	}

	inline void
	close()
	{
		uv_close( reinterpret_cast< uv_handle_t* >( m_handle ), []( uv_handle_t *handle )
		{
			delete handle;
		} );
	}

protected:

	struct write_s : public uv_udp_send_s
	{
		write_s( uv_udp_t *handle, buffer buf, promise< void > ret )
		:
			m_buf( std::move( buf ) ),
			m_ret( ret )
		{
			this->handle = handle;

			m_uv_buf.base	= reinterpret_cast< char* >( m_buf.mutable_data() );
#if defined( WIN32 )
			m_uv_buf.len	= static_cast< ULONG >( m_buf.size() );
#else
			m_uv_buf.len	= m_buf.size();
#endif
		}
	
		uv_buf_t				m_uv_buf;
		buffer					m_buf;
		promise< void >			m_ret;
	};

	inline static void
	on_send( write_s *req, int libuv_err )
	{
		auto err = std::error_code( libuv_err, libuv::error_category() );

		assert( req );

		if ( req )
		{
			auto handle = reinterpret_cast< uv_udp_t* >( req->handle );
			assert( handle );

			if ( handle )
			{
				auto self = reinterpret_cast< net_udp_socket* >( handle->data );

				//self->on_send( error, request->m_ret );
			}
		
			if ( req )
			{
				delete req;
			}
		}
	}

	inline static void
	on_alloc( uv_udp_t *handle, size_t size_hint, uv_buf_t *buf )
	{
		assert( handle );
		assert( buf );

		auto self = reinterpret_cast< net_udp_socket* >( handle->data );
		
		self->m_recv_buf.capacity( size_hint );
		
		buf->base	= reinterpret_cast< char* >( self->m_recv_buf.mutable_data() );
#if defined( WIN32 )
		buf->len	= static_cast< ULONG >( size_hint );
#else
		buf->len	= size_hint;
#endif
	}

	inline static void
	on_recv( uv_udp_t *handle, std::int32_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags )
	{
		assert( handle );
		assert( buf );

		nunused( flags );
		
		ip::endpoint from;
		
		if ( addr )
		{
			ip::sockaddr_to_endpoint( *reinterpret_cast< const sockaddr_storage* >( addr ), from );
		}
		
		if ( nread > 0 )
		{
//			self->owner()->on_recv( std::error_code(), from, buffer( buf->base, nread, nread, buffer::do_not_delete_data ) );
			// self->on_recv( std::error_code(), from, buffer( buf->base, nread, buffer::policy::exclusive, nullptr, nullptr ) );
		}
		else if ( nread < 0 )
		{
			// self->on_recv( std::error_code( nread, libuv::error_category() ), from, buffer() );
		}
	}

	options		m_options;
	buffer		m_recv_buf;
	uv_udp_t	*m_handle;
};

TEST_CASE( "nodeoze/smoke/net/tcp")
{
	auto message									= std::string( "this is one small step for man" );
	auto server_events 								= std::vector< std::string >();
	auto client_events 								= std::vector< std::string >();
	auto name										= ip::endpoint( "127.0.0.1", 5555 );
	auto server										= net::tcp::server::create( name );
	deque< std::shared_ptr< net::tcp::socket > >	connections;
	auto client										= net::tcp::socket::create( name );
	bool											done = false;

	server->on( "listening", [&]() mutable
	{
		server_events.push_back( "listening" );
	} );

	server->on( "connection", [&]( net::tcp::socket::ptr sock ) mutable
	{
		server_events.push_back( "connection" );

		connections.emplace_back( std::move( sock ) );

		connections.back()->on( "data", [&]( buffer buf ) mutable
		{
			server_events.push_back( "data" );

			REQUIRE( buf.to_string() == message );

			connections.back()->write( buf );
		} );

		connections.back()->on( "error", [&]( std::error_code err ) mutable
		{
			REQUIRE( !err );
		} );
	} );

	server->on( "error", [&]( std::error_code err ) mutable
	{
		CHECK( !err );
		done = true;
	} );

	client->on( "connect", [&]() mutable
	{
		client_events.push_back( "connect" );

		client->write( message );

		client->on( "data", [&]( buffer b ) mutable
		{
			client_events.push_back( "data" );
			CHECK( b.to_string() == message );
			done = true;
		} );
	} );

	client->on( "drain", [&]() mutable
	{
		client_events.push_back( "drain" );
	} );

	client->on( "error", [&]( std::error_code err ) mutable
	{
		CHECK( !err );
		done = true;
	} );

	while ( !done )
	{
		runloop::shared().run( runloop::mode_t::once );
	}

	REQUIRE( server_events.size() == 3 );
	REQUIRE( server_events[ 0 ] == "listening" );
	REQUIRE( server_events[ 1 ] == "connection" );
	REQUIRE( server_events[ 2 ] == "data" );
	REQUIRE( client_events.size() == 3 );
	REQUIRE( client_events[ 0 ] == "connect" );
	REQUIRE( client_events[ 1 ] == "drain" );
	REQUIRE( client_events[ 2 ] == "data" );
}