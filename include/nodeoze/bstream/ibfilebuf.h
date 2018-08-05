#ifndef NODEOZE_BSTREAM_IBFILEBUF_H
#define NODEOZE_BSTREAM_IBFILEBUF_H

#include <system_error>
#include <cstdint>
#include <assert.h>
#include <cstdlib>
#include <algorithm>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <nodeoze/bstream/ibmembuf.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/types.h>


#ifndef NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE
#define NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE  16384UL
#endif

namespace nodeoze 
{
namespace bstream 
{

class ibfilebuf : public ibstreambuf
{
public:

    inline
    ibfilebuf( size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE )
    :
    m_buf{ buffer_size, buffer::policy::exclusive },
    m_filename{},
    m_is_open{ false },
    m_flags{ O_RDONLY },
    m_fd{ -1 }
    {}

    inline
    ibfilebuf( std::string const& filename, std::error_code& err, int flag_overrides = 0, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE )
    :
    m_buf{ buffer_size, buffer::policy::exclusive },
    m_filename{ filename },
    m_is_open{ false },
    m_flags{ O_RDONLY | flag_overrides },
    m_fd{ -1 }
    {
        really_open( err );
    }

    inline
    ibfilebuf( std::string const& filename, int flag_overrides = 0, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE )
    :
    m_buf{ buffer_size, buffer::policy::exclusive },
    m_filename{ filename },
    m_is_open{ false },
    m_flags{ O_RDONLY | flag_overrides },
    m_fd{ -1 }
    {
        std::error_code err;
        really_open( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    inline void
    open( std::string const& filename, std::error_code& err, int flag_overrides = 0 )
    {
        m_filename = filename;
        m_flags = O_RDONLY | flag_overrides;
        really_open( err );
    }

    inline void
    open( std::string const& filename, int flag_overrides = 0 )
    {
        m_filename = filename;
        m_flags = O_RDONLY | flag_overrides;
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
        if ( m_is_open )
        {
            auto close_result = ::close( m_fd );
            if ( close_result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                goto exit;
            }

            m_is_open = false;
        }

    exit:
        return;
    }

    inline void
    close()
    {
        if ( m_is_open )
        {
            auto close_result = ::close( m_fd );
            if ( close_result < 0 )
            {
                throw std::system_error{ std::error_code{ errno, std::generic_category() } };
            }

            m_is_open = false;
        }
    }

protected:

    virtual position_type
    really_seek( seek_anchor where, offset_type offset, std::error_code& err ) override;

    virtual position_type
    really_tell( seek_anchor where, std::error_code& err ) override;

    virtual size_type
    really_underflow( std::error_code& err ) override;

private:

    inline size_type
    load_buffer( std::error_code& err )
    {
        clear_error( err );
        assert( gnext() == gbase() );

        auto read_result = ::read( m_fd, gbase(), m_buf.size() );
        if ( read_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            read_result = 0;
            goto exit;
        }       
        gend( gnext() + read_result );

    exit:
        return read_result;
    }

    inline void
    reset_ptrs()
    {
        auto base = m_buf.data();
        set_ptrs( base, base, base );
    }

    void really_open( std::error_code& err )
    {
        clear_error( err );
        if ( m_is_open )
        {
            close( err );
            if ( err ) goto exit;
            m_is_open = false;
        }

        m_fd = ::open( m_filename.c_str(), m_flags );
        if ( m_fd < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }

        m_is_open = true;
        reset_ptrs();

    exit:
        return;
    }

    buffer              m_buf;
    std::string         m_filename;
    bool                m_is_open;
    int                 m_flags;
    int                 m_fd;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IBFILEBUF_H