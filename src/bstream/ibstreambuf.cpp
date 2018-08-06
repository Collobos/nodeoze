#include <nodeoze/bstream/ibstreambuf.h>
#include <nodeoze/bstream/error.h>

using namespace nodeoze;
using namespace bstream;

position_type
ibstreambuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            result = ( gend() - gbase() ) + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 || result > ( gend() - gbase() ) )
    {
        err = make_error_code( std::errc::invalid_seek );
        result = invalid_position;
        goto exit;
    }

    gnext( gbase() + result );

exit:
    return result;

}

position_type
ibstreambuf::really_tell( seek_anchor where, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos();
        }
        break;
        
        case seek_anchor::end:
        {
            result = static_cast< position_type >( gend() - gbase() );
        }
        break;

        case seek_anchor::begin:
        {
            result = 0;
        }
        break;
    }

    return result;
}

size_type
ibstreambuf::really_underflow( std::error_code& err )
{
    clear_error( err );
    return 0UL;
}

byte_type 
ibstreambuf::get( std::error_code& err )
{
    byte_type result = 0;
    if ( gnext() >= gend() )
    {
        assert( gnext() == gend() );
        auto available = underflow( err );
        if ( err ) goto exit;
        if ( available < 1 )
        {
            err = make_error_code( bstream::errc::read_past_end_of_stream );
            goto exit;
        }
    }
    assert( gnext() < gend() );
    result = *m_gnext++;
exit:
    return result;
}

byte_type
ibstreambuf::get()
{
    if ( gnext() >= gend() )
    {
        assert( gnext() == gend() );
        if ( underflow() < 1 )
        {
            throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
        }
    }
    assert( gnext() < gend() );
    return *m_gnext++;
}

byte_type
ibstreambuf::peek( std::error_code& err )
{
    byte_type result = 0;
    if ( gnext() >= gend() )
    {
        assert( gnext() == gend() );
        auto available = underflow( err );
        if ( err ) goto exit;
        if ( available < 1 )
        {
            err = make_error_code( bstream::errc::read_past_end_of_stream );
            goto exit;
        }
    }
    assert( gnext() < gend() );
    result = * m_gnext;
exit:
    return result;
}

byte_type
ibstreambuf::peek()
{
    if ( gnext() >= gend() )
    {
        assert( gnext() == gend() );
        if ( underflow() < 1 )
        {
            throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
        }
    }
    assert( gnext() < gend() );
    return * m_gnext;
}

buffer
ibstreambuf::getn( size_type n, std::error_code& err )
{
    buffer buf{ n };
    auto got = getn( buf.data(), n, err );
    if ( got < n )
    {
        buf.size( got );
    }
    return buf;
}

buffer
ibstreambuf::getn( size_type n )
{
    buffer buf{ n };
    auto got = getn( buf.data(), n );
    if ( got < n )
    {
        buf.size( got );
    }
    return buf;
}

size_type 
ibstreambuf::getn( byte_type* dst, size_type n, std::error_code& err )
{
    size_type available = gend() - gnext();
    size_type remaining = n;
    byte_type* bp = dst;
    while ( remaining > 0 )
    {
        if ( available < 1 )
        {
            underflow( err );
            if ( err ) goto exit;
            available = gend() - gnext();
            if ( available < 1 )
            {
                break;
            }
        }
        size_type chunk_size = std::min( remaining, available );
        ::memcpy( bp, gnext(), chunk_size );
        bp += chunk_size;
        m_gnext += chunk_size;
        remaining -= chunk_size;
        available = gend() - gnext();
    }
exit:
    return n - remaining; 
}

size_type 
ibstreambuf::getn( byte_type* dst, size_type n )
{
    size_type available = gend() - gnext();
    size_type remaining = n;
    byte_type* bp = dst;
    while ( remaining > 0 )
    {
        if ( available < 1 )
        {
            underflow();
            available = gend() - gnext();
            if ( available < 1 )
            {
                break;
            }
        }
        size_type chunk_size = std::min( remaining, available );
        ::memcpy( bp, gnext(), chunk_size );
        bp += chunk_size;
        m_gnext += chunk_size;
        remaining -= chunk_size;
        available = gend() - gnext();
    }
    return n - remaining; 
}

position_type
ibstreambuf::seek( position_type position )
{
    std::error_code err;
    auto result = really_seek( seek_anchor::begin, static_cast< offset_type >( position ), err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

position_type
ibstreambuf::seek( seek_anchor where, offset_type offset )
{
    std::error_code err;
    auto result = really_seek( where, offset, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

position_type
ibstreambuf::tell( seek_anchor where )
{
    std::error_code err;
    auto result = really_tell( where, err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

size_type
ibstreambuf::underflow()
{
    std::error_code err;
    auto available = really_underflow( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return available;
}

