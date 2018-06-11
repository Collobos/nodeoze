#ifndef _nodeoze_net_h
#define _nodeoze_net_h

#include <nodeoze/event.h>
#include <nodeoze/stream.h>
#include <nodeoze/endpoint.h>

namespace nodeoze {

namespace net {

#if defined( WIN32 )

#include <winsock2.h>
using native_type = SOCKET;

#else

using native_type = int;

#endif

namespace tcp {

/*
 * events:
 *
 * "lookup"
 * "connect"
 * "ready"
 * "data"
 * "drain"
 * "end"
 * "close"
 * "error"
 * "timeout"
 */

class socket : public stream::duplex
{
public:

	class handle;

	socket();

	socket( native_type fd );

	socket( const socket &rhs ) = delete;

	socket( socket &&rhs );

	socket( handle *h );

	virtual ~socket();

	socket&
	operator=( const socket &rhs ) = delete;

	socket&
	operator=( socket &&rhs );

	promise< void >
	connect( ip::endpoint to );

protected:

	friend class server;
	friend handle;


	virtual promise< void >
	really_write( buffer b );

	virtual void
	really_read();

	void
	on_connect( std::error_code error, promise< void > ret );

	void
	on_send( std::error_code error, promise< void > ret );

	void
	on_recv( std::error_code error, buffer &buf );

	handle *m_handle;
};

/*
 * events:
 *
 * "listening"
 * "connection"
 * "error"
 * "close"
 */

class server : public event::emitter<>
{
public:

	server();

	server( const server &rhs ) = delete;

	server( server &&rhs );

	virtual ~server();

	server&
	operator=( const server& rhs ) = delete;

	server&
	operator=( server &&rhs );

	promise< void >
	listen( ip::endpoint endpoint, std::size_t qsize );

	ip::endpoint
	name() const;

	virtual void
	close();

private:

	class handle;

	void
	on_bind( std::error_code err, promise< void > ret );

	void
	on_accept( std::error_code err, socket sock );

	handle *m_handle;
};


}

namespace udp {

enum class multicast_membership_type
{
	join,
	leave
};

class socket : public stream::duplex
{
public:

	socket();

	socket( native_type fd );

	socket( const socket &rhs ) = delete;

	socket( socket &&rhs );

	socket&
	operator=( const socket &rhs ) = delete;

	socket&
	operator=( socket &&rhs );

protected:

	virtual promise< void >
	really_write( buffer b );

	virtual void
	really_read();

	void
	on_bind( std::error_code error, promise< void > ret );
	
	void
	on_send( std::error_code error, promise< void > ret );
	
	void
	on_recv( std::error_code error, const nodeoze::ip::endpoint &from, nodeoze::buffer buf );

	class handle;
	friend handle;

	handle *m_handle;
};

}

}

}

#endif
