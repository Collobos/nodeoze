#ifndef NODEOZE_BSTREAM_STDLIB_FORWARD_LIST_H
#define NODEOZE_BSTREAM_STDLIB_FORWARD_LIST_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <forward_list>

namespace nodeoze
{
namespace bstream
{

template< class T, class Alloc >
struct value_deserializer< std::forward_list< T, Alloc >,
        typename std::enable_if_t< is_ibstream_readable< T >::value > >
{
    std::forward_list< T, Alloc > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::forward_list< T, Alloc > 
    get( ibstream& is )
    {
        auto length = is.read_array_header();
        std::forward_list< T, Alloc > result;
        auto it = result.before_begin();
        for ( auto i = 0u; i < length; ++i )
        {
            it = result.emplace_after( it, ibstream_initializer< T >::get( is ) );
        }
        return result;
    }
};	

template< class T, class Alloc >
struct serializer< std::forward_list< T, Alloc > >
{
	static obstream& put( obstream& os, const std::forward_list< T, Alloc >& flist )
	{
		auto length = std::distance( flist.begin(), flist.end() );
		os.write_array_header( length );
		for ( auto it = flist.begin(); it != flist.end(); ++it )
		{
			os << *it;
		}
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_FORWARD_LIST_H