#ifndef NODEOZE_BSTREAM_STDLIB_UNORDERED_MAP_H
#define NODEOZE_BSTREAM_STDLIB_UNORDERED_MAP_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <unordered_map>

namespace nodeoze
{
namespace bstream
{

template< class K, class V, class Hash, class Equal, class Alloc >
struct value_deserializer< std::unordered_map< K, V, Hash, Equal, Alloc >,
        typename std::enable_if_t<
            is_ibstream_readable< K >::value &&
            is_ibstream_readable< V >::value > >
{
    std::unordered_map< K, V, Hash, Equal, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unordered_map< K, V, Hash, Equal, Alloc >
    get( ibstream& is )
    {
        using pair_type = std::pair< K,V >;
        using map_type = std::unordered_map< K, V, Hash, Equal, Alloc >;
        
        auto length = is.read_array_header();
        map_type result;
        result.reserve( length );
        for ( auto i = 0u; i < length; ++i )
        {
            result.insert( is.read_as< pair_type >() );
        }
        return result;
    }
};

template< class K, class V, class Hash, class Equal, class Alloc >
struct value_deserializer< std::unordered_multimap< K, V, Hash, Equal, Alloc >,
        typename std::enable_if_t<
            is_ibstream_readable< K >::value &&
            is_ibstream_readable< V >::value > >
{
    std::unordered_multimap< K, V, Hash, Equal, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unordered_multimap< K, V, Hash, Equal, Alloc >
    get( ibstream& is )
    {
        using pair_type = std::pair< K,V >;
        using map_type = std::unordered_multimap< K, V, Hash, Equal, Alloc >;
        
        auto length = is.read_array_header();
        map_type result;
        result.reserve( length );
        for ( auto i = 0u; i < length; ++i )
        {
            result.insert( is.read_as< pair_type >() );
        }
        return result;
    }
};

template< class K, class V, class Hash, class Equal, class Alloc >
struct serializer< std::unordered_map< K, V, Hash, Equal, Alloc > >
{
	static obstream& put( obstream& os, std::unordered_map< K, V, Hash, Equal, Alloc > const& map )
	{
		os.write_array_header( map.size() );
		for ( auto it = map.begin(); it != map.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};
	
template< class K, class V, class Hash, class Equal, class Alloc >
struct serializer< std::unordered_multimap< K, V, Hash, Equal, Alloc > >
{
	static obstream& put( obstream& os, std::unordered_multimap< K, V, Hash, Equal, Alloc > const& map )
	{
		os.write_array_header( map.size() );
		for ( auto it = map.begin(); it != map.end(); ++it )
		{
			os.write_array_header( 2 );
			os << it->first;
			os << it->second;
		}
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_UNORDERED_MAP_H