#ifndef NODEOZE_BSTREAM_STDLIB_TUPLE_H
#define NODEOZE_BSTREAM_STDLIB_TUPLE_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <tuple>

namespace nodeoze
{
namespace bstream
{

template< class... Args >
struct value_deserializer< std::tuple< Args... >,
        std::enable_if_t< utils::conjunction< is_ibstream_readable< Args >::value... >::value > >
{
    using tuple_type = std::tuple< Args... >;
    
    tuple_type 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static tuple_type
    get( ibstream& is )
    {
        is.check_array_header( std::tuple_size< tuple_type >::value );
        tuple_type tup;
        get_members< 0, Args... >( is, tup );
        return tup;
    }
    
    template< unsigned int N, class First, class... Rest >
    static 
    typename std::enable_if< ( sizeof...( Rest ) > 0 ) >::type
    get_members( ibstream& is, tuple_type& tup )
    {
        is >> std::get< N >( tup );
        get_members< N+1, Rest... >( is, tup );
    }
    
    template < unsigned int N, class T >
    static void
    get_members( ibstream& is, tuple_type& tup )
    {
        is >> std::get< N >( tup );
    }
};

template< class... Args >
struct serializer< std::tuple< Args... > >
{
	using tuple_type = std::tuple< Args... >;
	static obstream& put( obstream& os, tuple_type const& tup )
	{
		os.write_array_header( std::tuple_size< tuple_type >::value );
		put_members< 0, Args... >( os, tup );
		return os;
	}
	
	template< unsigned int N, class First, class... Rest >
	static typename std::enable_if< ( sizeof...( Rest ) > 0 ) >::type
	put_members( obstream& os, tuple_type const& tup )
	{
		os << std::get< N >( tup );
		put_members< N+1, Rest... >( os, tup );
	}
	
	template< unsigned int N, class T >
	static void
	put_members( obstream& os, tuple_type const& tup )
	{
		os << std::get< N >( tup );
	}		
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_TUPLE_H