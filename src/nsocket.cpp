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
 
#include <nodeoze/nsocket.h>
#include <nodeoze/nproxy.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nnotification.h>
#include <nodeoze/ncompat.h>
#include <nodeoze/nlog.h>
#include <nodeoze/nmarkers.h>
#include "libuv/nsocket_libuv.h"
#include <thread>

#if defined( WIN32 )
#	include <WinSock2.h>
#	define	CLOSE_SOCKET( X)	::closesocket( X )
#else
#	include <sys/socket.h>
#	include <arpa/inet.h>
#	include <netinet/in.h>
#	include <netinet/tcp.h>
#	include <netdb.h>
#	include <fcntl.h>
#	define	CLOSE_SOCKET( X)	::close( X )
#	define INVALID_SOCKET -1
#endif

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark ip::tcp::ssocket implementation
#endif

ip::tcp::ssocket::ssocket( const ip::endpoint &to, std::chrono::milliseconds timeout )
:
	m_to( to ),
	m_timeout( timeout ),
	m_fd( INVALID_SOCKET )
{
}


ip::tcp::ssocket::ssocket( ssocket &&rhs )
{
	rhs.m_fd = m_fd;
	m_fd = INVALID_SOCKET;
}


ip::tcp::ssocket::~ssocket()
{
	close();
}


ip::tcp::ssocket&
ip::tcp::ssocket::operator=( ssocket &&rhs )
{
	rhs.m_fd = m_fd;
	m_fd = INVALID_SOCKET;
	
	return *this;
}


std::error_code
ip::tcp::ssocket::connect()
{
	assert( m_fd == INVALID_SOCKET );

	auto addr	= sockaddr_in();
	auto err 	= std::error_code();
	
	m_fd = ::socket( AF_INET, SOCK_STREAM, 0 );
	
	memset( &addr, 0, sizeof( addr ) );
	addr.sin_family			= AF_INET;
	m_to.addr() >> addr.sin_addr.s_addr;
	addr.sin_port			= htons( m_to.port() );
	
#if defined( WIN32 )
	DWORD on = 1;
	unsigned long mode = 1;
	auto ret = ioctlsocket( m_fd, FIONBIO, &mode );
	setsockopt( m_fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast< char* >( &on ), sizeof( int ) );
	on = 1;
	setsockopt( m_fd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast< char* >( &on ), sizeof( int ) );
#else
	int on = 1;
	auto ret = fcntl( m_fd, F_SETFL, fcntl( m_fd, F_GETFL, 0 ) | O_NONBLOCK );
	setsockopt( m_fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof( int ) );
	on = 1;
	setsockopt( m_fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof( int ) );
#endif

	if ( ret == 0 )
	{
		ret = ::connect( m_fd, reinterpret_cast< sockaddr* >( &addr ), sizeof( addr ) );

		if ( ret == -1 )
		{
#if defined( WIN32 )
			if ( ::WSAGetLastError() == WSAEWOULDBLOCK )
#else
			if ( errno == EINPROGRESS )
#endif
			{
				timeval timeout = { 2, 0 };
				fd_set write_fds;
				
				FD_ZERO( &write_fds );
				FD_SET( m_fd, &write_fds );
				
				ret = select( m_fd + 1, nullptr, &write_fds, nullptr, &timeout );
				
				if ( ret == 0 )
				{
					close();
					err = make_error_code( std::errc::timed_out );
				}
				else if ( ret < 0 )
				{
					close();
					err = std::error_code( errno, std::generic_category() );
				}
			}
			else
			{
				close();
				err = std::error_code( errno, std::generic_category() );
			}
		}
	}
	else
	{
		close();
		err = std::error_code( errno, std::generic_category() );
	}
	
	return err;
}

	
std::error_code
ip::tcp::ssocket::send( buffer buf, std::chrono::milliseconds timeout )
{
	nunused( timeout );

	auto err = std::error_code();
	
	while ( !err )
	{
#if defined( WIN32 )
		auto num = ::send( m_fd, reinterpret_cast< const char* >( buf.data() ), buf.size(), 0 );
#else
		auto num = ::send( m_fd, buf.const_data(), buf.size(), 0 );
#endif
		
		if ( num > 0 )
		{
			break;
		}
		else
		{
			close();
			err = connect();
		}
	}
	
	return err;
}

	
std::error_code
ip::tcp::ssocket::recv( buffer &buf, std::chrono::milliseconds timeout )
{
	auto read_fds	= fd_set();
	auto tv			= timeval();
	auto err		= std::error_code();
	
	FD_ZERO( &read_fds );
	FD_SET( m_fd, &read_fds );
	
	tv.tv_sec  = timeout.count() / 1000;
	tv.tv_usec = ( timeout.count() % 1000 ) * 1000;
	
	auto num = select( m_fd + 1, &read_fds, nullptr, nullptr, &tv );
	
	if ( num == 1 )
	{
#if defined( WIN32 )
		num = ::recv( m_fd, reinterpret_cast< char* >( buf.data() ), buf.capacity(), 0 );
#else
		num = ::recv( m_fd, buf.mutable_data(), buf.capacity(), 0 );
#endif
				
		if ( num > 0 )
		{
			buf.size( num );
		}
		else if ( num == 0 )
		{
			err = make_error_code( std::errc::connection_reset );
		}
		else
		{
			err = std::error_code( errno, std::generic_category() );
		}
	}
	else
	{
		err = make_error_code( std::errc::timed_out );
	}
	
	return err;
}

	
void
ip::tcp::ssocket::close()
{
	if ( m_fd != INVALID_SOCKET )
	{
		CLOSE_SOCKET( m_fd );
		m_fd = INVALID_SOCKET;
	}
}


ip::tcp::ssocket::native_t
ip::tcp::ssocket::release()
{
	auto ret = m_fd;
	
	m_fd = INVALID_SOCKET;
	
	return ret;
}


#if defined( __APPLE__ )
#	pragma mark ip::tcp::socket implementation
#endif

const std::string	ip::tcp::socket::close_event	= "ip_tcp_socket_close_event";
std::uint64_t		ip::tcp::socket::m_next_tag		= 1;

ip::tcp::socket::socket()
:
	m_state( stream::state_t::disconnected ),
	m_handle( nullptr ),
	m_tag( m_next_tag++ )
{
	mlog( marker::socket_tcp, log::level_t::info, "%", this );
}


ip::tcp::socket::socket( handle *h )
:
	m_state( stream::state_t::connected ),
	m_handle( h ),
	m_tag( m_next_tag++ )
{
	mlog( marker::socket_tcp, log::level_t::info, "%", this );
	start_recv();
}


ip::tcp::socket::socket( ssocket &s )
:
	m_state( stream::state_t::connected ),
	m_handle( new socket::handle( this, s.release() ) ),
	m_tag( m_next_tag++ )
{
	mlog( marker::socket_tcp, log::level_t::info, "%", this );
	start_recv();
}


ip::tcp::socket::socket( socket &&rhs )
:
	m_handle( nullptr )
{
	mlog( marker::socket_tcp, log::level_t::info, "%", this );
	operator=( std::move( rhs ) );
}


ip::tcp::socket::~socket()
{
	mlog( marker::socket_tcp, log::level_t::info, "%", this );
	m_filters.clear();
	close();
}


ip::endpoint
ip::tcp::socket::name() const
{
	ip::endpoint name;

	ncheck_error_quiet( m_handle, exit );

	name = m_handle->name();

exit:

	return name;
}


ip::tcp::socket&
ip::tcp::socket::operator=( ip::tcp::socket &&rhs )
{
	mlog( marker::socket_tcp, log::level_t::info, "%", this );

	if ( m_handle )
	{
		m_handle->close();
		m_handle = nullptr;
	}
		
	m_handle			= rhs.m_handle;

	if ( m_handle )
	{
		m_handle->set_owner( this );
	}
	
	m_peek_reply		= std::move( rhs.m_peek_reply );
	m_recv_reply		= std::move( rhs.m_recv_reply );
	m_filters			= std::move( rhs.m_filters );
	m_state				= rhs.m_state;
	m_tag				= rhs.m_tag;
	
	rhs.m_handle		= nullptr;
	rhs.m_state			= stream::state_t::disconnected;

	return *this;
}


void
ip::tcp::socket::push( stream::filter *filter )
{
	if ( m_handle )
	{
		buffer dummy;
		buffer out_buf;
		
		// Let's see if this guy needs to send something
		
		if ( filter->send( dummy, out_buf ) != stream::state_t::error )
		{
			if ( out_buf.size() > 0 )
			{
				send( std::move( out_buf ) )
				.then( [=]() mutable
				{
				},
				[=]( std::error_code err ) mutable
				{
					nunused( err );
				} );
			}
		}
	}
	
	m_filters.emplace_back( filter );

	mlog( marker::socket_filter, log::level_t::info, "% push filter % (%) (%)", this, filter->name(), filter->state(), m_filters.size() );
}


stream::filter*
ip::tcp::socket::find( const std::string &name ) const
{
	stream::filter *filter = nullptr;
	
	for ( auto &it : m_filters )
	{
		if ( it->name() == name )
		{
			filter = it.get();
			break;
		}
	}
	
	return filter;
}


promise< void >
ip::tcp::socket::connect( const ip::endpoint &to )
{
	auto ret = promise< void >();
	
	mlog( marker::socket_tcp_connect, log::level_t::info, "% connect to % (%)", this, to.to_string(), m_filters.size() );
	
	close();
	assert( !m_handle );
	
	m_handle = new socket::handle( this );
	assert( m_handle );
	
	nodeoze::uri tmp;
	tmp.set_host( to.addr().to_string() );
	
	if ( proxy::manager::shared().is_socks() && !proxy::manager::shared().bypass( tmp ) )
	{
		ip::endpoint endpoint( proxy::manager::shared().resource().host(), proxy::manager::shared().resource().port() );
		m_filters.emplace_front( proxy::manager::shared().create_filter( to ) );
		ip::endpoint proxy_to;
		proxy_to.set_addr( proxy::manager::shared().resource().host() );
		proxy_to.set_port( proxy::manager::shared().resource().port() );
		mlog( marker::socket_tcp_connect, log::level_t::info, "proxy connect to %", proxy_to.to_string() );
		m_handle->connect( proxy_to, ret );
	}
	else
	{
		m_handle->connect( to, ret );
	}
	
	return ret;
}


promise< void >
ip::tcp::socket::send( buffer in_buf )
{
	auto ret = promise< void >();
	
	mlog( marker::socket_tcp_send, log::level_t::info, "% send % bytes (%)", this, in_buf.size(), m_filters.size() );
	send( m_filters.rbegin(), std::move( in_buf ), ret );
	
	return ret;
}


void
ip::tcp::socket::send( filters_t::reverse_iterator it, buffer in_buf, promise< void > ret )
{
	mlog( marker::socket_tcp_send, log::level_t::info, "% has % filters", this, m_filters.size() );

	for ( ; ( in_buf.size() > 0 ) && ( it != m_filters.rend() ); it++ )
	{
		mlog( marker::socket_tcp_send, log::level_t::info, "% send % bytes to filter %", this, in_buf.size(), ( *it )->name() );
		buffer out_buf;
		( *it )->send( in_buf, out_buf );
		in_buf = std::move( out_buf );
	}
	
	if ( in_buf.size() )
	{
		really_send( in_buf, ret );
	}
	else
	{
		ret.resolve();
	}
}


void
ip::tcp::socket::really_send( buffer &buf, promise< void > ret )
{
	if ( m_handle != nullptr )
	{
		if ( buf.size() > 0 )
		{
			mlog( marker::socket_tcp_send, log::level_t::info, "send % bytes to handle 0x%", buf.size(), m_handle );
			m_handle->send( std::move( buf ), ret );
		}
		else
		{
			ret.resolve();
		}
	}
	else
	{
		ret.reject( make_error_code( std::errc::not_connected ), reject_context );
	}
}


void
ip::tcp::socket::peek( recv_reply_f reply )
{
	m_peek_reply = reply;
}


void
ip::tcp::socket::recv( recv_reply_f reply )
{
	m_recv_reply = std::move( reply );
}


void
ip::tcp::socket::start_recv()
{
	mlog( marker::socket_tcp_recv, log::level_t::info, "" );
	ncheck_error_quiet( m_handle, exit );
	m_handle->recv();

exit:

	return;
}


void
ip::tcp::socket::recv( buffer &in_buf )
{
	std::vector< buffer >	in_bufs;
	buffer					out_send_buf;
	std::vector< buffer >	out_recv_bufs;
	
	mlog( marker::socket_tcp_recv, log::level_t::info, "state: %, buf size = %", m_state, in_buf.size() );
	
	assert( m_state == stream::state_t::connected );
	assert( in_buf.size() > 0 );
	
	in_bufs.emplace_back( std::move( in_buf ) );
	
	for ( auto it = m_filters.begin(); ( in_bufs.size() > 0 ) && ( it != m_filters.end() ); it++ )
	{
		mlog( marker::socket_tcp_recv, log::level_t::info, "sending % buffers to filter %", in_bufs.size(), ( *it )->name() );
		
		m_state = ( *it )->recv( in_bufs, out_send_buf, out_recv_bufs );
		
		mlog( marker::socket_tcp_recv, log::level_t::info, "filter % state is %", ( *it )->name(), m_state );
		
		if ( m_state == stream::state_t::connected )
		{
			if ( out_send_buf.size() > 0 )
			{
				auto p = promise< void >();
				
				mlog( marker::socket_tcp_recv, log::level_t::info, "filter % produced % bytes to send", ( *it )->name(), out_send_buf.size() );
				
				send( std::reverse_iterator< filters_t::iterator >( it ), std::move( out_send_buf ), p );
			}
	
			in_bufs = std::move( out_recv_bufs );
		}
		else
		{
			break;
		}
	}

	if ( m_recv_reply )
	{
		if ( m_state == stream::state_t::connected )
		{
			for ( auto &it : in_bufs )
			{
				if ( it.size() > 0 )
				{
					if ( m_recv_reply )
					{
						mlog( marker::socket_tcp_recv, log::level_t::info, "invoking handler with % bytes", it.size() );
						m_recv_reply( std::error_code(), it );
					}
				}
			}
		}
		else
		{
			buffer dummy;
	
			mlog( marker::socket_tcp_recv, log::level_t::info, "invoking handler with eof" );
			m_recv_reply( make_error_code( std::errc::no_message_available ), dummy );
		}
	}
	else
	{
		mlog( marker::socket_tcp_recv, log::level_t::warning, "no recv reply" );
	}
}


void
ip::tcp::socket::set_keep_alive( bool val )
{
	assert( m_handle );
	m_handle->set_keep_alive( val );
}


bool
ip::tcp::socket::is_open() const
{
	return ( m_handle ) ? true : false;
}


ip::endpoint
ip::tcp::socket::peer() const
{
	return ( m_handle ) ? m_handle->peer() : ip::endpoint();
}


bool
ip::tcp::socket::is_peer_local() const
{
	return peer().addr().is_loopback();
}


void
ip::tcp::socket::close()
{
	if ( m_handle )
	{
		mlog( marker::socket_tcp_close, log::level_t::info, "name: %, peer: %, handle: %", name().to_string(), peer().to_string(), m_handle );
		
		m_handle->close();
		
		for ( auto &filter : m_filters )
		{
			filter->reset();
		}
		
		m_handle		= nullptr;
		m_state			= stream::state_t::disconnected;
		m_recv_reply	= nullptr;
		
		notification::shared().publish( notification::local, 0, make_oid( this ), close_event );
	}
}


void
ip::tcp::socket::on_connect( std::error_code error, promise< void > ret )
{
	mlog( marker::socket_tcp_connect, log::level_t::info, "name: %, peer: %, handle: %, error: %", name().to_string(), peer().to_string(), m_handle, error );
	
	if ( !error )
	{
		m_state = stream::state_t::connected;
		
		start_recv();
		
		ret.resolve();
	}
	else
	{
		ret.reject( error, reject_context );
	}
}


void
ip::tcp::socket::on_send( std::error_code error, promise< void > ret )
{
	mlog( marker::socket_tcp_send, log::level_t::info, "name: %, peer: %, handle: %, error: %", name().to_string(), peer().to_string(), m_handle, error );
	
	if ( !error )
	{
		ret.resolve();
	}
	else
	{
		ret.reject( error, reject_context );
	}
}


void
ip::tcp::socket::on_recv( std::error_code err, buffer &buf )
{
	mlog( marker::socket_tcp_recv, log::level_t::info, "name: %, peer: %, handle: %, error: % (%), size = %", name().to_string(), peer().to_string(), m_handle, err, err.message(), buf.size() );
	
	assert( ( m_state == stream::state_t::handshaking ) || ( m_state == stream::state_t::connected ) );
	
	if ( !err )
	{
		assert( buf.size() > 0 );

		// Yes this is a little weird. Problem is that the peek reply can change the socket owner in 
		// the handle.  We take care of this by stashing away the handle and using whoever it's pointing
		// to call recv().

		auto handle = m_handle;

		if ( m_peek_reply )
		{
			m_peek_reply( std::error_code(), buf );
		}

		handle->owner()->recv( buf );
	}
	else if ( m_recv_reply )
	{
		buffer dummy;
		m_recv_reply( err, dummy );
		
		/*
		 * do not touch this object after calling reply handler
		 */
	}
}


#if defined( __APPLE__ )
#	pragma mark ip::tcp::acceptor implementation
#endif

std::uint64_t		ip::tcp::acceptor::m_next_tag = 1;

ip::tcp::acceptor::acceptor()
:
	m_handle( new handle( this ) ),
	m_tag( m_next_tag++ )
{
	mlog( marker::socket_tcp_accept, log::level_t::info, "this = %", this );
}


ip::tcp::acceptor::acceptor( acceptor &&rhs )
:
	m_handle( nullptr )
{
	mlog( marker::socket_tcp_accept, log::level_t::info, "this = %", this );
	operator=( std::move( rhs ) );
}


ip::tcp::acceptor::~acceptor()
{
	mlog( marker::socket_tcp_accept, log::level_t::info, "this = %, m_handle = %", this, m_handle );

	if ( m_handle )
	{
		m_handle->close();
		m_handle = nullptr;
	}
}


ip::tcp::acceptor&
ip::tcp::acceptor::operator=( ip::tcp::acceptor &&rhs )
{
	mlog( marker::socket_tcp_accept, log::level_t::info, "this = %, m_handle = %, rhs = %, rhs->handle = %", this, m_handle, &rhs, rhs.m_handle );

	if ( m_handle )
	{
		m_handle->close();
		m_handle = nullptr;
	}

	m_handle			= rhs.m_handle;

	if ( m_handle )
	{
		m_handle->set_owner( this );
	}
	
	m_tag				= rhs.m_tag;
	
	rhs.m_handle		= nullptr;

	return *this;
}

std::error_code
ip::tcp::acceptor::bind( const ip::endpoint &name )
{
	auto err = std::error_code();
	
	mlog( marker::socket_tcp_accept, log::level_t::info, "bind to %", name.to_string() );
	ncheck_error_action_quiet( m_handle, err = make_error_code( std::errc::not_connected ), exit );
	err = m_handle->bind( name );
	
exit:

	return err;
}


void
ip::tcp::acceptor::accept( std::size_t qsize, accept_reply_f reply )
{
	ncheck_error_action_quiet( m_handle, reply( make_error_code( std::errc::owner_dead ), socket() ), exit );
	m_handle->accept( qsize, reply );
	
exit:

	return;
}


ip::endpoint
ip::tcp::acceptor::name() const
{
	ip::endpoint name;
	
	ncheck_error_quiet( m_handle, exit );
	name = m_handle->name();
	
exit:

	return name;
}


void
ip::tcp::acceptor::on_bind( std::error_code error, promise< void > ret )
{
	mlog( marker::socket_tcp_accept, log::level_t::info, "name: %, handle: %, error: %", name().to_string(), m_handle, error );

	if ( !error )
	{
		ret.resolve();
	}
	else
	{
		ret.reject( error, reject_context );
	}
}


void
ip::tcp::acceptor::on_accept( std::error_code error, ip::tcp::socket socket, accept_reply_f reply )
{
	mlog( marker::socket_tcp_accept, log::level_t::info, "name: %, handle: %, error %", name().to_string(), m_handle, error );
	reply( error, std::move( socket ) );
}


#if defined( __APPLE__ )
#	pragma mark ip::udp::socket implementation
#endif

std::uint64_t ip::udp::socket::m_next_tag = 1;

ip::udp::socket::socket()
:
	m_handle( new handle( this ) ),
	m_tag( m_next_tag++ )
{
	assert( m_handle );
}


ip::udp::socket::socket( socket &&rhs )
:
	m_handle( nullptr )
{
	operator=( std::move( rhs ) );
}


ip::udp::socket::~socket()
{
	if ( m_handle )
	{
		m_handle->close();
		m_handle = nullptr;
	}
}


ip::udp::socket&
ip::udp::socket::operator=( socket &&rhs )
{
	if ( m_handle )
	{
		m_handle->close();
		m_handle = nullptr;
	}

	m_handle			= rhs.m_handle;
	m_tag				= rhs.m_tag;

	if ( m_handle )
	{
		m_handle->set_owner( this );
	}

	m_recv_reply		= rhs.m_recv_reply;
	
	rhs.m_handle		= nullptr;
	rhs.m_recv_reply	= nullptr;
	
	return *this;
}


std::error_code
ip::udp::socket::bind( const ip::endpoint &name )
{
	auto err = std::error_code();

	mlog( marker::socket_udp_bind, log::level_t::info, "handle: %, bind to %", m_handle, name.to_string() );
	ncheck_error_action_quiet( m_handle, err = make_error_code( std::errc::not_connected ), exit );
	ncheck_error_action_quiet( m_handle->owner() == this, err = make_error_code( std::errc::not_connected  ), exit );
	
	err = m_handle->bind( name );
	
exit:

	return err;
}


ip::endpoint
ip::udp::socket::name() const
{
	ip::endpoint name;
	
	ncheck_error_quiet( m_handle, exit );
	
	name = m_handle->name();
	
exit:

	return name;
}


std::error_code
ip::udp::socket::set_membership( const ip::endpoint &endpoint, const ip::address &iface, membership_t membership )
{
	auto err = std::error_code();

	ncheck_error_action_quiet( m_handle, err = make_error_code( std::errc::not_connected ), exit );
	
	err = m_handle->set_membership( endpoint, iface, membership );
	
exit:

	return err;
}


std::error_code
ip::udp::socket::set_broadcast( bool val )
{
	auto err = std::error_code();

	ncheck_error_action_quiet( m_handle, err = make_error_code( std::errc::not_connected ), exit );
	
	err = m_handle->set_broadcast( val );
	
exit:

	return err;
}


promise< void >
ip::udp::socket::send( buffer buf, const ip::endpoint &to )
{
	auto ret = promise< void >();

	mlog( marker::socket_udp_send, log::level_t::info, "handle: %, bytes: %, to: %", m_handle, buf.size(), to.to_string() );
	
	ncheck_error_action_quiet( m_handle, ret.reject( make_error_code( std::errc::not_connected ), reject_context ), exit );
	
	m_handle->send( std::move( buf ), to, ret );
	
exit:

	return ret;
}


void
ip::udp::socket::recv( recv_reply_f reply )
{
	m_recv_reply = reply;
}


void
ip::udp::socket::on_bind( std::error_code error, promise< void > ret )
{
	mlog( marker::socket_udp_bind, log::level_t::info, "handle: %, error: %", m_handle, error );

	if ( !error )
	{
		ret.resolve();
	}
	else
	{
		ret.reject( error, reject_context );
	}
}


void
ip::udp::socket::on_send( std::error_code error, promise< void > ret )
{
	mlog( marker::socket_udp_send, log::level_t::info, "error: %", error );

	if ( !error )
	{
		ret.resolve();
	}
	else
	{
		ret.reject( error, reject_context );
	}
}

	
void
ip::udp::socket::on_recv( std::error_code error, const ip::endpoint &from, buffer buf )
{
	mlog( marker::socket_udp_recv, log::level_t::info, "handle: %, error: %, from: %, size: %", m_handle, error, from.to_string(), buf.size() );
	
	if ( m_recv_reply )
	{
		m_recv_reply( error, from, std::move( buf ) );
	}
	else
	{
		mlog( marker::socket_udp_recv, log::level_t::warning, "received message from % but have no recv handler for it.", from.to_string() );
	}
}
