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
 
#ifndef _nodeoze_socket_libuv_h
#define _nodeoze_socket_libuv_h

#include <nodeoze/nsocket.h>
#include <nodeoze/nendpoint.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nbuffer.h>
#include <nodeoze/nstream.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nmacros.h>
#include "nendpoint_libuv.h"
#include "nerror_libuv.h"
#include <functional>
#include <queue>
#include <errno.h>
#include <uv.h>

#if defined( WIN32 )

#include <win/stream-inl.h>

int
uv_tcp_open_stream(uv_tcp_t* handle, uv_os_sock_t sock)
{
  int r = uv_tcp_open(handle, sock);
  if (r == 0) {
    uv_connection_init((uv_stream_t*)handle);
    handle->flags |= UV_HANDLE_BOUND | UV_HANDLE_READABLE | UV_HANDLE_WRITABLE;
  }

  return r;
}

#else

# define uv_tcp_open_stream uv_tcp_open

#endif



namespace nodeoze {

namespace ip {

namespace tcp {

class socket::handle : protected uv_tcp_s
{
public:

	typedef std::function< void ( nodeoze::err_t err, buffer &buf ) >	recv_reply_f;
	
	handle( tcp::socket *owner )
	{
		auto err = uv_tcp_init( uv_default_loop(), this );
		ncheck_error( err == 0, exit, "uv_tcp_init() failed: %", uv_strerror( err ) );
		
		mlog( marker::socket_tcp, log::level_t::info, "opened 0x%", this );
	
		this->data = owner;
	
	exit:

		return;
	}
	
	handle( tcp::socket *owner, ssocket::native_t fd )
	{
		auto err = uv_tcp_init( uv_default_loop(), this );
		ncheck_error( err == 0, exit, "uv_tcp_init() failed: %", uv_strerror( err ) );

		err = uv_tcp_open_stream( this, fd );
		ncheck_error( err == 0, exit, "uv_tcp_open() failed: %", uv_strerror( err ) );
		
		mlog( marker::socket_tcp, log::level_t::info, "opened 0x%", this );
	
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
	
	inline tcp::socket*
	owner() const
	{
		return reinterpret_cast< nodeoze::ip::tcp::socket* >( this->data );
	}

	inline void
	set_owner( tcp::socket *owner )
	{
		this->data = owner;
	}
	
	inline void
	connect( const ip::endpoint &to, promise< void > ret )
	{
		sockaddr_storage addr;
		
		ip::endpoint_to_sockaddr( to, addr );
	
		auto request	= new connect_s( this, to, ret );
		auto err		= uv_tcp_connect( request, this, reinterpret_cast< sockaddr* >( &addr ), reinterpret_cast< uv_connect_cb >( on_connect ) );
		ncheck_error_action( err == 0, on_connect( request, err ), exit, "uv_tcp_connect() to % failed (%)", to.to_string(), uv_strerror( err ) );
	
	exit:
	
		return;
	}
	
	inline void
	send( buffer buf, promise< void > ret )
	{
		if ( buf.size() > 0 )
		{
			auto request = new write_s( this, buf, ret );
			auto err = uv_write( request, reinterpret_cast< uv_stream_t* >( this ), &request->m_uv_buf, 1, reinterpret_cast< uv_write_cb >( on_send ) );
			ncheck_error_action( err == 0, on_send( request, err ), exit, "uv_write() failed: %", uv_strerror( err ) );
		}
		
	exit:
		
		return;
	}

	inline void
	recv()
	{
		auto err = uv_read_start( reinterpret_cast< uv_stream_t* >( this ), reinterpret_cast< uv_alloc_cb >( on_alloc ), reinterpret_cast< uv_read_cb >( on_recv ) );
		ncheck_error_action( err == 0, on_recv( this, err, nullptr ), exit, "uv_read_start() failed: %", uv_strerror( err ) );
		mlog( marker::socket_tcp, log::level_t::info, "starting recv on handle % with peer %", this, peer() );
	
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
		mlog( marker::socket_tcp, log::level_t::info, "closed 0x%", this );
		
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
		write_s( socket::handle *owner, buffer &buf, promise< void > ret )
		:
			m_buf( std::move( buf ) ),
			m_ret( ret )
		{
			handle			= reinterpret_cast< uv_stream_t* >( owner );
			m_uv_buf.base	= reinterpret_cast< char* >( m_buf.data() );
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
	on_connect( connect_s *request, int err )
	{
		socket::handle	*stream	= nullptr;
		ip::tcp::socket *owner	= nullptr;
		std::error_code	error;
		
		ncheck_error_quiet( request, exit );
		stream = reinterpret_cast< socket::handle* >( request->handle );
		ncheck_error_action_quiet( stream, request->m_ret.reject( std::make_error_code( std::errc::not_connected ), reject_context ), exit );
		owner = reinterpret_cast< ip::tcp::socket* >( stream->data );
		ncheck_error_action_quiet( owner, request->m_ret.reject( std::make_error_code( std::errc::not_connected ), reject_context ), exit );
		
		if ( err != 0 )
		{
			error = std::error_code( err, libuv::error_category() );
		}
		
		owner->on_connect( error, request->m_ret );
		
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
		ip::tcp::socket	*owner;
		std::error_code	error;
		
		ncheck_error_quiet( request, exit );
		stream = request->handle;
		ncheck_error_action_quiet( stream, request->m_ret.reject( std::make_error_code( std::errc::not_connected ), reject_context ), exit );
		owner = reinterpret_cast< ip::tcp::socket* >( stream->data );
		ncheck_error_action_quiet( owner, request->m_ret.reject( std::make_error_code( std::errc::not_connected ), reject_context ), exit );
		
		if ( err != 0 )
		{
			error = std::error_code( err, libuv::error_category() );
		}
		
		owner->on_send( error, request->m_ret );
		
	exit:

		if ( request )
		{
			delete request;
		}
	}

	inline static void
	on_alloc( ip::tcp::socket::handle *self, size_t size_hint, uv_buf_t *buf )
	{
		ncheck_error( self, exit, "self is null" );
		ncheck_error( buf, exit, "buf is null" );
		
		self->m_recv_buf.capacity( size_hint );
		
		buf->base	= reinterpret_cast< char* >( self->m_recv_buf.data() );
	#if defined( WIN32 )
		buf->len	= static_cast< ULONG >( size_hint );
	#else
		buf->len	= size_hint;
	#endif
		
	exit:

		return;
	}

	inline static void
	on_recv( ip::tcp::socket::handle *self, std::int32_t nread, const uv_buf_t *buf )
	{
		nunused( buf );

		ip::tcp::socket	*owner;
		std::error_code	error;
		
		ncheck_error( self, exit, "self is null" );
		owner = reinterpret_cast< ip::tcp::socket* >( self->data );
		ncheck_error( owner, exit, "owner is null" );
		
		if ( nread > 0 )
		{
			self->m_recv_buf.size( nread );
		}
		else if ( nread < 0 )
		{
			error = std::error_code( nread, libuv::error_category() );
			self->m_recv_buf.size( 0 );
		}

		owner->on_recv( error, self->m_recv_buf );
		
	exit:

		return;
	}

	buffer m_recv_buf;
};


class acceptor::handle : protected uv_tcp_s
{
public:

	handle( acceptor *owner )
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
	
	inline acceptor*
	owner() const
	{
		return reinterpret_cast< nodeoze::ip::tcp::acceptor* >( this->data );
	}

	inline void
	set_owner( acceptor *owner )
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
	accept( std::size_t qsize, accept_reply_f reply )
	{
		assert( owner() );
		
		m_reply = reply;
		
		auto error = std::error_code( uv_listen( reinterpret_cast< uv_stream_t* >( this ), static_cast< int >( qsize ), reinterpret_cast< uv_connection_cb >( on_accept ) ), libuv::error_category() );
		ncheck_error_action( !error, owner()->on_accept( error, tcp::socket(), reply ), exit, "uv_listen() on % failed (%)", m_name.to_string(), error );
		
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
	on_accept( acceptor::handle *self, int err )
	{
		auto	handle	= static_cast< socket::handle* >( nullptr );
		auto	error	= std::error_code( err, libuv::error_category() );
		
		ncheck_error( self, exit, "self is null" );
		ncheck_error( self->owner(), exit, "self->owner is null" );
		ncheck_error( err == 0, exit, "on_accept() failed: %", uv_strerror( err ) );
		
		handle = new socket::handle( nullptr );
		err = uv_accept( reinterpret_cast< uv_stream_t* >( self ), reinterpret_cast< uv_stream_t* >( handle ) );
		ncheck_error( !error, exit, "uv_accept() failed (%)", error );
		
		self->owner()->on_accept( error, handle, self->m_reply );
		handle = nullptr;
		
	exit:

		if ( handle )
		{
			handle->close();
		}
	}

	ip::endpoint		m_name;
	accept_reply_f		m_reply;
};

}

namespace udp {

class socket::handle : protected uv_udp_s
{
public:

	typedef std::function< void ( nodeoze::err_t err, const ip::endpoint &from, buffer buf ) >	recv_reply_f;

	handle( udp::socket *owner )
	{
		auto err = uv_udp_init( uv_default_loop(), this );
		ncheck_error( err == 0, exit, "uv_udp_init() failed: %", uv_strerror( err ) );
	
		this->data = owner;
	
	exit:

		return;
	}
	
	handle( const udp::socket &rhs ) = delete;
	
	handle( udp::socket &&rhs ) = delete;
	
	handle&
	operator=( const handle &rhs ) = delete;
	
	handle&
	operator=( handle &&rhs ) = delete;
	
	inline udp::socket*
	owner() const
	{
		return reinterpret_cast< udp::socket* >( this->data );
	}

	inline void
	set_owner( udp::socket *owner )
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
	set_membership( const ip::endpoint &endpoint, const ip::address iface, membership_t membership )
	{
		auto err = std::error_code();
		
		err = std::error_code( uv_udp_set_membership( this, endpoint.addr().to_string().c_str(), iface.to_string().c_str(), membership == membership_t::join ? UV_JOIN_GROUP : UV_LEAVE_GROUP ), libuv::error_category() );
		ncheck_error_quiet( !err, exit );
		if ( membership == membership_t::join )
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
			
			m_uv_buf.base	= reinterpret_cast< char* >( m_buf.data() );
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
		socket::handle	*self;
		auto			error = std::error_code( err, libuv::error_category() );
		
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
		
		buf->base	= reinterpret_cast< char* >( self->m_recv_buf.data() );
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
			self->owner()->on_recv( std::error_code(), from, buffer( buf->base, nread, nread, buffer::do_not_delete_data ) );
		}
		else if ( nread < 0 )
		{
			self->owner()->on_recv( std::error_code( nread, libuv::error_category() ), from, buffer() );
			/*
			else if ( nread == UV__EOF )
			{
				self->owner()->on_recv( err_t::eof, from, buffer() );
			}
			else if ( nread != 0 )
			{
				nlog( log::level_t::error, "recv error % (%)", nread, uv_strerror( static_cast< int >( nread ) ) );
			
				self->owner()->on_recv( err_t::network_error, from, buffer() );
			}
		*/
		}
		
	exit:

		return;
	}

	buffer m_recv_buf;
};

}

}

}


	
#endif
