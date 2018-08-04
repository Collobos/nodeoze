	
#include <nodeoze/bstream/numstream.h>
#include <nodeoze/bstream/ibmembuf.h>
    
using nodeoze;
using bstream;

buffer
inumstream::getn( size_type nbytes, bool throw_on_incomplete = true )
{
    if ( m_share_buffer )
    {
        auto strmbuf_ptr = m_strmbuf.get();
        auto membuf_ptr = dynamic_cast< ibmembuf* >( strmbuf_ptr );
        if ( ! membuf_ptr )
        {

        }
        else
        {
            buffer buf = get_membuf().get_slice( nbytes );
            if ( buf.size() < nbytes && throw_on_incomplete )
            {
                throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
            }
            return buf;

        }
    }
    else
    {
        buffer buf = m_strmbuf->getn( nbytes );
        if ( throw_on_incomplete && buf.size() < nbytes )
        {
            throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
        }
        return buf;
    }
}

buffer
inumstream::getn( size_type nbytes, std::error_code& err, bool err_on_incomplete = true )
{
    clear_error( err );
    buffer buf;
    if ( m_share_buffer )
    {
        auto strmbuf_ptr = m_strmbuf.get();
        auto membuf_ptr = dynamic_cast< ibmembuf* >( strmbuf_ptr );
        if ( ! membuf_ptr )
        {
            err = make_error_code( bstream::errc::ibstreambuf_not_shareable );
            goto exit;
        }
        buf = get_membuf().get_slice( nbytes );
        if ( buf.size() < nbytes && err_on_incomplete )
        {
            err = make_error_code( bstream::errc::read_past_end_of_stream );
            goto exit;
        }
    }
    else
    {
        buf = m_strmbuf->getn( nbytes, err );
        if ( err ) goto exit;

        if ( err_on_incomplete && buf.size() < nbytes )
        {
            err = make_error_code( bstream::errc::read_past_end_of_stream );
            goto exit;
        }
    }
exit:
    return buf;
}

