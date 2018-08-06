#include <nodeoze/bstream/ibmembuf.h>

using namespace nodeoze;
using namespace bstream;

buffer
ibmembuf::get_slice( size_type n )
{
    size_type available = static_cast< size_type >( gend() - gnext() );
    size_type slice_size = std::min( available, n );
    if ( slice_size < 1 )
    {
        return buffer{};
    }
    else
    {
        auto pos = gpos();
        gbump( n );
        return m_buf.slice( pos, n );
    }
}

buffer
ibmembuf::getn( size_type n, std::error_code& err )
{
    clear_error( err );
    return get_slice( n );
}

buffer
ibmembuf::getn( size_type n )
{
    return get_slice( n );
}
