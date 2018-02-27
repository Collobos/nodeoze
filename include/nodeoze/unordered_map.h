#pragma once

#include <boost/unordered_map.hpp>

namespace nodeoze
{
	template< typename Key, typename Mapped, typename Hash = boost::hash<Key>, typename Pred = std::equal_to<Key>, typename Alloc = std::allocator<std::pair<Key const, Mapped> > >
	using unordered_map = boost::unordered_map< Key, Mapped, Hash, Pred, Alloc >;
}
