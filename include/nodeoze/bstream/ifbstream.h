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
    {
        get_filebuf().pubimbue( std::locale::classic() );
    }

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
        get_filebuf().pubimbue( std::locale::classic() );
        if ( ! get_filebuf().open( filename, mode ))
        {
           throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }
    }

    inline
    ifbstream( std::string const& filename, std::error_code& ec, ibs_context::ptr context = nullptr )
    :
    ibstream{ std::make_unique< std::filebuf >(), std::move( context ) }
    {
        clear_error( ec );
        get_filebuf().pubimbue( std::locale::classic() );
        if ( ! get_filebuf().open( filename, mode ))
        {
            ec = std::error_code{ errno, std::generic_category() };
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

    inline void
    open( std::string const& filename, std::error_code& ec )
    {
        clear_error( ec );
        if ( ! get_filebuf().open( filename, mode ) )
        {
           ec = std::error_code{ errno, std::generic_category() };
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

    inline void
    close( std::error_code& ec )
    {
        clear_error( ec );
        if ( ! get_filebuf().close() )
        {
           ec = std::error_code{ errno, std::generic_category() };
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