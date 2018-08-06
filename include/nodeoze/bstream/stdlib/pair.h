#ifndef NODEOZE_BSTREAM_STDLIB_PAIR_H
#define NODEOZE_BSTREAM_STDLIB_PAIR_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <utility>

namespace nodeoze
{
namespace bstream
{

template< class T1, class T2 >
struct value_deserializer< std::pair< T1, T2 >,
        typename std::enable_if_t<
            std::is_move_constructible< T1 >::value &&
            std::is_move_constructible< T2 >::value > >
{
    std::pair< T1, T2 > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::pair< T1, T2 >
    get( ibstream& is )
    {
        is.check_array_header( 2 );
        T1 t1{ ibstream_initializer< T1 >::get( is ) };
        T2 t2{ ibstream_initializer< T2 >::get( is ) };
        return std::make_pair( std::move( t1 ), std::move( t2 ) );
    }
};

template< class T1, class T2 >
struct value_deserializer< std::pair< T1, T2 >,
        typename std::enable_if_t<
            ( !std::is_move_constructible< T1 >::value && std::is_copy_constructible< T1 >::value ) &&
            std::is_move_constructible< T2 >::value > >
{
    std::pair< T1, T2 > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::pair< T1, T2 >
    get( ibstream& is )
    {
        is.check_array_header( 2 );
        T1 t1{ ibstream_initializer< T1 >::get( is ) };
        T2 t2{ ibstream_initializer< T2 >::get( is ) };
        return std::make_pair( t1, std::move( t2 ) );
    }
};

template< class T1, class T2 >
struct value_deserializer< std::pair< T1, T2 >,
        typename std::enable_if_t<
            std::is_move_constructible< T1 >::value &&
            ( !std::is_move_constructible< T2 >::value && std::is_copy_constructible< T2 >::value ) > >
{
    std::pair< T1, T2 > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::pair< T1, T2 >
    get( ibstream& is )
    {
        is.check_array_header( 2 );
        T1 t1{ ibstream_initializer< T1 >::get( is ) };
        T2 t2{ ibstream_initializer< T2 >::get( is ) };
        return std::make_pair( std::move( t1 ), t2 );
    }
};

template< class T1, class T2 >
struct value_deserializer< std::pair< T1, T2 >,
        typename std::enable_if_t<
            ( !std::is_move_constructible< T1 >::value && std::is_copy_constructible< T1 >::value ) &&
            ( !std::is_move_constructible< T2 >::value && std::is_copy_constructible< T2 >::value ) > >
{
    std::pair< T1, T2 > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::pair< T1, T2 >
    get( ibstream& is )
    {
        is.check_array_header( 2 );
        T1 t1{ ibstream_initializer< T1 >::get( is ) };
        T2 t2{ ibstream_initializer< T2 >::get( is ) };
        return std::make_pair( t1, t2 );
    }
};

template< class T1, class T2 >
struct serializer< std::pair< T1, T2 > >
{
	static obstream& put( obstream& os, std::pair< T1, T2 > const& p )
	{
		os.write_array_header( 2 );
		os << p.first;
		os << p.second;
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_PAIR_H