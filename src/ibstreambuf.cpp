#include <nodeoze/bstream/ibstreambuf.h>

using namespace nodeoze;
using namespace bstream;

position_type
bstream::ibstreambuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
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
bstream::ibstreambuf::really_tell( seek_anchor where, std::error_code& err )
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
bstream::ibstreambuf::really_underflow( std::error_code& err )
{
    clear_error( err );
    return 0UL;
}