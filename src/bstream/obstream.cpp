#include <nodeoze/bstream/obstream.h>

using namespace nodeoze;
using namespace bstream;

obstream&
obstream::write_map_header(std::uint32_t size )
{
    if (size <= 15 )
    {
        std::uint8_t code = 0x80 | static_cast< std::uint8_t >(size );
        put(code );
    }
    else if (size <= std::numeric_limits< std::uint16_t >::max() )
    {
        put(typecode::map_16 );
        put_num(static_cast< std::uint16_t >(size ) );
    }
    else
    {
        put(typecode::map_32 );
        put_num(static_cast< std::uint32_t >(size ) );
    }
    return *this;
}

obstream&
obstream::write_map_header(std::uint32_t size, std::error_code& err )
{
    clear_error( err );
    try
    {
        write_map_header( size );
    }
    catch ( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}

obstream&
obstream::write_array_header(std::uint32_t size )
{
    if (size <= 15 )
    {
        std::uint8_t code = 0x90 | static_cast< std::uint8_t >(size );
        put(code );
    }
    else if (size <= std::numeric_limits< std::uint16_t >::max() )
    {
        put(typecode::array_16 );
        put_num(static_cast< std::uint16_t >(size ) );
    }
    else
    {
        put(typecode::array_32 );
        put_num(static_cast< std::uint32_t >(size ) );
    }
    return *this;
}      

obstream&
obstream::write_array_header(std::uint32_t size, std::error_code& err )
{
    clear_error( err );
    try
    {
        write_array_header( size );
    }
    catch ( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}

obstream&
obstream::write_blob_header(std::uint32_t size )
{
    if (size <= std::numeric_limits< std::uint8_t >::max() )
    {
        put(typecode::bin_8 );
        put_num(static_cast< std::uint8_t >(size ) );
    }
    else if (size <= std::numeric_limits< std::uint16_t >::max() )
    {
        put(typecode::bin_16 );
        put_num(static_cast< std::uint16_t >(size ) );
    }
    else
    {
        put(typecode::bin_32 );
        put_num(static_cast< std::uint32_t >(size ) );
    }
    return *this;
}

obstream&
obstream::write_blob_header(std::uint32_t size, std::error_code& err )
{
    clear_error( err );
    try
    {
        write_blob_header( size );
    }
    catch ( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}

obstream&
obstream::write_ext( std::uint8_t ext_type, buffer const& buf )
{
    auto size = buf.size();
    switch ( size )
    {
        case 1:
            put_num( typecode::fixext_1 ).put_num( ext_type ).put_num( buf[ 0 ] );
            break;
        case 2:
            put_num( typecode::fixext_2 ).put_num( ext_type ).put_num( buf[ 0 ] ).put_num( buf[ 1 ] );
            break;
        case 4:
            put_num( typecode::fixext_4 ).put_num( ext_type ).putn( buf.data(), 4 );
            break;
        case 8:
            put_num( typecode::fixext_8 ).put_num( ext_type ).putn( buf.data(), 8 );
            break;
        case 16:
            put_num( typecode::fixext_16 ).put_num( ext_type ).putn( buf.data(), 16 );
            break;
        default:
            if ( size <= std::numeric_limits< std::uint8_t >::max() )
            {
                put_num( typecode::ext_8 ).put_num( static_cast< std::uint8_t >( size ) ).put_num( ext_type );
            }
            else if ( size <= std::numeric_limits< std::uint16_t >::max() )
            {
                put_num( typecode::ext_16 ).put_num( static_cast< std::uint16_t >( size ) ).put_num( ext_type );
            }
            else if ( size <= std::numeric_limits< std::uint32_t >::max() )
            {
                put_num( typecode::ext_32 ).put_num( static_cast< std::uint32_t >( size ) ).put_num( ext_type );
            }
            else
            {
                throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
            }
            putn( buf.data(), size );
    }
    return *this;
}

obstream&
obstream::write_ext( std::uint8_t ext_type, buffer const& buf, std::error_code& err )
{
    clear_error( err );
    try
    {
        write_ext( ext_type, buf );
    }
    catch ( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}

obstream&
obstream::write_ext( std::uint8_t ext_type, std::vector< std::uint8_t > const& vec )
{
    auto size = vec.size();
    switch ( size )
    {
        case 1:
            put_num( typecode::fixext_1 ).put_num( ext_type ).put_num( vec[ 0 ] );
            break;
        case 2:
            put_num( typecode::fixext_2 ).put_num( ext_type ).put_num( vec[ 0 ] ).put_num( vec[ 1 ] );
            break;
        case 4:
            put_num( typecode::fixext_4 ).put_num( ext_type ).putn( vec.data(), 4 );
            break;
        case 8:
            put_num( typecode::fixext_8 ).put_num( ext_type ).putn( vec.data(), 8 );
            break;
        case 16:
            put_num( typecode::fixext_16 ).put_num( ext_type ).putn( vec.data(), 16 );
            break;
        default:
            if ( size <= std::numeric_limits< std::uint8_t >::max() )
            {
                put_num( typecode::ext_8 ).put_num( static_cast< std::uint8_t >( size ) ).put_num( ext_type );
            }
            else if ( size <= std::numeric_limits< std::uint16_t >::max() )
            {
                put_num( typecode::ext_16 ).put_num( static_cast< std::uint16_t >( size ) ).put_num( ext_type );
            }
            else if ( size <= std::numeric_limits< std::uint32_t >::max() )
            {
                put_num( typecode::ext_32 ).put_num( static_cast< std::uint32_t >( size ) ).put_num( ext_type );
            }
            else
            {
                throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
            }
            putn( vec.data(), size );
    }
    return *this;
}

obstream&
obstream::write_ext( std::uint8_t ext_type, std::vector< std::uint8_t > const& vec, std::error_code& err )
{
    clear_error( err );
    try
    {
        write_ext( ext_type, vec );
    }
    catch( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}

obstream&
obstream::write_ext_header( std::uint8_t ext_type, std::uint32_t size )
{
    switch ( size )
    {
        case 1:
            put_num( typecode::fixext_1 ).put_num( ext_type );
            break;
        case 2:
            put_num( typecode::fixext_2 ).put_num( ext_type );
            break;
        case 4:
            put_num( typecode::fixext_4 ).put_num( ext_type );
            break;
        case 8:
            put_num( typecode::fixext_8 ).put_num( ext_type );
            break;
        case 16:
            put_num( typecode::fixext_16 ).put_num( ext_type );
            break;
        default:
            if ( size <= std::numeric_limits< std::uint8_t >::max() )
            {
                put_num( typecode::ext_8 ).put_num( static_cast< std::uint8_t >( size ) ).put_num( ext_type );
            }
            else if ( size <= std::numeric_limits< std::uint16_t >::max() )
            {
                put_num( typecode::ext_16 ).put_num( static_cast< std::uint16_t >( size ) ).put_num( ext_type );
            }
            else if ( size <= std::numeric_limits< std::uint32_t >::max() )
            {
                put_num( typecode::ext_32 ).put_num( static_cast< std::uint32_t >( size ) ).put_num( ext_type );
            }
            else
            {
                throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
            }
    }
    return *this;
}

obstream&
obstream::write_ext_header( std::uint8_t ext_type, std::uint32_t size, std::error_code& err  )
{
    clear_error( err );
    try
    {
        write_ext_header( ext_type, size );
    }
    catch( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}	

obstream&
obstream::write_ext( std::uint8_t ext_type, void* data, std::uint32_t size )
{
    write_ext_header( ext_type, size );
    putn( data, size );
    return *this;
}

obstream&
obstream::write_ext( std::uint8_t ext_type, void* data, std::uint32_t size, std::error_code& err )
{
    write_ext_header( ext_type, size, err );
    if ( err ) goto exit;
    putn( data, size, err );
exit:
    return *this;
}

obstream&
obstream::obstream::write_null_ptr()
{
	write_array_header(2 );
	*this << invalid_tag;	// type tag
	write_nil();
	return *this;
}

obstream&
obstream::write_null_ptr( std::error_code& err )
{
    clear_error( err );
    try
    {
        write_null_ptr();
    }
    catch ( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}

obstream&
obstream::write_error_code( std::error_code const& ecode )
{
	auto category_index = m_context->index_of_category( ecode.category() );
	write_array_header( 2 );
	*this << category_index;
	*this << ecode.value();
	return *this;
}

obstream&
obstream::write_error_code( std::error_code const& ecode, std::error_code& err )
{
    clear_error( err );
    try
    {
        write_error_code( ecode );
    }
    catch ( std::system_error const& e )
    {
        err = e.code();
    }
    return *this;
}

std::size_t 
obstream::map_header_size(std::size_t map_size )
{
    if (map_size < 16 ) 
        return 1;
    else if (map_size <= std::numeric_limits< std::uint16_t >::max() )
        return 3;
    else return 5;
}
    
std::size_t 
obstream::array_header_size(std::size_t array_size )
{
    if (array_size < 16 ) 
        return 1;
    else if (array_size <= std::numeric_limits< std::uint16_t >::max() )
        return 3;
    else return 5;
}
    
std::size_t 
obstream::blob_header_size(std::size_t blob_size )
{
    if (blob_size < 256 ) 
        return 2;
    else if (blob_size <= std::numeric_limits< std::uint16_t >::max() )
        return 3;
    else return 5;
}
