#ifndef NODEOZE_BSTREAM_OMBSTREAM_H
#define NODEOZE_BSTREAM_OMBSTREAM_H

#include <nodeoze/bstream/obstream.h>
#include <nodeoze/bstream/bstreambuf.h>

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

    inline
    ombstream( std::unique_ptr< obmembuf > strmbuf, obs_context::ptr context = nullptr )
    : obstream{ std::move( strmbuf ), std::move( context ) }
    {}

    inline
    ombstream( buffer&& buf, obs_context::ptr context = nullptr )
    :
    obstream{ std::make_unique< obmembuf >( std::move( buf ) ), std::move( context ) }
    {}
 
    inline
    ombstream( size_type size, obs_context::ptr context = nullptr )
    :
    ombstream( std::make_unique< obmembuf >( size ), std::move( context ) )
    {}

    inline buffer
    get_buffer()
    {
        return get_membuf().get_buffer();
    }

    inline buffer
    release_buffer()
    {
        return get_membuf().release_buffer();
    }

    inline void
    clear()
    {
        get_membuf().clear();
    }

    inline obmembuf&
    get_membuf()
    {
        return reinterpret_cast< obmembuf& >( * m_strmbuf );
    }

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_OMBSTREAM_H