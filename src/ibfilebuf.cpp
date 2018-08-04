#include <nodeoze/bstream/ibfilebuf.h>

using namespace nodeoze;
using namespace bstream;

position_type
bstream::ibfilebuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ::lseek( m_fd, offset, SEEK_CUR );
        }
        break;

        case seek_anchor::end:
        {
            result = ::lseek( m_fd, offset, SEEK_END );
        }
        break;

        case seek_anchor::begin:
        {
            result = ::lseek( m_fd, offset, SEEK_SET );
        }
        break;
    }
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        result = invalid_position;
        goto exit;
    }
    gbase_offset( result );
    reset_ptrs();
exit:
    return result;
}

position_type
bstream::ibfilebuf::really_tell( seek_anchor where, std::error_code& err )
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
            auto save = gbase_offset();
            result = ::lseek( m_fd, 0, SEEK_END );
            if ( result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                result = invalid_position;
                goto exit;
            }
            auto restore_result = ::lseek( m_fd, save, SEEK_SET );
            if ( restore_result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                result = invalid_position;
                goto exit;
            }
            assert( restore_result == save );
        }
        break;

        case seek_anchor::begin:
        {
            result = 0;
        }
        break;
    }
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        result = invalid_position;
        goto exit;
    }
exit:
    return result;
}

size_type
bstream::ibfilebuf::really_underflow( std::error_code& err )
{
    assert( gnext() == gend() );
    gbase_offset( gpos() );
    gnext( gbase() );
    size_type available = load_buffer( err );
    if ( err ) 
    {
        available = 0;
    }
    return available;
}
