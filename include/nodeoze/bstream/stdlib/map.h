#ifndef NODEOZE_BSTREAM_STDLIB_MAP_H
#define NODEOZE_BSTREAM_STDLIB_MAP_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <map>

namespace nodeoze
{
namespace bstream
{

template< class K, class V, class Compare, class Alloc >
struct value_deserializer< std::map< K, V, Compare, Alloc >,
        typename std::enable_if_t<
            is_ibstream_readable< K >::value &&
            is_ibstream_readable< V >::value > >
{
    std::map< K, V, Compare, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::map< K, V, Compare, Alloc >
    get( ibstream& is )
    {
        using pair_type = std::pair< K, V >;
        using map_type = std::map< K, V, Compare, Alloc >;
        auto length = is.read_array_header();
        map_type result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.insert( is.read_as< pair_type >() );
        }
        return result;
    }
};

template< class K, class V, class Compare, class Alloc >
struct value_deserializer< std::multimap< K, V, Compare, Alloc >,
        typename std::enable_if_t<
            is_ibstream_readable< K >::value &&
            is_ibstream_readable< V >::value > >
{
    std::multimap< K, V, Compare, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::multimap< K, V, Compare, Alloc >
    get( ibstream& is )
    {
        using pair_type = std::pair< K, V >;
        using map_type = std::multimap< K, V, Compare, Alloc >;
        auto length = is.read_array_header();
        map_type result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.insert( is.read_as< pair_type >() );
        }
        return result;
    }
};

template< class K, class V, class Compare, class Alloc >
struct serializer< std::map< K, V, Compare, Alloc > >
{
	static obstream& put( obstream& os, std::map< K, V, Compare, Alloc > const& mp )
	{
		os.write_array_header( mp.size() );
		for ( auto it = mp.begin(); it != mp.end(); ++it )
		{
			os.write_array_header( 2 );
			os << it->first;
			os << it->second;
		}
		return os;
	}
};
	
template< class K, class V, class Compare, class Alloc >
struct serializer< std::multimap< K, V, Compare, Alloc > >
{
	static obstream& put( obstream& os, std::multimap< K, V, Compare, Alloc > const& mp )
	{
		os.write_array_header( mp.size() );
		for ( auto it = mp.begin(); it != mp.end(); ++it )
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

#endif // NODEOZE_BSTREAM_STDLIB_MAP_H