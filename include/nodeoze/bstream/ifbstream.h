#ifndef NODEOZE_BSTREAM_IFBSTREAM_H
#define NODEOZE_BSTREAM_IFBSTREAM_H

#include <nodeoze/bstream/ibstream.h>
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

    ifbstream( ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< ibfilebuf >(), std::move( context ) }
    {}

    ifbstream( ifbstream const& ) = delete;
    ifbstream( ifbstream&& ) = delete;

    inline
    ifbstream( std::unique_ptr< ibfilebuf > fbuf, ibs_context::ptr context = nullptr )
    : ibstream{ std::move( fbuf ), std::move( context ) }
    {}

    inline
    ifbstream( ibfilebuf&& fbuf, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< ibfilebuf >( std::move( fbuf ) ), std::move( context ) }
    {}

    inline
    ifbstream( std::string const& filename, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< ibfilebuf >( filename ), std::move( context ) }
    {}

    inline
    ifbstream( std::string const& filename, std::error_code& err, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< ibfilebuf >( filename, err ), std::move( context ) }
    {}

    inline void
    open( std::string const& filename )
    {
        get_filebuf().open( filename );
    }

    inline void
    open( std::string const& filename, std::error_code& err )
    {
        get_filebuf().open( filename, err );
    }

    inline bool
    is_open() const
    {
        return get_filebuf().is_open();
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

    inline ibfilebuf&
    get_filebuf()
    {
        return reinterpret_cast< ibfilebuf& >( get_streambuf() );
    }

    inline ibfilebuf const&
    get_filebuf() const
    {
        return reinterpret_cast< ibfilebuf const& >( get_streambuf() );
    }

    inline std::unique_ptr< ibfilebuf >
    release_filebuf()
    {
        return bstream::utils::static_unique_ptr_cast< ibfilebuf >( release_streambuf() );
    }

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IFBSTREAM_H