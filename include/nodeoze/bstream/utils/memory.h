#ifndef NODEOZE_BSTREAM_UTILS_MEMORY_H
#define NODEOZE_BSTREAM_UTILS_MEMORY_H

#include <memory>

namespace nodeoze
{
namespace bstream
{
namespace utils 
{

template< class Derived, class Base >
typename std::enable_if_t< std::is_move_constructible< Derived >::value, std::unique_ptr< Derived > >
static_unique_ptr_cast( std::unique_ptr< Base >&& p )
{
    std::unique_ptr< Derived > derp = 
        std::make_unique< Derived >( std::move( reinterpret_cast< Derived& >( *p.get() ) ) );
    p = nullptr;
    return derp;     
}


// template< class Derived, class Base >
// std::unique_ptr< Derived >
// static_unique_ptr_cast( std::unique_ptr< Base >&& p )
// {
//     return std::unique_ptr< Derived >{ reinterpret_cast< Derived* >( p.release() ) };
// }

/*
template< typename Derived, typename Base, typename Deleter >
inline std::unique_ptr< Derived, Deleter > 
static_unique_ptr_cast( std::unique_ptr< Base, Deleter >&& p )
{
    auto d = static_cast< Derived * >( p.release() );
    return std::unique_ptr< Derived, Deleter >( d, std::move( p.get_deleter() ) );
}

template< typename Derived, typename Base, typename Deleter >
inline std::unique_ptr< Derived, Deleter > 
dynamic_unique_ptr_cast( std::unique_ptr< Base, Deleter >&& p )
{
    if( Derived *result = dynamic_cast< Derived * >( p.get() ) ) 
	{
        p.release();
        return std::unique_ptr< Derived, Deleter >( result, std::move( p.get_deleter() ) );
    }
    return std::unique_ptr< Derived, Deleter >( nullptr, p.get_deleter() );
}
*/

} // namespace utils
} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_UTILS_MEMORY_H