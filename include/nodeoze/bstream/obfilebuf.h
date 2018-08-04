#ifndef NODEOZE_BSTREAM_OBFILEBUF_H
#define NODEOZE_BSTREAM_OBFILEBUF_H

#include <system_error>
#include <cstdint>
#include <assert.h>
#include <cstdlib>
#include <algorithm>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <nodeoze/bstream/obstreambuf.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/types.h>


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

    inline
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

    inline
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

    inline
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

    inline
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

    inline void
    open( std::string const& filename, open_mode mode, std::error_code& err )
    {
        m_filename = filename;
        m_mode = mode;
        m_flags = to_flags( mode );
        really_open( err );
    }

    inline void
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

    inline void
    open( std::string const& filename, open_mode mode, int flags_override, std::error_code& err )
    {
        m_filename = filename;
        m_mode = mode;
        m_flags = flags_override;
        really_open( err );
    }

    inline void
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

    inline bool
    is_open() const noexcept
    {
        return m_is_open;
    }

    inline void
    close( std::error_code& err )
    {
        clear_error( err );
        flush( err );
        if ( err ) goto exit;
        
        {
            auto result = ::close( m_fd );
            if ( result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
            }
            m_is_open = false;
        }
    exit:
        return;
    }

    inline void
    close()
    {
        std::error_code err;
        close( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    inline void
    open( std::error_code& err )
    {
        really_open( err );
    }

    inline void
    open()
    {
        std::error_code err;
        really_open( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    inline open_mode
    mode() const noexcept
    {
        return m_mode;
    }

    inline void
    mode( open_mode m )
    {
        m_mode = m;
        m_flags = to_flags( m );
    }

    inline int
    flags() const noexcept
    {
        return m_flags;
    }

    inline void
    flags( int flags )
    {
        m_flags = flags;
    }

    inline void
    filename( std::string const& filename )
    {
        m_filename = filename;
    }

    inline std::string const&
    filename() const noexcept
    {
        return m_filename;
    }

    inline position_type
    truncate( std::error_code& err )
    {
        clear_error( err );
        position_type result = invalid_position;

        flush( err );
        if ( err ) goto exit;

        {
            auto pos = ppos();
            assert( pos == pbase_offset() );
            auto trunc_result = ::ftruncate( m_fd, pos );
            if ( trunc_result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                goto exit;
            }

            force_high_watermark( pos );
            last_touched( pos );
            result = pos;
        }

    exit:
        return result;
    }

    inline position_type
    truncate()
    {
        std::error_code err;
        auto result = truncate( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
        return result;
    }

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

    static inline bool
    is_truncate( int flags )
    {
        return ( flags & O_TRUNC ) != 0;
    }

    static inline bool
    is_append( int flags )
    {
        return ( flags & O_APPEND ) != 0;
    }

    inline void
    reset_ptrs()
    {
        auto base = m_data.data();
        set_ptrs( base, base, base + m_data.size() );
    }

    inline void 
    really_open( std::error_code& err )
    {
        clear_error( err );
        if ( m_is_open )
        {
            close( err );
            if ( err ) goto exit;
        }

        if ( ( m_flags & O_CREAT ) != 0 )
        {
            mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // set permssions to rw-r--r--
            m_fd = ::open( m_filename.c_str(), m_flags, mode );
        }
        else
        {
            m_fd = ::open( m_filename.c_str(), m_flags );
        }

        if ( m_fd < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }

        m_is_open = true;

        {
            auto end_pos = ::lseek( m_fd, 0, SEEK_END );
            if ( end_pos < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                goto exit;
            }
            force_high_watermark( end_pos );

            if ( m_mode == open_mode::at_end || is_append( m_flags ) )
            {
                pbase_offset( end_pos );
                last_touched( end_pos ); // An acceptable lie.
            }
            else
            {
                auto pos = ::lseek( m_fd, 0, SEEK_SET );
                if ( pos < 0 )
                {
                    err = std::error_code{ errno, std::generic_category() };
                    goto exit;
                }
                assert( pos == 0 );
                pbase_offset( 0 );
                last_touched( 0UL );            
            }
            reset_ptrs();
            dirty( false );
        }

    exit:
        return;
    }

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