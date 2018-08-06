#ifndef NODEOZE_BSTREAM_STDLIB_DEQUE_H
#define NODEOZE_BSTREAM_STDLIB_DEQUE_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <deque>

namespace nodeoze
{
namespace bstream
{
template< class T, class Alloc >
struct value_deserializer< std::deque< T, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::deque< T, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::deque< T, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::deque< T, Alloc > result;
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace_back( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};

template< class T, class Alloc >
struct serializer< std::deque< T, Alloc > >
{
	static obstream& put( obstream& os, const std::deque< T, Alloc >& deq )
	{
		os.write_array_header( deq.size() );
		for ( auto it = deq.begin(); it != deq.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_DEQUE_H