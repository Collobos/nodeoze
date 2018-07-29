#ifndef NODEOZE_BSTREAM_OFBSTREAM_H
#define NODEOZE_BSTREAM_OFBSTREAM_H

#include <nodeoze/bstream/obstream.h>
#include <nodeoze/bstream/utils/memory.h>
#include <fstream>
#include <system_error>

namespace nodeoze
{
namespace bstream
{

class ofbstream : public obstream
{
public:

    inline 
    ofbstream( context_base const& cntxt = get_default_context() )
    :
    obstream{ std::make_unique< obfilebuf >(), cntxt }
    {
    }

    ofbstream( ofbstream const& ) = delete;
    ofbstream( ofbstream&& ) = delete;

    inline
    ofbstream( std::unique_ptr< obfilebuf > fbuf, context_base const& cntxt = get_default_context() )
    : obstream{ std::move( fbuf ), cntxt }
    {}

    inline
    ofbstream( std::string const& filename, open_mode mode = obfilebuf::default_mode, context_base const& cntxt = get_default_context() )
    :
    obstream{ std::make_unique< obfilebuf >( filename, mode ), cntxt }
    {}

    inline
    ofbstream( std::string const& filename, open_mode mode, std::error_code& err, context_base const& cntxt = get_default_context() )
    :
    obstream{ std::make_unique< obfilebuf >(), cntxt }
    {
        get_filebuf().open( filename, mode, err );
    }

    inline void
    open( std::string const& filename, open_mode mode )
    {
        get_filebuf().open( filename, mode );
    }

    inline void
    open( std::string const& filename, open_mode mode, std::error_code& err )
    {
        get_filebuf().open( filename, mode, err );
    }

    inline bool
    is_open() const
    {
        return get_filebuf().is_open();
    }

    inline void
    flush()
    {
        get_filebuf().flush();
    }

    inline void
    flush( std::error_code& err )
    {
        get_filebuf().flush( err );
    }

    inline void
    close()
    {
        get_filebuf().close();
    }

    inline void
    close( std::error_code& err )
    {
        get_filebuf().close( err );
    }

    inline obfilebuf&
    get_filebuf()
    {
        return reinterpret_cast< obfilebuf& >( get_streambuf() );
    }

    inline obfilebuf const&
    get_filebuf() const
    {
        return reinterpret_cast< obfilebuf const& >( get_streambuf() );
    }

    inline std::unique_ptr< obfilebuf >
    release_filebuf()
    {
        return bstream::utils::static_unique_ptr_cast< obfilebuf >( release_streambuf() );
    }

protected:

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_OFBSTREAM_H