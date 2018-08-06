#ifndef NODEOZE_BSTREAM_STDLIB_LIST_H
#define NODEOZE_BSTREAM_STDLIB_LIST_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <list>

namespace nodeoze
{
namespace bstream
{

template< class T, class Alloc >
struct value_deserializer< std::list< T, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::list< T, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::list< T, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::list< T, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace_back( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};

template< class T, class Alloc >
struct serializer< std::list< T, Alloc > >
{
	static obstream& put( obstream& os, const std::list< T, Alloc >& lst )
	{
		os.write_array_header( lst.size() );
		for ( auto it = lst.begin(); it != lst.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_LIST_H