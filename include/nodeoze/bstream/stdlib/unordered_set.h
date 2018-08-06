#ifndef NODEOZE_BSTREAM_STDLIB_UNORDERED_SET_H
#define NODEOZE_BSTREAM_STDLIB_UNORDERED_SET_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <unordered_set>

namespace nodeoze
{
namespace bstream
{

template< class T, class Hash, class Equal, class Alloc >
struct value_deserializer< std::unordered_set< T, Hash, Equal, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::unordered_set< T, Hash, Equal, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unordered_set< T, Hash, Equal, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::unordered_set< T, Hash, Equal, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};	

template< class T, class Hash, class Equal, class Alloc >
struct value_deserializer< std::unordered_multiset< T, Hash, Equal, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::unordered_multiset< T, Hash, Equal, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unordered_multiset< T, Hash, Equal, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::unordered_multiset< T, Hash, Equal, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};	

template< class T, class Hash, class Equal, class Alloc >
struct serializer< std::unordered_set< T, Hash, Equal, Alloc > >
{
	static obstream& put( obstream& os, const std::unordered_set< T, Hash, Equal, Alloc >& s )
	{
		os.write_array_header( s.size() );
		for ( auto it = s.begin(); it != s.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

template< class T, class Hash, class Equal, class Alloc >
struct serializer< std::unordered_multiset< T, Hash, Equal, Alloc > >
{
	static obstream& put( obstream& os, const std::unordered_multiset< T, Hash, Equal, Alloc >& s )
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

#endif // NODEOZE_BSTREAM_STDLIB_UNORDERED_SET_H