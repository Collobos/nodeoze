#ifndef _nodeoze_net_h
#define _nodeoze_net_h

#include <nodeoze/nevent.h>

namespace nodeoze {

namespace net {

namespace tcp {

class server : public event::emitter
{
public:

	static ptr
	create();

	server();

	void
	listen( std::uint16_t port );

	void
	accept( std::uint32_t qsize );
};

class socket : public stream::duplex
{
};

}

namespace udp {

class socket : public stream::duplex
{
};

}

}

}

#endif
