#include <nodeoze/bstream/obmembuf.h>

using namespace nodeoze;
using namespace bstream;

bool
bstream::obmembuf::really_make_writable()
{
    if ( ! m_buf.is_writable() )
    {
        position_type pos = pnext() - pbase();
        m_buf.force_unique( pos );
        auto new_base = m_buf.data();
        set_ptrs( new_base, new_base + pos, new_base + m_buf.size() );
    }
    return true;
}

void
bstream::obmembuf::really_flush( std::error_code& err )
{
    clear_error( err );
    assert( dirty() && pnext() > dirty_start() );
}

void
bstream::obmembuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto hwm = get_high_watermark();
    auto pos = ppos();
    assert( hwm < pend() - pbase() && pos <= pend() - pbase() );
    assert( last_touched() != pos );

    if ( hwm < pos )
    {
        dirty_start( pbase() + hwm );
        size_type n = static_cast< size_type >( pos - hwm );
        ::memset( dirty_start(), 0, n );
        dirty( true );

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
bstream::obmembuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
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
            auto end_pos = get_high_watermark(); // high watermark is current from flush() above
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
    }
    else if ( result >= static_cast< position_type >( pend() - pbase() ) )
    {
        resize( result );
        auto new_base = m_buf.data();
        set_ptrs( new_base, new_base + result, new_base + m_buf.size() );
    }
    else
    {
        pnext( pbase() + result );
    }

exit:
    return result;
}

void
bstream::obmembuf::really_overflow( size_type n, std::error_code& err )
{
    clear_error( err );
    assert( std::less_equal< byte_type * >()( pnext(), pend() ) );
    assert( ( pnext() - pbase() ) + n > m_buf.size() );
    auto pos = ppos();
    size_type required = ( pnext() - pbase() ) + n;
    resize( required );
    auto new_base = m_buf.data();
    set_ptrs( new_base, new_base + pos, new_base + m_buf.size() );
}