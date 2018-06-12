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
    imbstream( std::unique_ptr< ibmembuf > strmbuf, ibs_context::ptr context = nullptr )
    : ibstream{ std::move( strmbuf ), std::move( context ) }
    {}

    inline
    imbstream( buffer const& buf, ibs_context::ptr context = nullptr )
    :
    ibstream( std::make_unique< ibmembuf >( buf ), std::move( context ) )
    {}

    inline
    imbstream( buffer&& buf, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< ibmembuf >( std::move( buf ) ), std::move( context ) }
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

    inline void
    use( std::unique_ptr< ibmembuf > strmbuf, ibs_context::ptr context )
    {
        inumstream::use( std::move( strmbuf ) );
        m_context = std::move( context );
        reset();
    }

    inline void
    use( std::unique_ptr< ibmembuf > strmbuf, ibs_context::ptr context, std::error_code& err )
    {
        inumstream::use( std::move( strmbuf ) );
        m_context = std::move( context );
        reset( err );
    }

    inline void
    use( buffer&& buf, ibs_context::ptr context )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ) ) );
        m_context = std::move( context );
        reset();
    }

    inline void
    use( buffer&& buf, ibs_context::ptr context, std::error_code& err )
    {
        inumstream::use( std::make_unique< ibmembuf >( std::move( buf ) ) );
        m_context = std::move( context );
        reset( err );
    }

/*
    inline
    imbstream( buffer const& buf )
    : 
    ibstream{ std::unique_ptr< ibmembuf >( new ibmembuf( buf ) ) }
    {}

    inline
    imbstream( buffer&& buf )
    : 
    ibstream{ std::unique_ptr< ibmembuf>( new ibmembuf( std::move( buf ) ) }
    {}
*/
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
