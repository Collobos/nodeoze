#ifndef _nodeoze_net_h
#define _nodeoze_net_h

#include <nodeoze/event.h>
#include <nodeoze/stream.h>
#include <nodeoze/endpoint.h>

namespace nodeoze {

namespace net {

namespace tcp {

/** a tcp socket
 
 * \code
 * events:
 *
 * on( "lookup", []() {} );
 * "connect"
 * "ready"
 * on( "data", []( buffer b ) {} );
 * "drain"
 * "end"
 * "close"
 * "error"
 * "timeout"
 * 
 * \endcode
 */

class socket : public stream::duplex
{
public:

	class options
	{
	public:

		options( ip::endpoint endpoint )
		:
			m_endpoint( endpoint )
		{
		}

		inline const ip::endpoint&
		endpoint() const
		{
			return m_endpoint;
		}

		bool
		keep_alive() const
		{
			return m_keep_alive;
		}

		options&
		keep_alive( bool val )
		{
			m_keep_alive = val;
			return *this;
		}

		bool
		nagle() const
		{
			return m_nagle;
		}

		options&
		nagle( bool val )
		{
			m_nagle = val;
			return *this;
		}

	private:

		ip::endpoint	m_endpoint;
		bool			m_keep_alive;
		bool			m_nagle;
	};

	using ptr = std::shared_ptr< socket >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );

	virtual ~socket() = 0;
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

	class options
	{
	public:

		options( ip::endpoint endpoint )
		:
			m_endpoint( std::move( endpoint ) )
		{
		}

		const ip::endpoint&
		endpoint() const
		{
			return m_endpoint;
		}

		std::int32_t
		qsize() const
		{
			return m_qsize;
		}

		options&
		qsize( std::int32_t val )
		{
			m_qsize = val;
			return *this;
		}

	private:

		ip::endpoint	m_endpoint;
		std::int32_t	m_qsize = 5;
	};

	using ptr = std::shared_ptr< server >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );

	virtual ~server() = 0;

	virtual const ip::endpoint&
	name() const = 0;
};

}

namespace udp {

class socket : public event::emitter<>
{
public:

	class options
	{
	public:

		options( ip::endpoint endpoint )
		:
			m_endpoint( endpoint )
		{
		}

		inline const ip::endpoint&
		endpoint() const
		{
			return m_endpoint;
		}

	private:

		ip::endpoint m_endpoint;
	};

	enum class multicast_membership_type
	{
		join,
		leave
	};

	using ptr = std::shared_ptr< socket >;

	static ptr
	create( options options );

	static ptr
	create( options options, std::error_code &err );

	virtual ~socket() = 0;
};

}

}

}

#endif
