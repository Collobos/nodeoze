#include <nodeoze/bstream/obfilebuf.h>
#include <unistd.h>

using namespace nodeoze;
using namespace bstream;

bool
obfilebuf::really_make_writable()
{
    return true;
}

void
obfilebuf::really_flush( std::error_code& err )
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
obfilebuf::really_touch( std::error_code& err )
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
obfilebuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
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
obfilebuf::really_overflow( size_type, std::error_code& err )
{
    clear_error( err );
    assert( pbase_offset() == ppos() && pnext() == pbase() );
}

void
obfilebuf::close( std::error_code& err )
{
    clear_error( err );
    flush( err );
    if ( err ) goto exit;
    
    {
        auto result = ::close( m_fd );
        if ( result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
        }
        m_is_open = false;
    }
exit:
    return;
}

void
obfilebuf::close()
{
    std::error_code err;
    close( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

void
obfilebuf::open()
{
    std::error_code err;
    really_open( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
}

position_type
obfilebuf::truncate( std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    flush( err );
    if ( err ) goto exit;

    {
        auto pos = ppos();
        assert( pos == pbase_offset() );
        auto trunc_result = ::ftruncate( m_fd, pos );
        if ( trunc_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }

        force_high_watermark( pos );
        last_touched( pos );
        result = pos;
    }

exit:
    return result;
}

position_type
obfilebuf::truncate()
{
    std::error_code err;
    auto result = truncate( err );
    if ( err )
    {
        throw std::system_error{ err };
    }
    return result;
}

void 
obfilebuf::really_open( std::error_code& err )
{
    clear_error( err );
    if ( m_is_open )
    {
        close( err );
        if ( err ) goto exit;
    }

    if ( ( m_flags & O_CREAT ) != 0 )
    {
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // set permssions to rw-r--r--
        m_fd = ::open( m_filename.c_str(), m_flags, mode );
    }
    else
    {
        m_fd = ::open( m_filename.c_str(), m_flags );
    }

    if ( m_fd < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        goto exit;
    }

    m_is_open = true;

    {
        auto end_pos = ::lseek( m_fd, 0, SEEK_END );
        if ( end_pos < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        force_high_watermark( end_pos );

        if ( m_mode == open_mode::at_end || is_append( m_flags ) )
        {
            pbase_offset( end_pos );
            last_touched( end_pos ); // An acceptable lie.
        }
        else
        {
            auto pos = ::lseek( m_fd, 0, SEEK_SET );
            if ( pos < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                goto exit;
            }
            assert( pos == 0 );
            pbase_offset( 0 );
            last_touched( 0UL );            
        }
        reset_ptrs();
        dirty( false );
    }

exit:
    return;
}

