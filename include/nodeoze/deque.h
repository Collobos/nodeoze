#pragma once

#include <boost/container/deque.hpp>

namespace nodeoze
{
	template< typename T >
	using deque = boost::container::deque< T >;
}
