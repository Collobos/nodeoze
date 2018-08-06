#ifndef NODEOZE_BSTREAM_IFBSTREAM_H
#define NODEOZE_BSTREAM_IFBSTREAM_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/ibfilebuf.h>
#include <nodeoze/bstream/utils/memory.h>
#include <fstream>
#include <system_error>

namespace nodeoze
{
namespace bstream
{

class ifbstream : public ibstream
{
public:

    ifbstream( context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >(), cntxt }
    {}

    ifbstream( ifbstream const& ) = delete;
    ifbstream( ifbstream&& ) = delete;

    ifbstream( std::unique_ptr< ibfilebuf > fbuf, context_base const& cntxt = get_default_context() )
    : ibstream{ std::move( fbuf ), cntxt }
    {}

    ifbstream( ibfilebuf&& fbuf, context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >( std::move( fbuf ) ), cntxt }
    {}

    ifbstream( std::string const& filename, context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >( filename ), cntxt }
    {}

    ifbstream( std::string const& filename, std::error_code& err, context_base const& cntxt = get_default_context() )
    :
    ibstream{ std::make_unique< ibfilebuf >( filename, err ), cntxt }
    {}

    void
    open( std::string const& filename )
    {
        get_filebuf().open( filename );
    }

    void
    open( std::string const& filename, std::error_code& err )
    {
        get_filebuf().open( filename, err );
    }

    bool
    is_open() const
    {
        return get_filebuf().is_open();
    }

    void
    close()
    {
        get_filebuf().close();
    }

    void
    close( std::error_code& err )
    {
        get_filebuf().close( err );
    }

    ibfilebuf&
    get_filebuf()
    {
        return reinterpret_cast< ibfilebuf& >( get_streambuf() );
    }

    ibfilebuf const&
    get_filebuf() const
    {
        return reinterpret_cast< ibfilebuf const& >( get_streambuf() );
    }

    std::unique_ptr< ibfilebuf >
    release_filebuf()
    {
        return bstream::utils::static_unique_ptr_cast< ibfilebuf >( release_streambuf() );
    }

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IFBSTREAM_H