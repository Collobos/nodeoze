#include <nodeoze/bstream/numstream.h>
#include <nodeoze/bstream/error.h>

using namespace nodeoze;
using namespace bstream;

buffer
inumstream::getn( size_type nbytes, bool throw_on_incomplete )
{
    buffer buf = m_strmbuf->getn( nbytes );
    if ( throw_on_incomplete && buf.size() < nbytes )
    {
        throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
    }
    return buf;
}

buffer
inumstream::getn( size_type nbytes, std::error_code& err, bool err_on_incomplete )
{
    clear_error( err );
    buffer buf = m_strmbuf->getn( nbytes, err );
    if ( err ) goto exit;

    if ( err_on_incomplete && buf.size() < nbytes )
    {
        err = make_error_code( bstream::errc::read_past_end_of_stream );
        goto exit;
    }
exit:
    return buf;
}

size_type
inumstream::getn( byte_type* dst, size_type nbytes, bool throw_on_incomplete )
{
    auto result = m_strmbuf->getn( dst, nbytes );
    if ( throw_on_incomplete && result < nbytes )
    {
        throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
    }
    return result;
}

size_type
inumstream::getn( byte_type* dst, size_type nbytes, std::error_code& err, bool err_on_incomplete )
{
    clear_error( err );
    auto result = m_strmbuf->getn( dst, nbytes, err );
    if ( err ) goto exit;

    if ( err_on_incomplete && result < nbytes )
    {
        err = make_error_code( bstream::errc::read_past_end_of_stream );
        goto exit;
    }
exit:
    return result;
}