#include <nodeoze/bstream/obstreambuf.h>

using namespace nodeoze;
using namespace bstream;

void
bstream::obstreambuf::putn( const byte_type* src, size_type n, std::error_code& err )
{
    make_writable();
    if ( ! dirty() )
    {
        touch( err );
        if ( err ) goto exit;
        dirty_start( pnext() );
    }
    if ( n <= static_cast< size_type >( pend() - pnext() ) ) // optimize for common case ( no overflow )
    {
        ::memcpy( pnext(), src, n );
        m_pnext += n;
        dirty( true );
    }
    else 
    {
        overflow( n, err );
        if ( err ) goto exit;
        if ( n <= static_cast< size_type >( pend() - pnext() ) ) // try it in one go
        {
            if ( ! dirty() )
            {
                dirty_start( pnext() );
            }
            ::memcpy( pnext(), src, n );
            m_pnext += n;
            dirty( true );
        }
        else // put it in chunks
        {
            auto remaining = n;
            auto p = src;
            while ( remaining > 0 )
            {
                if ( pnext() >= pend() ) // should be false on first iteration
                {
                    assert( pnext() == pend() );
                    overflow( remaining, err );
                    if ( err ) goto exit;
                }

                assert( pend() - pnext() > 0 );

                size_type chunk_size = std::min( remaining, static_cast< size_type >( pend() - pnext() ) );
                if ( ! dirty() )
                {
                    dirty_start( pnext() );
                }
                ::memcpy( pnext(), p, chunk_size );
                remaining -= chunk_size;
                p += chunk_size;
                m_pnext += chunk_size;
                dirty( true );
            }
        }
    }
    
exit:
    return;
}

void
bstream::obstreambuf::filln( const byte_type fill_byte, size_type n, std::error_code& err )
{
    make_writable();
    if ( ! dirty() )
    {
        touch( err );
        if ( err ) goto exit;
        dirty_start( pnext() );
    }
    if ( n <= static_cast< size_type >( pend() - pnext() ) ) // optimize for common case ( no overflow )
    {
        ::memset( pnext(), fill_byte, n );
        m_pnext += n;
        dirty( true );
    }
    else 
    {
        overflow( n, err );
        if ( err ) goto exit;
        if ( n <= static_cast< size_type >( pend() - pnext() ) ) // try it in one go
        {
            if ( ! dirty() )
            {
                dirty_start( pnext() );
            }
            ::memset( pnext(), fill_byte, n );
            m_pnext += n;
            dirty( true );
        }
        else // put it in chunks
        {
            auto remaining = n;
            while ( remaining > 0 )
            {
                if ( pnext() >= pend() ) // should be false on first iteration
                {
                    assert( pnext() == pend() );
                    overflow( remaining, err );
                    if ( err ) goto exit;
                }

                assert( pend() - pnext() > 0 );

                size_type chunk_size = std::min( remaining, static_cast< size_type >( pend() - pnext() ) );
                if ( ! dirty() )
                {
                    dirty_start( pnext() );
                }
                ::memset( pnext(), fill_byte, chunk_size );
                remaining -= chunk_size;
                m_pnext += chunk_size;
                dirty( true );
            }
        }
    }
    
exit:
    return;
}

bool
bstream::obstreambuf::really_make_writable()
{
    return true;
}

void
bstream::obstreambuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto hwm = get_high_watermark();
    auto pos = ppos();
    assert( hwm < pend() - pbase() && pos <= pend() - pbase() );
    assert( last_touched() != pos );

    if ( hwm < pos )
    {
        pnext( pbase() + ( hwm - pbase_offset() ) );

        filln( pos - hwm, 0, err );
        if ( err ) goto exit;

        flush( err );
        if ( err ) goto exit;

        assert( ppos() == pos );
        assert( get_high_watermark() == pos );
        assert( last_touched() == pos );
    }
    else
    {
        last_touched( pos );
        assert( hwm >= pos );
    }

exit:
    return;
}

position_type
bstream::obstreambuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    flush( err );
    if ( err ) goto exit;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            // high watermark will be current because of flush() above
            auto end_pos = get_high_watermark();
            result = end_pos + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 || result > pend() - pbase() )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
        goto exit;
    }

    pnext( pbase() + ( result - pbase_offset() ) );

exit:
    return result;
}

position_type
bstream::obstreambuf::really_tell( seek_anchor where, std::error_code& err )
{
    clear_error( err );

    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos();
        }
        break;
        
        case seek_anchor::end:
        {
            if ( dirty() )
            {
                set_high_watermark(); // Note: hwm may advance ahead of last_flushed and dirty_start here. This should be ok.
            }
            result = get_high_watermark();
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

void
bstream::obstreambuf::really_overflow( size_type, std::error_code& err )
{
    err = make_error_code( std::errc::no_buffer_space );
}

void
bstream::obstreambuf::really_flush( std::error_code& err )
{
    clear_error( err );
    assert( dirty() && pnext() > dirty_start() );
}

