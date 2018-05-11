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

    static constexpr std::ios::openmode mode = std::ios::in | std::ios::binary;

    ifbstream( ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< std::filebuf >(), std::move( context ) }
    {}

    ifbstream( ifbstream const& ) = delete;
    ifbstream( ifbstream&& ) = delete;

    inline
    ifbstream( std::unique_ptr< std::filebuf > fbuf, ibs_context::ptr context = nullptr )
    : ibstream{ std::move( fbuf ), std::move( context ) }
    {}

    inline
    ifbstream( std::filebuf&& fbuf, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< std::filebuf >( std::move( fbuf ) ), std::move( context ) }
    {}

    inline
    ifbstream( std::string const& filename, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< std::filebuf >(), std::move( context ) }
    {
       if ( ! get_filebuf().open( filename, mode ))
       {
           throw std::system_error{ std::error_code{ errno, std::generic_category() } };
       }
    }

    inline void
    open( std::string const& filename )
    {
        if ( ! get_filebuf().open( filename, mode ) )
        {
           throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }
    }

    inline bool
    is_open() const
    {
        return get_filebuf().is_open();
    }

    inline void
    close()
    {
        if ( ! get_filebuf().close() )
        {
           throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }
    }

    inline std::filebuf&
    get_filebuf()
    {
        return reinterpret_cast< std::filebuf& >( get_streambuf() );
    }

    inline std::filebuf const&
    get_filebuf() const
    {
        return reinterpret_cast< std::filebuf const& >( get_streambuf() );
    }

    inline std::unique_ptr< std::filebuf >
    release_filebuf()
    {
        return bstream::utils::static_unique_ptr_cast< std::filebuf >( release_streambuf() );
    }

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IFBSTREAM_H