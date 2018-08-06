#ifndef NODEOZE_BSTREAM_NUMERIC_DESERIALIZERS_H
#define NODEOZE_BSTREAM_NUMERIC_DESERIALIZERS_H

#include <nodeoze/bstream/ibstream_traits.h>

#include <cstdint>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/typecode.h>
#include <nodeoze/bstream/numstream.h>

namespace nodeoze
{
namespace bstream
{

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 1 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else if ( tcode >= typecode::negative_fixint_min )
        {
            return static_cast< T >( static_cast< std::int8_t >( tcode ) );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::int_8:
                    return static_cast< T >( static_cast< std::int8_t >( is.get() ) );
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }                
    }
};

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        !std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 1 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::uint_8:
                    return static_cast< T >( is.get() );
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }                
    }
};

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 2 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else if ( tcode >= typecode::negative_fixint_min )
        {
            return static_cast< T >( static_cast< std::int8_t >( tcode ) );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::int_8:
                    return static_cast< T >( is.get_num< int8_t >() );
                case typecode::uint_8:
                    return static_cast< T >( is.get_num< uint8_t >() );
                case typecode::int_16:
                    return static_cast< T >( is.get_num< int16_t >() );
                case typecode::uint_16:
                {
                    std::uint16_t n = is.get_num< uint16_t >();
                    if ( n <= std::numeric_limits< T >::max() )
                    {
                        return static_cast< T >( n );
                    }
                    else
                    {
                        throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                    }
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        !std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 2 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::uint_8:
                    return static_cast< T >( is.get_num< uint8_t >() );
                case typecode::uint_16:
                    return static_cast< T >( is.get_num< uint16_t >() );
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 4 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else if ( tcode >= typecode::negative_fixint_min )
        {
            return static_cast< T >( static_cast< std::int8_t >( tcode ) );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::int_8:
                    return static_cast< T >( is.get_num< int8_t >() );
                case typecode::uint_8:
                    return static_cast< T >( is.get_num< uint8_t >() );
                case typecode::int_16:
                    return static_cast< T >( is.get_num< int16_t >() );
                case typecode::uint_16:
                    return static_cast< T >( is.get_num< uint16_t >() );
                case typecode::int_32:
                    return static_cast< T >( is.get_num< int32_t >() );
                case typecode::uint_32:
                {
                    std::uint32_t n = is.get_num< uint32_t >();
                    if ( n <= std::numeric_limits< T >::max() )
                    {
                        return static_cast< T >( n );
                    }
                    else
                    {
                        throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                    }
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        !std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 4 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::uint_8:
                    return static_cast< T >( is.get_num< uint8_t >() );
                case typecode::uint_16:
                    return static_cast< T >( is.get_num< uint16_t >() );
                case typecode::uint_32:
                    return static_cast< T >( is.get_num< uint32_t >() );
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 8 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else if ( tcode >= typecode::negative_fixint_min )
        {
            return static_cast< T >( static_cast< std::int8_t >( tcode ) );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::int_8:
                    return static_cast< T >( is.get_num< int8_t >() );
                case typecode::uint_8:
                    return static_cast< T >( is.get_num< uint8_t >() );
                case typecode::int_16:
                    return static_cast< T >( is.get_num< int16_t >() );
                case typecode::uint_16:
                    return static_cast< T >( is.get_num< uint16_t >() );
                case typecode::int_32:
                    return static_cast< T >( is.get_num< int32_t >() );
                case typecode::uint_32:
                    return static_cast< T >( is.get_num< uint32_t >() );
                case typecode::int_64:
                    return static_cast< T >( is.get_num< int64_t >() );
                case typecode::uint_64:
                {
                    std::uint64_t n = is.get_num< uint64_t >();
                    if ( n <= std::numeric_limits< T >::max() )
                    {
                        return static_cast< T >( n );
                    }
                    else
                    {
                        throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                    }
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template< class T >
struct value_deserializer< T, 
    std::enable_if_t<
        std::numeric_limits< T >::is_integer && 
        !std::numeric_limits< T >::is_signed && 
        sizeof( T ) == 8 > >
{
    T 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static T get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode <= typecode::positive_fixint_max )
        {
            return static_cast< T >( tcode );
        }
        else
        {
            switch( tcode )
            {
                case typecode::bool_true:
                    return static_cast< T >( 1 );
                case typecode::bool_false:
                    return static_cast< T >( 0 );
                case typecode::uint_8:
                    return static_cast< T >( is.get_num< uint8_t >() );
                case typecode::uint_16:
                    return static_cast< T >( is.get_num< uint16_t >() );
                case typecode::uint_32:
                    return static_cast< T >( is.get_num< uint32_t >() );
                case typecode::uint_64:
                    return static_cast< T >( is.get_num< uint64_t >() );
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template<>
struct value_deserializer< std::string >
{
    std::string 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static std::string get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max )
        {
            std::uint8_t mask = 0x1f;
            std::size_t length = tcode & mask;
            return is.getn( length ).to_string(); // TODO: does this work? construct string?
        }
        else
        {
            switch( tcode )
            {
                case typecode::str_8:
                {
                    std::size_t length = is.get_num< std::uint8_t >();
                    return is.getn( length ).to_string();
                }
                case typecode::str_16:
                {
                    std::size_t length = is.get_num< std::uint16_t >();
                    return is.getn( length ).to_string();
                }
                case typecode::str_32:
                {
                    std::size_t length = is.get_num< std::uint32_t >();
                    return is.getn( length ).to_string();
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};	

template<>
struct value_deserializer< nodeoze::string_alias >
{
    nodeoze::string_alias 
    operator()( inumstream& is ) const
    {
        return get( is );
    }

    static nodeoze::string_alias get( inumstream& is, std::size_t length )
    {
        return nodeoze::string_alias{ is.getn( length ) };
    }

    static nodeoze::string_alias get( inumstream& is )
    {
        auto tcode = is.get();
        if ( tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max )
        {
            std::uint8_t mask = 0x1f;
            std::size_t length = tcode & mask;
            return get( is, length );
        }
        else
        {
            switch( tcode )
            {
                case typecode::str_8:
                {
                    std::size_t length = is.get_num< std::uint8_t >();
                    return get( is, length );
                }
                case typecode::str_16:
                {
                    std::size_t length = is.get_num< std::uint16_t >();
                    return get( is, length );
                }
                case typecode::str_32:
                {
                    std::size_t length = is.get_num< std::uint32_t >();
                    return get( is, length );
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_NUMERIC_DESERIALIZERS_H