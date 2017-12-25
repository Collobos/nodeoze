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
 
#ifndef _nodeoze_socket_h
#define _nodeoze_socket_h

#include <nodeoze/nendpoint.h>
#include <nodeoze/nbuffer.h>
#include <nodeoze/nstream.h>
#include <nodeoze/nlog.h>
#include <functional>
#include <queue>
#include <errno.h>

namespace nodeoze {



#if defined( WIN32 )
	typedef SOCKET	native_socket_type;
#else
	typedef int		native_socket_type;
#endif

namespace ip {
	
namespace tcp {

class ssocket
{
public:

#if defined( WIN32 )
	typedef SOCKET	native_t;
#else
	typedef int		native_t;
#endif

	ssocket( const ip::endpoint &to, std::chrono::milliseconds timeout );
	
	ssocket( const ssocket &rhs ) = delete;
	
	ssocket( ssocket &&rhs );
	
	~ssocket();
	
	ssocket&
	operator=( const ssocket &rhs ) = delete;
	
	ssocket&
	operator=( ssocket &&rhs );
	
	std::error_code
	connect();
	
	std::error_code
	send( buffer buf, std::chrono::milliseconds timeout );
	
	std::error_code
	recv( buffer &buf, std::chrono::milliseconds timeout );
	
	void
	close();
	
	native_t
	release();
	
private:

	ip::endpoint				m_to;
	std::chrono::milliseconds	m_timeout;
	native_t					m_fd;
};

class socket
{
public:

#if defined( WIN32 )
	typedef SOCKET															native_t;
#else
	typedef int																native_t;
#endif

	static const std::string												close_event;

	typedef std::function< void ( std::error_code error, buffer &buf ) >	recv_reply_f;
	
	class																	handle;

	socket();
	
	socket( handle *h );
	
	socket( ssocket &sock );
	
	socket( const socket &rhs ) = delete;
	
	socket( socket &&rhs );
	
	~socket();

	socket&
	operator=( const socket &rhs ) = delete;
	
	socket&
	operator=( socket &&rhs );
	
	void
	push( stream::filter *filter );
	
	stream::filter*
	find( const std::string &name ) const;
	
	void
	remove( stream::filter *filter );

	promise< void >
	connect( const ip::endpoint &to );
	
	promise< void >
	send( buffer buf );
	
	void
	peek( recv_reply_f reply );
	
	void
	recv( recv_reply_f reply );
	
	void
	set_keep_alive( bool val );
	
	bool
	is_open() const;

	ip::endpoint
	name() const;
	
	ip::endpoint
	peer() const;
	
	bool
	is_peer_local() const;
	
	void
	close();
	
	inline std::uint64_t
	tag() const
	{
		return m_tag;
	}

private:

	typedef std::deque< std::unique_ptr< stream::filter > >	filters_t;
	friend													handle;

	void
	send( filters_t::reverse_iterator it, buffer buf, promise< void > ret );
	
	void
	really_send( buffer &buf, promise< void > ret );
	
	void
	start_recv();
	
	void
	recv( buffer &buf );

	void
	on_connect( std::error_code error, promise< void > ret );

	void
	on_send( std::error_code error, promise< void > ret );

	void
	on_recv( std::error_code error, buffer &buf );

	static std::uint64_t	m_next_tag;
	filters_t				m_filters;
	recv_reply_f			m_peek_reply;
	recv_reply_f			m_recv_reply;
	buffer					m_recv_buf;
	stream::state_t			m_state;
	handle					*m_handle;
	std::uint64_t			m_tag;
};


class acceptor
{
public:

	typedef std::function< void ( std::error_code error, socket sock ) > accept_reply_f;

	acceptor();
	
	acceptor( const acceptor &rhs ) = delete;
	
	acceptor( acceptor &&rhs );
	
	~acceptor();

	acceptor&
	operator=( const acceptor& rhs ) = delete;
	
	acceptor&
	operator=( acceptor &&rhs );
	
	inline bool
	operator==( std::uint64_t tag )
	{
		return ( m_tag == tag );
	}
	
	std::error_code
	bind( const nodeoze::ip::endpoint &name );

	void
	accept( std::size_t qsize, accept_reply_f reply );
	
	ip::endpoint
	name() const;
	
	inline std::uint64_t
	tag() const
	{
		return m_tag;
	}
	
private:

	class	handle;
	friend	handle;

	void
	on_bind( std::error_code error, promise< void > ret );

	void
	on_accept( std::error_code error, ip::tcp::socket socket, accept_reply_f reply );

	static std::uint64_t	m_next_tag;
	handle					*m_handle;
	std::uint64_t			m_tag;
};

}

namespace udp {

class socket
{
public:

	enum class membership_t
	{
		join,
		leave
	};

	typedef std::function< void ( std::error_code error, const ip::endpoint &from, buffer buf ) >	recv_reply_f;

	socket();
	
	socket( const socket &rhs ) = delete;
	
	socket( socket &&rhs );
	
	~socket();
	
	socket&
	operator=( const socket &rhs ) = delete;
	
	socket&
	operator=( socket &&rhs );
	
	std::error_code
	bind( const ip::endpoint &name );
	
	ip::endpoint
	name() const;
	
	std::error_code
	set_membership( const ip::endpoint &endpoint, const ip::address &iface, membership_t membership );
	
	std::error_code
	set_broadcast( bool val );
	
	promise< void >
	send( buffer buf, const ip::endpoint &to );
	
	void
	recv( recv_reply_f reply );
	
	inline std::uint64_t
	tag() const
	{
		return m_tag;
	}

protected:

	class	handle;
	friend	handle;

	void
	on_bind( std::error_code error, promise< void > ret );
	
	void
	on_send( std::error_code error, promise< void > ret );
	
	void
	on_recv( std::error_code error, const nodeoze::ip::endpoint &from, nodeoze::buffer buf );

	static std::uint64_t	m_next_tag;
	recv_reply_f			m_recv_reply;
	handle					*m_handle;
	std::uint64_t			m_tag;
};

}

}

inline std::ostream&
operator<<( std::ostream &os, ip::udp::socket::membership_t val )
{
	switch( val )
	{
		case ip::udp::socket::membership_t::join:
		{
			os << "join";
		}
		break;

		case ip::udp::socket::membership_t::leave:
		{
			os << "leave";
		}
		break;

		default:
		{
			os << "unknown";
		}
		break;
	}

	return os;
}

}

#endif
