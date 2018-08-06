#ifndef NODEOZE_BSTREAM_OBFILEBUF_H
#define NODEOZE_BSTREAM_OBFILEBUF_H

#include <fcntl.h>
#include <vector>
#include <nodeoze/bstream/obstreambuf.h>

#ifndef NODEOZE_BSTREAM_DEFAULT_OBFILEBUF_SIZE
#define NODEOZE_BSTREAM_DEFAULT_OBFILEBUF_SIZE  16384UL
#endif

namespace nodeoze 
{
namespace bstream 
{

class obfilebuf : public obstreambuf
{
public:

    static constexpr open_mode default_mode = open_mode::at_begin;

    obfilebuf( obfilebuf&& rhs )
    :
    obstreambuf{ std::move( rhs ) },
    m_data{ std::move( rhs.m_data ) },
    m_filename{ std::move( rhs.m_filename ) },
    m_is_open{ rhs.m_is_open },
    m_mode{ rhs.m_mode },
    m_flags{ rhs.m_flags },
    m_fd{ rhs.m_fd }
    {}

    obfilebuf( std::string const& filename, open_mode mode, std::error_code& err, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_OBFILEBUF_SIZE )
    :
    obstreambuf{},
    m_data( buffer_size ),
    m_filename{ filename },
    m_is_open{ false },
    m_mode{ mode },
    m_flags{ to_flags( mode ) },
    m_fd{ -1 }
    {
        reset_ptrs();
        really_open( err );
    }

    obfilebuf( std::string const& filename, open_mode mode = default_mode, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_OBFILEBUF_SIZE )
    :
    obstreambuf{},
    m_data( buffer_size ),
    m_filename{ filename },
    m_is_open{ false },
    m_mode{ mode },
    m_flags{ to_flags( mode ) },
    m_fd{ -1 }
    {
        reset_ptrs();
        std::error_code err;
        really_open( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    obfilebuf( open_mode mode = default_mode, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_OBFILEBUF_SIZE )
    :
    obstreambuf{},
    m_data( buffer_size ),
    m_filename{},
    m_is_open{ false },
    m_mode{ mode },
    m_flags{ to_flags( m_mode ) },
    m_fd{ -1 }
    {
        reset_ptrs();
    }

    void
    open( std::string const& filename, open_mode mode, std::error_code& err )
    {
        m_filename = filename;
        m_mode = mode;
        m_flags = to_flags( mode );
        really_open( err );
    }

    void
    open( std::string const& filename, open_mode mode = default_mode )
    {
        m_filename = filename;
        m_mode = mode;
        m_flags = to_flags( mode );
        std::error_code err;
        really_open( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    void
    open( std::string const& filename, open_mode mode, int flags_override, std::error_code& err )
    {
        m_filename = filename;
        m_mode = mode;
        m_flags = flags_override;
        really_open( err );
    }

    void
    open( std::string const& filename, open_mode mode, int flags_override )
    {
        m_filename = filename;
        m_mode = mode;
        m_flags = flags_override;
        std::error_code err;
        really_open( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    bool
    is_open() const noexcept
    {
        return m_is_open;
    }

    void
    close( std::error_code& err );

    void
    close();

    void
    open( std::error_code& err )
    {
        really_open( err );
    }

    void
    open();

    open_mode
    mode() const noexcept
    {
        return m_mode;
    }

    void
    mode( open_mode m )
    {
        m_mode = m;
        m_flags = to_flags( m );
    }

    int
    flags() const noexcept
    {
        return m_flags;
    }

    void
    flags( int flags )
    {
        m_flags = flags;
    }

    void
    filename( std::string const& filename )
    {
        m_filename = filename;
    }

    std::string const&
    filename() const noexcept
    {
        return m_filename;
    }

    position_type
    truncate( std::error_code& err );

    position_type
    truncate();

protected:

    virtual bool
    really_make_writable() override;

    virtual void
    really_flush( std::error_code& err ) override;

    virtual void
    really_touch( std::error_code& err ) override;

    virtual position_type
    really_seek( seek_anchor where, offset_type offset, std::error_code& err ) override;

    virtual void
    really_overflow( size_type, std::error_code& err ) override;

private:

    static bool
    is_truncate( int flags )
    {
        return ( flags & O_TRUNC ) != 0;
    }

    static bool
    is_append( int flags )
    {
        return ( flags & O_APPEND ) != 0;
    }

    void
    reset_ptrs()
    {
        auto base = m_data.data();
        set_ptrs( base, base, base + m_data.size() );
    }

    void 
    really_open( std::error_code& err );

    constexpr int
    to_flags( open_mode mode )
    {
        switch ( mode )
        {
            case open_mode::append:
                return O_WRONLY | O_CREAT | O_APPEND;
            case open_mode::truncate:
                return O_WRONLY | O_CREAT | O_TRUNC;
            default:
                return O_WRONLY | O_CREAT;
        }
    }

    std::vector< byte_type >    m_data;
    std::string                 m_filename;
    bool                        m_is_open;
    open_mode                   m_mode;
    int                         m_flags;
    int                         m_fd;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_OBMEMBUF_H