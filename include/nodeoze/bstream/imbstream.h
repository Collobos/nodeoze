#ifndef NODEOZE_BSTREAM_IMBSTREAM_H
#define NODEOZE_BSTREAM_IMBSTREAM_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/membuf.h>

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
    imbstream( std::unique_ptr< imembuf > strmbuf, ibs_context::ptr context = nullptr )
    : ibstream{ std::move( strmbuf ), std::move( context ) }
    {}

    inline
    imbstream( buffer const& buf, ibs_context::ptr context = nullptr )
    :
    ibstream( std::make_unique< imembuf >( buf ), std::move( context ) )
    {}

    inline
    imbstream( buffer&& buf, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< imembuf >( std::move( buf ) ), std::move( context ) }
    {}

/*
    inline
    imbstream( buffer const& buf )
    : 
    ibstream{ std::unique_ptr< imembuf >( new imembuf( buf ) ) }
    {}

    inline
    imbstream( buffer&& buf )
    : 
    ibstream{ std::unique_ptr< imembuf>( new imembuf( std::move( buf ) ) }
    {}
*/
    inline buffer
    get_buffer()
    {
        return get_membuf().get_buffer();
    }

	virtual buffer
	getn( size_type nbytes, bool throw_on_eof = true ) override
	{
        auto remaining = get_membuf().remaining();
        auto pos = get_membuf().position();
        if ( nbytes > remaining )
        {
            if ( throw_on_eof )
            {
                throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
            }

            nbytes = remaining;
        }
        get_membuf().advance( nbytes );
        return get_buffer().slice( pos, nbytes );
	}

protected:

    inline imembuf&
    get_membuf()
    {
        return reinterpret_cast< imembuf& >( * m_strmbuf );
    }

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IMBSTREAM_H
