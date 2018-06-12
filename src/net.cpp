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
#	pragma mark net::tcp::socket::handle implementation
#endif

class net::tcp::socket::handle : protected uv_tcp_s
{
public:

	handle( tcp::socket *owner )
	{
		auto err = uv_tcp_init( uv_default_loop(), this );
		ncheck_error_quiet( err == 0, exit );

		m_recv_buf.make_no_copy_on_write();
		
		this->data = owner;

	exit:

		return;
	}
	
	handle( const handle &rhs ) = delete;
	
	handle( handle &&rhs ) = delete;

	handle&
	operator=( const handle &rhs ) = delete;
	
	handle&
	operator=( handle &&rhs ) = delete;
	
	inline socket*
	owner() const
	{
		return reinterpret_cast< socket* >( this->data );
	}

	inline void
	set_owner( socket *owner )
	{
		this->data = owner;
	}
	
	inline void
	connect( ip::endpoint to, promise< void > ret )
	{
		sockaddr_storage addr;
		
		ip::endpoint_to_sockaddr( to, addr );
	
		auto request	= new connect_s( this, to, std::move( ret ) );
		auto err		= std::error_code( uv_tcp_connect( request, this, reinterpret_cast< sockaddr* >( &addr ), reinterpret_cast< uv_connect_cb >( on_connect ) ), libuv::error_category() );
		ncheck_error_action_quiet( !err, on_connect( request, err ), exit );
	
	exit:
	
		return;
	}
	
	inline void
	write( buffer buf, promise< void > ret )
	{
		if ( buf.size() > 0 )
		{
			auto request	= new write_s( this, std::move( buf ), std::move( ret ) );
			auto err		= uv_write( request, reinterpret_cast< uv_stream_t* >( this ), &request->m_uv_buf, 1, reinterpret_cast< uv_write_cb >( on_send ) );
			ncheck_error_action_quiet( !err, on_send( request, err ), exit );
		}
		else
		{
			ret.resolve();
		}
		
	exit:
		
		return;
	}

	inline void
	read_start()
	{
		auto err = std::error_code( uv_read_start( reinterpret_cast< uv_stream_t* >( this ), reinterpret_cast< uv_alloc_cb >( on_alloc ), reinterpret_cast< uv_read_cb >( on_recv ) ), libuv::error_category() );
		ncheck_error_action_quiet( !err, owner()->emit( "error", err ), exit );
	
	exit:

		return;
	}

	inline void
	read_stop()
	{
		auto err = std::error_code( uv_read_stop( reinterpret_cast< uv_stream_t* >( this ) ), libuv::error_category() );
		ncheck_error_action_quiet( !err, owner()->emit( "error", err ), exit );

	exit:

		return;
	}


	inline void
	set_keep_alive( bool val )
	{
		uv_tcp_keepalive( this, val, 0 );
	}

	inline ip::endpoint
	name() const
	{
		sockaddr_storage	addr;
		int					len;
		ip::endpoint		ret;

		memset( &addr, 0, sizeof( addr ) );
		len = sizeof( sockaddr_storage );
		uv_tcp_getsockname( this, reinterpret_cast< sockaddr* >( &addr ), &len );

		ip::sockaddr_to_endpoint( addr, ret );

		return ret;
	}

	inline ip::endpoint
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

	inline void
	close()
	{
		this->data = nullptr;

		uv_close( reinterpret_cast< uv_handle_t* >( this ), []( uv_handle_t *self ) mutable
		{
			delete reinterpret_cast< handle* >( self );
		} );
	}

private:

	struct connect_s : public uv_connect_s
	{
		connect_s( socket::handle *owner, const nodeoze::ip::endpoint &dest, promise< void > ret )
		:
			m_dest( dest ),
			m_ret( ret )
		{
			handle = reinterpret_cast< uv_stream_t* >( owner );
		}
		
		nodeoze::ip::endpoint	m_dest;
		promise< void >			m_ret;
	};

	struct write_s : public uv_write_t
	{
		write_s( socket::handle *owner, buffer buf, promise< void > ret )
		:
			m_buf( std::move( buf ) ),
			m_ret( std::move( ret ) )
		{
			handle			= reinterpret_cast< uv_stream_t* >( owner );
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

	inline ~handle()
	{
	}
	
	inline static void
	on_connect( connect_s *request, std::error_code err )
	{
		socket::handle	*stream	= nullptr;
		socket			*owner	= nullptr;

		assert( request );
		ncheck_error_quiet( request, exit );
		stream = reinterpret_cast< socket::handle* >( request->handle );
		assert( stream );
		ncheck_error_action_quiet( stream, request->m_ret.reject( make_error_code( std::errc::not_connected ), reject_context ), exit );
		owner = reinterpret_cast< socket* >( stream->data );
		assert( owner );
		ncheck_error_action_quiet( owner, request->m_ret.reject( make_error_code( std::errc::not_connected ), reject_context ), exit );

		if ( !err )
		{
			owner->emit( "connect" );
		}
		else
		{
			owner->emit( "error", err );
		}
		
	exit:

		if ( request )
		{
			delete request;
		}
	}

	inline static void
	on_send( write_s *request, int err )
	{
		uv_stream_s		*stream;
		socket			*owner;
		
		ncheck_error_quiet( request, exit );
		stream = request->handle;
		ncheck_error_action_quiet( stream, request->m_ret.reject( make_error_code( std::errc::not_connected ), reject_context ), exit );
		owner = reinterpret_cast< socket* >( stream->data );
		ncheck_error_action_quiet( owner, request->m_ret.reject( make_error_code( std::errc::not_connected ), reject_context ), exit );

		if ( !err )
		{
			request->m_ret.resolve();
			owner->emit( "write" );
		}
		else
		{
			request->m_ret.reject( std::error_code( err, libuv::error_category() ), reject_context );
			owner->emit( "error", std::error_code( err, libuv::error_category() ) );
		}
		
	exit:

		if ( request )
		{
			delete request;
		}
	}

	inline static void
	on_alloc( handle *self, size_t size_hint, uv_buf_t *buf )
	{
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
	on_recv( handle *self, std::int32_t nread, const uv_buf_t *buf )
	{
		nunused( buf );

		socket			*owner;
		std::error_code	err;
		
		assert( self );
		ncheck_error_quiet( self, exit );
		owner = reinterpret_cast< socket* >( self->data );
		assert( owner );
		ncheck_error_quiet( owner, exit );
		
		if ( nread > 0 )
		{
			self->m_recv_buf.size( nread );

	fprintf( stderr, "size of buf: %d\n", self->m_recv_buf.size() );
			owner->emit( "data", self->m_recv_buf );
		}
		else if ( nread < 0 )
		{
			err = std::error_code( nread, libuv::error_category() );
			owner->emit( "error", err );
			self->m_recv_buf.size( 0 );
		}

	exit:

		return;
	}

	buffer m_recv_buf;
};

#if defined( __APPLE__ )
#	pragma mark net::tcp::server::handle implementation
#endif

class net::tcp::server::handle : protected uv_tcp_s
{
public:

	handle( server *owner )
	{
		auto err = uv_tcp_init( uv_default_loop(), this );
		ncheck_error( err == 0, exit, "uv_tcp_init() failed; %", uv_strerror( err ) );
		assert( this->type == UV_TCP );
		this->data = owner;
	
	exit:

		return;
	}
	
	handle( const handle &rhs ) = delete;
	
	handle( handle &&rhs ) = delete;

	handle&
	operator=( const handle& rhs ) = delete;
	
	handle&
	operator=( handle &&rhs ) = delete;
	
	inline server*
	owner() const
	{
		return reinterpret_cast< nodeoze::net::tcp::server* >( this->data );
	}

	inline void
	set_owner( server *owner )
	{
		this->data = owner;
	}

	inline std::error_code
	bind( const nodeoze::ip::endpoint &name )
	{
		assert( owner() );
		
		sockaddr_storage	addr;
		int					len;
		std::error_code		err;
		
		ip::endpoint_to_sockaddr( name, addr );
		
		err = std::error_code( uv_tcp_bind( this, reinterpret_cast< sockaddr* >( &addr ), 0 ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		
		len = sizeof( addr );
		err = std::error_code( uv_tcp_getsockname( this, reinterpret_cast< sockaddr* >( &addr ), &len ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		
		ip::sockaddr_to_endpoint( addr, m_name );
		
	exit:
	
		return err;
	}

	inline void
	listen( std::size_t qsize )
	{
		assert( owner() );
		
		auto err = std::error_code( uv_listen( reinterpret_cast< uv_stream_t* >( this ), static_cast< int >( qsize ), reinterpret_cast< uv_connection_cb >( on_accept ) ), libuv::error_category() );
		ncheck_error_action( !err, owner()->emit( "error", err ), exit, "uv_listen() on % failed (%)", m_name.to_string(), err );

		owner()->emit( "listening" );
		
	exit:

		return;
	}

	inline ip::endpoint
	name() const
	{
		return m_name;
	}

	inline void
	close()
	{
		assert( this->type == UV_TCP );

		uv_close( reinterpret_cast< uv_handle_t* >( this ), []( uv_handle_t *self )
		{
			delete reinterpret_cast< handle* >( self );
		} );
	}

private:

	~handle()
	{
		nlog( log::level_t::debug, "" );
	}

	inline static void
	on_accept( handle *self, int libuv_err )
	{
		auto	handle	= static_cast< socket::handle* >( nullptr );
		auto	err		= std::error_code( libuv_err, libuv::error_category() );
		
		ncheck_error_quiet( self, exit );
		ncheck_error_quiet( self->owner(), exit );
		ncheck_error_action_quiet( !err, self->owner()->emit( "error", err ), exit );
		
		handle = new socket::handle( nullptr );
		err = std::error_code( uv_accept( reinterpret_cast< uv_stream_t* >( self ), reinterpret_cast< uv_stream_t* >( handle ) ), libuv::error_category() );
		ncheck_error_action_quiet( !err, self->owner()->emit( "error", err ), exit );

		self->owner()->emit( "connection", std::make_shared< net::tcp::socket >( handle ) );

		handle = nullptr;
		
	exit:

		if ( handle )
		{
			handle->close();
		}
	}

	ip::endpoint m_name;
};

#if defined( __APPLE__ )
#	pragma mark net::udp::socket::handle implementation
#endif

class net::udp::socket::handle : protected uv_udp_s
{
public:

	handle( socket *owner )
	{
		auto err = uv_udp_init( uv_default_loop(), this );
		ncheck_error( err == 0, exit, "uv_udp_init() failed: %", uv_strerror( err ) );
	
		this->data = owner;
	
	exit:

		return;
	}
	
	handle( const handle &rhs ) = delete;
	
	handle( handle &&rhs ) = delete;
	
	handle&
	operator=( const handle &rhs ) = delete;
	
	handle&
	operator=( handle &&rhs ) = delete;
	
	inline socket*
	owner() const
	{
		return reinterpret_cast< socket* >( this->data );
	}

	inline void
	set_owner( socket *owner )
	{
		this->data = owner;
	}

	inline std::error_code
	bind( const ip::endpoint &name )
	{
		assert( owner() );
		
		struct sockaddr_storage addr;
		std::error_code			err;
		
		ip::endpoint_to_sockaddr( name, addr );
		
		err = std::error_code( uv_udp_bind( this, reinterpret_cast< sockaddr* >( &addr ), UV_UDP_REUSEADDR ), libuv::error_category() );
		ncheck_error( !err, exit, "uv_udp_bind() to % failed % (%)", name.to_string(), err.value(), err.message() );
		
		err = std::error_code( uv_udp_recv_start( this, reinterpret_cast< uv_alloc_cb >( on_alloc ), reinterpret_cast< uv_udp_recv_cb >( on_recv ) ), libuv::error_category() );
		ncheck_error( !err, exit, "uv_udp_recv_start() on % failed % (%)", name.to_string(), err.value(), err.message() );
		
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
		uv_udp_getsockname( this, reinterpret_cast< sockaddr* >( &addr ), &len );
		
		ip::sockaddr_to_endpoint( addr, ret );
		
		return ret;
	}
	
	inline std::error_code
	set_membership( const ip::endpoint &endpoint, const ip::address iface, multicast_membership_type membership )
	{
		auto err = std::error_code();
		
		err = std::error_code( uv_udp_set_membership( this, endpoint.addr().to_string().c_str(), iface.to_string().c_str(), membership == multicast_membership_type::join ? UV_JOIN_GROUP : UV_LEAVE_GROUP ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		if ( membership == multicast_membership_type::join )
		{
			err = std::error_code( uv_udp_set_multicast_loop( this, 1 ), libuv::error_category() );
			ncheck_error_quiet( !err, exit );
		}

	exit:

		return err;
	}
	
	inline std::error_code
	set_broadcast( bool val )
	{
		auto err = std::error_code( uv_udp_set_broadcast( this, val ), libuv::error_category() );
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
		
		auto request	= new write_s( this, buf, ret );
		auto err		= uv_udp_send( request, this, &request->m_uv_buf, 1, reinterpret_cast< sockaddr* >( &addr ), reinterpret_cast< uv_udp_send_cb >( on_send ) );
		ncheck_error_action( err == 0, on_send( request, err ), exit, "uv_udp_send() to % failed (%)", to.to_string(), uv_strerror( err ) );
		
	exit:

		return;
	}

	inline void
	close()
	{
		uv_close( reinterpret_cast< uv_handle_t* >( this ), []( uv_handle_t *self )
		{
			delete reinterpret_cast< handle* >( self );
		} );
	}

protected:

	struct write_s : public uv_udp_send_s
	{
		write_s( socket::handle *owner, buffer &buf, promise< void > ret )
		:
			m_buf( std::move( buf ) ),
			m_ret( ret )
		{
			handle			= owner;
			
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

	~handle()
	{
	}
	
	inline static void
	on_send( write_s *request, int err )
	{
		handle	*self;
		auto	error = std::error_code( err, libuv::error_category() );
		
		ncheck_error( request, exit, "request is null" );
		self = static_cast< socket::handle* >( request->handle );
		ncheck_error( self, exit, "self is null" );
		ncheck_error( self->owner(), exit, "self->owner is null" );
		
		self->owner()->on_send( error, request->m_ret );
		
	exit:

		if ( request )
		{
			delete request;
		}
	}

	inline static void
	on_alloc( socket::handle *self, size_t size_hint, uv_buf_t *buf )
	{
		ncheck_error( self, exit, "self is null" );
		ncheck_error( buf, exit, "buf is null" );
		
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
	on_recv( socket::handle *self, std::int32_t nread, const uv_buf_t *buf, const struct sockaddr *addr, unsigned flags )
	{
		nunused( flags );
		
		ip::endpoint from;
		
		ncheck_error( self, exit, "self is null" );
		ncheck_error( self->owner(), exit, "self->owner is null" );
		
		if ( addr )
		{
			ip::sockaddr_to_endpoint( *reinterpret_cast< const sockaddr_storage* >( addr ), from );
		}
		
		if ( nread > 0 )
		{
//			self->owner()->on_recv( std::error_code(), from, buffer( buf->base, nread, nread, buffer::do_not_delete_data ) );
			self->owner()->on_recv( std::error_code(), from, buffer( buf->base, nread, buffer::policy::exclusive, nullptr, nullptr ) );
		}
		else if ( nread < 0 )
		{
			self->owner()->on_recv( std::error_code( nread, libuv::error_category() ), from, buffer() );
		}
		
	exit:

		return;
	}

	buffer m_recv_buf;
};

#if defined( __APPLE__ )
#	pragma mark net::tcp::server implementation
#endif

net::tcp::server::server()
:
	m_handle( nullptr )
{
}


net::tcp::server::~server()
{
}


promise< void >
net::tcp::server::listen( ip::endpoint endpoint, std::size_t qsize )
{
	auto ret = promise< void >();

	m_handle = new net::tcp::server::handle( this );

	auto err = m_handle->bind( endpoint );

	if ( err )
	{
		emit( "error", err );
		ret.reject( err, reject_context );
		goto exit;
	}

	m_handle->listen( qsize );

	ret.resolve();

exit:

	return ret;
}


ip::endpoint
net::tcp::server::name() const
{
	auto ret = ip::endpoint();

	if ( m_handle )
	{
		ret = m_handle->name();
	}

	return ret;
}


void
net::tcp::server::close()
{
}


#if defined( __APPLE__ )
#	pragma mark net::tcp::socket implementation
#endif

net::tcp::socket::socket()
:
	m_handle( nullptr )
{
}


net::tcp::socket::socket( native_type fd )
{
}


net::tcp::socket::socket( socket &&rhs )
:
	m_handle( rhs.m_handle )
{
	if ( m_handle )
	{
		m_handle->set_owner( this );

		rhs.m_handle = nullptr;
	}
}


net::tcp::socket::socket( handle *h )
:
	m_handle( h )
{
	m_handle->set_owner( this );
}


net::tcp::socket::~socket()
{
	if ( m_handle )
	{
		m_handle->close();
		m_handle = nullptr;
	}
}


promise< void >
net::tcp::socket::connect( ip::endpoint endpoint )
{
	auto ret = promise< void >();

	m_handle = new handle( this );

	m_handle->connect( std::move( endpoint ), ret );

	return ret;
}


promise< void >
net::tcp::socket::really_write( buffer b )
{
	auto ret = promise< void >();

	if ( m_handle )
	{
		m_handle->write( std::move( b ), ret );
	}
	else
	{
		ret.reject( make_error_code( std::errc::invalid_argument ), reject_context );
	}

	return ret;
}


void
net::tcp::socket::really_read()
{
	assert( m_handle );

	if ( m_handle )
	{
		m_handle->read_start();
	}
	else
	{
		emit( "error", std::error_code( UV_EFAULT, libuv::error_category() ) );
	}
}


void
net::tcp::socket::really_pause()
{
	assert( m_handle );

	if ( m_handle )
	{
		m_handle->read_stop();
	}
	else
	{
		emit( "error", std::error_code( UV_EFAULT, libuv::error_category() ) );
	}
}

#if defined( __APPLE__ )
#	pragma mark net::udp::socket implementation
#endif

net::udp::socket::socket()
{
}


net::udp::socket::socket( native_type fd )
{
}


promise< void >
net::udp::socket::really_write( buffer b )
{
}


void
net::udp::socket::really_read()
{
}


void
net::udp::socket::really_pause()
{
	assert( m_handle );

	if ( m_handle )
	{
		// m_handle->read_stop();
	}
	else
	{
		emit( "error", std::error_code( UV_EFAULT, libuv::error_category() ) );
	}
}

TEST_CASE( "nodeoze/smoke/net/tcp")
{
	auto message									= std::string( "this is one small step for man" );
	auto server_events 								= std::vector< std::string >();
	auto client_events 								= std::vector< std::string >();
	auto name										= ip::endpoint( "127.0.0.1", 5000 );
	net::tcp::server								server;
	deque< std::shared_ptr< net::tcp::socket > >	connections;
	net::tcp::socket								sock;
	bool											done = false;

	server.on( "listening", [&]() mutable
	{
fprintf( stderr, "listening\n" );
		server_events.push_back( "listening" );
	} );

	server.on( "connection", [&]( std::shared_ptr< net::tcp::socket > sock ) mutable
	{
		server_events.push_back( "connection" );

		connections.emplace_back( std::move( sock ) );

		connections.back()->on( "data", [&]( buffer buf ) mutable
		{
			server_events.push_back( "data" );

	fprintf( stderr, "size: %d\n", buf.size() );
	fprintf( stderr, "buf: %s\n", buf.to_string().c_str() );

			REQUIRE( buf.to_string() == message );

			done = true;
		} );

		connections.back()->on( "error", [&]( std::error_code err ) mutable
		{
			REQUIRE( !err );
		} );
	} );

	server.on( "error", [&]( std::error_code err ) mutable
	{
		CHECK( !err );
		done = true;
	} );

	server.listen( name, 5 );

	sock.on( "connect", [&]() mutable
	{
fprintf( stderr, "connect\n" );
		client_events.push_back( "connect" );

		sock.write( message );
	} );

	sock.on( "drain", [&]() mutable
	{
		client_events.push_back( "drain" );
	} );

	sock.on( "error", [&]( std::error_code err ) mutable
	{
		CHECK( !err );
		done = true;
	} );

	sock.connect( name );

	while ( !done )
	{
		runloop::shared().run( runloop::mode_t::once );
	}

	REQUIRE( server_events.size() == 2 );
	REQUIRE( server_events[ 0 ] == "listening" );
	REQUIRE( server_events[ 1 ] == "connection" );
	REQUIRE( client_events.size() == 1 );
	REQUIRE( client_events[ 0 ] == "connect" );
}