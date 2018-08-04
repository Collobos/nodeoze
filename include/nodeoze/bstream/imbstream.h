#ifndef NODEOZE_BSTREAM_IMBSTREAM_H
#define NODEOZE_BSTREAM_IMBSTREAM_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/ibmembuf.h>
#include <nodeoze/bstream/utils/memory.h>

namespace nodeoze
{
namespace bstream
{

class imbstream : public ibstream
{
public:

    imbstream() = delete;
    imbstream( imbstream const& ) = delete;
    imbstream( imbstream&& ) = delete;

    inline
    imbstream( std::unique_ptr< ibmembuf > strmbuf, context_base const& cntxt = get_default_context() )
    : ibstream{ std::move( strmbuf ), cntxt }
    {}

    inline
    imbstream( buffer const& buf, context_base const& cntxt = get_default_context(), bool share_buffer = true )
    :
    ibstream( std::make_unique< ibmembuf >( buf ), cntxt )
    {}

    inline
    imbstream( buffer&& buf, context_base const& cntxt = get_default_context(), bool share_buffer = true )
    :
    ibstream{ std::make_unique< ibmembuf >( std::move( buf ) ), cntxt }
    {}

    inline void
    use( std::unique_ptr< ibmembuf > strmbuf )
    {
        inumstream::use( std::move( strmbuf ) );
        reset();
    }

    inline void
    use( std::unique_ptr< ibmembuf > strmbuf, std::error_code& err )
    {
        inumstream::use( std::move( strmbuf ) );
        reset( err );
    }

    inline void
    use ( buffer&& buf, bool share_buffer = true )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ), share_buffer ) );
        reset();
    }

    inline void
    use( buffer&& buf, std::error_code& err, bool share_buffer = true )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ), share_buffer ) );
        reset( err );
    }

    inline buffer
    get_buffer()
    {
        return get_membuf().get_buffer();
    }

    inline ibmembuf&
    get_membuf()
    {
        return reinterpret_cast< ibmembuf& >( * m_strmbuf );
    }

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IMBSTREAM_H
