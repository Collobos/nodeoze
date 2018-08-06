#ifndef NODEOZE_BSTREAM_STDLIB_SET_H
#define NODEOZE_BSTREAM_STDLIB_SET_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <set>

namespace nodeoze
{
namespace bstream
{

template< class T, class Compare, class Alloc >
struct value_deserializer< std::set< T, Compare, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::set< T, Compare, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::set< T, Compare, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::set< T, Compare, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};	

template< class T, class Compare, class Alloc >
struct value_deserializer< std::multiset< T, Compare, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::multiset< T, Compare, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::multiset< T, Compare, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::multiset< T, Compare, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};

template< class T, class Compare, class Alloc >
struct serializer< std::set< T, Compare, Alloc > >
{
	static obstream& put( obstream& os, const std::set< T, Compare, Alloc >& s )
	{
		os.write_array_header( s.size() );
		for ( auto it = s.begin(); it != s.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

template< class T, class Compare, class Alloc >
struct serializer< std::multiset< T, Compare, Alloc > >
{
	static obstream& put( obstream& os, const std::multiset< T, Compare, Alloc >& s )
	{
		os.write_array_header( s.size() );
		for ( auto it = s.begin(); it != s.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_SET_H