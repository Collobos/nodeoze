#ifndef NODEOZE_BSTREAM_STDLIB_VECTOR_H
#define NODEOZE_BSTREAM_STDLIB_VECTOR_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <vector>

namespace nodeoze
{
namespace bstream
{

template< class T, class Alloc >
struct value_deserializer< std::vector< T, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::vector< T, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::vector< T, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::vector< T, Alloc > result;
        result.reserve( length );
        for ( auto i = 0u; i < length; ++i )
        {
            result.emplace_back( ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};	

template< class T, class Alloc >
struct serializer< std::vector< T, Alloc > >
{
	static obstream& put( obstream& os, const std::vector< T, Alloc >& vec )
	{
		os.write_array_header( vec.size() );
		for ( auto it = vec.begin(); it != vec.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_VECTOR_H