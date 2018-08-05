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
    imbstream( buffer const& buf, context_base const& cntxt = get_default_context(), buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstream( std::make_unique< ibmembuf >( buf, pol ), cntxt )
    {}

    inline
    imbstream( buffer&& buf, context_base const& cntxt = get_default_context(), buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstream{ std::make_unique< ibmembuf >( std::move( buf ), pol ), cntxt }
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
    use ( buffer&& buf, buffer::policy pol = buffer::policy::copy_on_write )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ), pol ) );
        reset();
    }

    inline void
    use( buffer&& buf, std::error_code& err, buffer::policy pol = buffer::policy::copy_on_write )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ), pol ) );
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
