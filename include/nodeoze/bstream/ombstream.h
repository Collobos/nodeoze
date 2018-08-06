#ifndef NODEOZE_BSTREAM_OMBSTREAM_H
#define NODEOZE_BSTREAM_OMBSTREAM_H

#include <nodeoze/bstream/obstream.h>
#include <nodeoze/bstream/obmembuf.h>
#include <nodeoze/bstream/utils/memory.h>

namespace nodeoze
{
namespace bstream
{

class ombstream : public obstream
{
public:
    ombstream() = delete;
    ombstream( ombstream const& ) = delete;
    ombstream( ombstream&& ) = delete;

    ombstream( std::unique_ptr< obmembuf > strmbuf, context_base const& cntxt = get_default_context() )
    : obstream{ std::move( strmbuf ), cntxt }
    {}

    ombstream( buffer&& buf, context_base const& cntxt = get_default_context() )
    :
    obstream{ std::make_unique< obmembuf >( std::move( buf ) ), cntxt }
    {}
 
    ombstream( size_type size, context_base const& cntxt = get_default_context() )
    :
    ombstream( std::make_unique< obmembuf >( size ), cntxt )
    {}

    buffer
    get_buffer()
    {
        return get_membuf().get_buffer();
    }

    buffer
    release_buffer()
    {
        return get_membuf().release_buffer();
    }

    void
    clear()
    {
        get_membuf().clear();
    }

    obmembuf&
    get_membuf()
    {
        return reinterpret_cast< obmembuf& >( * m_strmbuf );
    }
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_OMBSTREAM_H