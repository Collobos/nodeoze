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

    enum class open_mode
    {
        truncate,
        append,
        at_end,
        at_begin
    };

    inline 
    ofbstream( obs_context::ptr context = nullptr )
    :
    obstream{ std::make_unique< std::filebuf >(), std::move( context ) }
    {
        get_filebuf().pubimbue( std::locale::classic() );
    }

    ofbstream( ofbstream const& ) = delete;
    ofbstream( ofbstream&& ) = delete;

    inline
    ofbstream( std::unique_ptr< std::filebuf > fbuf, obs_context::ptr context = nullptr )
    : obstream{ std::move( fbuf ), std::move( context ) }
    {}

    inline
    ofbstream( std::filebuf&& fbuf, obs_context::ptr context = nullptr )
    :
    obstream{ std::make_unique< std::filebuf >( std::move( fbuf ) ), std::move( context ) }
    {}

    inline
    ofbstream( std::string const& filename, open_mode mode, obs_context::ptr context = nullptr )
    :
    obstream{ std::make_unique< std::filebuf >(), std::move( context ) }
    {
        get_filebuf().pubimbue( std::locale::classic() );
        if ( ! get_filebuf().open( filename, to_flags( mode ) ))
        {
           throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }
    }

    inline
    ofbstream( std::string const& filename, open_mode mode, std::error_code& ec, obs_context::ptr context = nullptr )
    :
    obstream{ std::make_unique< std::filebuf >(), std::move( context ) }
    {
        clear_error( ec );
        get_filebuf().pubimbue( std::locale::classic() );
        if ( ! get_filebuf().open( filename, to_flags( mode ) ))
        {
           ec = std::error_code{ errno, std::generic_category() };
        }
    }

    inline void
    open( std::string const& filename, open_mode mode )
    {
        if ( ! get_filebuf().open( filename, to_flags( mode ) ) )
        {
           throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }
    }

    inline void
    open( std::string const& filename, open_mode mode, std::error_code& ec )
    {
        clear_error( ec );
        if ( ! get_filebuf().open( filename, to_flags( mode ) ) )
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

protected:

    constexpr std::ios::openmode
    to_flags( open_mode mode )
    {
        switch ( mode )
        {
            case open_mode::append:
                return  std::ios::out | std::ios::app | std::ios::binary;
            case open_mode::truncate:
                return std::ios::out | std::ios::trunc | std::ios::binary;
            case open_mode::at_end:
                return std::ios::in | std::ios::out | std::ios::ate | std::ios::binary;
            case open_mode::at_begin:
                return std::ios::in | std::ios::out | std::ios::binary;
        }
    }

};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_OFBSTREAM_H