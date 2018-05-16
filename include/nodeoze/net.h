#ifndef _nodeoze_net_h
#define _nodeoze_net_h

#include <nodeoze/nevent.h>

namespace nodeoze {

namespace net {

namespace tcp {

/*
 * events:
 *
 * "listening"
 * "connection"
 * "error"
 * "close"
 */

class server : public event::emitter
{
public:

	using ptr = std::shared_ptr< server >;

	static ptr
	create();

	virtual void
	listen( std::uint16_t port, std::uint32_t qsize ) = 0;

	virtual ip::endpoint
	address() const = 0;

	virtual void
	close() = 0;
};

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

	using ptr = std::shared_ptr< socket >;

	static ptr
	create();

	static ptr
	create( native_type fd );
};

}

namespace udp {

class socket : public stream::duplex
{
public:

	using ptr = std::shared_ptr< socket >;

	static ptr
	create();

	static ptr
	create( native_type fd );
};

}

}

}

#endif
