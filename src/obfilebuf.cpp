#include <nodeoze/bstream/obfilebuf.h>

using namespace nodeoze;
using namespace bstream;

bool
bstream::obfilebuf::really_make_writable()
{
    return true;
}

void
bstream::obfilebuf::really_flush( std::error_code& err )
{
    clear_error( err );
    auto pos = ppos();
    assert( dirty() && pnext() > dirty_start() );
    assert( dirty_start() == pbase() );
    if ( last_touched() != pbase_offset() )
    {
        auto seek_result = ::lseek( m_fd, pbase_offset(), SEEK_SET );
        if ( seek_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        assert( seek_result == pbase_offset() );
    }

    {
        size_type n = static_cast< size_type >( pnext() - pbase() );
        auto write_result = ::write( m_fd, pbase(), n );
        if ( write_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        assert( static_cast< size_type >( write_result ) == n );
        pbase_offset( pos );
        pnext( pbase() );
    }
exit:
    return;
}

void
bstream::obfilebuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto pos = ppos();
    assert( pbase_offset() == pos && pnext() == pbase() );
    assert( last_touched() != pos );

    auto result = ::lseek( m_fd, pos, SEEK_SET );
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        goto exit;
    }
    last_touched( pos );
    
exit:
    return;
}

position_type
bstream::obfilebuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
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

    if ( result < 0 )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
        goto exit;
    }

    pbase_offset( result );
    pnext( pbase() );

exit:
    return result;
}

void
bstream::obfilebuf::really_overflow( size_type, std::error_code& err )
{
    clear_error( err );
    assert( pbase_offset() == ppos() && pnext() == pbase() );
}