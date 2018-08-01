#ifndef NODEOZE_BSTREAM_IMBSTREAM_H
#define NODEOZE_BSTREAM_IMBSTREAM_H

#include <nodeoze/bstream/ibstream.h>

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
    imbstream( buffer const& buf, context_base const& cntxt = get_default_context() )
    :
    ibstream( std::make_unique< ibmembuf >( buf ), cntxt )
    {}

    inline
    imbstream( buffer&& buf, context_base const& cntxt = get_default_context() )
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
    use ( buffer&& buf )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ) ) );
        reset();
    }

    inline void
    use( buffer&& buf, std::error_code& err )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ) ) );
        reset( err );
    }

    inline buffer
    get_buffer()
    {
        return get_membuf().get_buffer();
    }

	virtual buffer
	getn( size_type nbytes, bool throw_on_incomplete = true ) override
	{
        buffer buf = get_membuf().get_slice( nbytes );
        if ( buf.size() < nbytes && throw_on_incomplete )
        {
            throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
        }
        return buf;
	}

    virtual buffer
    getn( size_type nbytes, std::error_code& err, bool error_on_incomplete = true ) override
    {
        clear_error( err );
        buffer buf = get_membuf().get_slice( nbytes );
        if ( buf.size() < nbytes && error_on_incomplete )
        {
            err = make_error_code( bstream::errc::read_past_end_of_stream );
        }
        return buf;
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
