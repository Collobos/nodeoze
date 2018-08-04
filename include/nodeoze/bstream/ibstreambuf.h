#ifndef NODEOZE_BSTREAM_IBSTREAMBUF_H
#define NODEOZE_BSTREAM_IBSTREAMBUF_H

#include <system_error>
#include <cstdint>
#include <assert.h>
#include <cstdlib>
#include <algorithm>
#include <nodeoze/buffer.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/types.h>

namespace nodeoze 
{
namespace bstream 
{

#ifndef DOCTEST_CONFIG_DISABLE
namespace detail
{
    class ibs_test_probe;
}
#endif

class ibstreambuf
{
public:
#ifndef DOCTEST_CONFIG_DISABLE
	friend class detail::ibs_test_probe;
#endif
    inline
    ibstreambuf( byte_type * buf, size_type size )
    :
    m_gbase_offset{ 0 },
    m_gbase{ buf },
    m_gnext{ buf },
    m_gend{ buf + size }
    {}

protected:

    inline
    ibstreambuf()
    : ibstreambuf{ nullptr, 0 }
    {}

public:

    virtual ~ibstreambuf() {}

    inline byte_type 
    get( std::error_code& err )
    {
        byte_type result = 0;
        if ( gnext() >= gend() )
        {
            assert( gnext() == gend() );
            auto available = underflow( err );
            if ( err ) goto exit;
            if ( available < 1 )
            {
                err = make_error_code( bstream::errc::read_past_end_of_stream );
                goto exit;
            }
        }
        assert( gnext() < gend() );
        result = *m_gnext++;
    exit:
        return result;
    }

    inline byte_type
    get()
    {
        if ( gnext() >= gend() )
        {
            assert( gnext() == gend() );
            if ( underflow() < 1 )
            {
                throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
            }
        }
        assert( gnext() < gend() );
        return *m_gnext++;
    }

    inline byte_type
    peek( std::error_code& err )
    {
        byte_type result = 0;
        if ( gnext() >= gend() )
        {
            assert( gnext() == gend() );
            auto available = underflow( err );
            if ( err ) goto exit;
            if ( available < 1 )
            {
                err = make_error_code( bstream::errc::read_past_end_of_stream );
                goto exit;
            }
        }
        assert( gnext() < gend() );
        result = * m_gnext;
    exit:
        return result;
    }

    inline byte_type
    peek()
    {
        if ( gnext() >= gend() )
        {
            assert( gnext() == gend() );
            if ( underflow() < 1 )
            {
                throw std::system_error{ make_error_code( bstream::errc::read_past_end_of_stream ) };
            }
        }
        assert( gnext() < gend() );
        return * m_gnext;
    }

    virtual buffer
    getn( size_type n, std::error_code& err )
    {
        buffer buf{ n };
        auto got = getn( buf.rdata(), n, err );
        if ( got < n )
        {
            buf.size( got );
        }
        return buf;
    }

    virtual buffer
    getn( size_type n )
    {
        buffer buf{ n };
        auto got = getn( buf.rdata(), n );
        if ( got < n )
        {
            buf.size( got );
        }
        return buf;
    }

    inline size_type 
    getn( byte_type* dst, size_type n, std::error_code& err )
    {
        size_type available = gend() - gnext();
        size_type remaining = n;
        byte_type* bp = dst;
        while ( remaining > 0 )
        {
            if ( available < 1 )
            {
                underflow( err );
                if ( err ) goto exit;
                available = gend() - gnext();
                if ( available < 1 )
                {
                    break;
                }
            }
            size_type chunk_size = std::min( remaining, available );
            ::memcpy( bp, gnext(), chunk_size );
            bp += chunk_size;
            m_gnext += chunk_size;
            remaining -= chunk_size;
            available = gend() - gnext();
        }
    exit:
        return n - remaining; 
    }

    inline size_type 
    getn( byte_type* dst, size_type n )
    {
        size_type available = gend() - gnext();
        size_type remaining = n;
        byte_type* bp = dst;
        while ( remaining > 0 )
        {
            if ( available < 1 )
            {
                underflow();
                available = gend() - gnext();
                if ( available < 1 )
                {
                    break;
                }
            }
            size_type chunk_size = std::min( remaining, available );
            ::memcpy( bp, gnext(), chunk_size );
            bp += chunk_size;
            m_gnext += chunk_size;
            remaining -= chunk_size;
            available = gend() - gnext();
        }
        return n - remaining; 
    }

    inline position_type
    seek( position_type position, std::error_code& err )
    {
        return really_seek( seek_anchor::begin, static_cast< offset_type >( position ), err );
    }

    inline position_type
    seek( position_type position )
    {
        std::error_code err;
        auto result = really_seek( seek_anchor::begin, static_cast< offset_type >( position ), err );
        if ( err )
        {
            throw std::system_error{ err };
        }
        return result;
    }

    inline position_type
    seek( seek_anchor where, offset_type offset, std::error_code& err )
    {
        return really_seek( where, offset, err );
    }

    inline position_type
    seek( seek_anchor where, offset_type offset )
    {
        std::error_code err;
        auto result = really_seek( where, offset, err );
        if ( err )
        {
            throw std::system_error{ err };
        }
        return result;
    }

    inline position_type
    tell( seek_anchor where, std::error_code& err )
    {
        return really_tell( where, err );
    }

    inline position_type
    tell( std::error_code& err )
    {
        return really_tell( seek_anchor::current, err );
    }

    inline position_type
    tell( seek_anchor where = seek_anchor::current )
    {
        std::error_code err;
        auto result = really_tell( where, err );
        if ( err )
        {
            throw std::system_error{ err };
        }
        return result;
    }

protected:

    inline byte_type*
    gbump( offset_type offset )
    {
        m_gnext += offset;
        return m_gnext;
    }

    inline position_type
    gpos() const
    {
        return gbase_offset() + ( gnext() - gbase() );
    }

    inline size_type
    underflow( std::error_code& err )
    {
        return really_underflow( err );
    }

    inline size_type
    underflow()
    {
        std::error_code err;
        auto available = really_underflow( err );
        if ( err )
        {
            throw std::system_error{ err };
        }
        return available;
    }

    inline void
    gbase_offset( position_type base_offset )
    {
        m_gbase_offset = base_offset;
    }

    inline position_type
    gbase_offset() const
    {
        return m_gbase_offset;
    }

    inline void
    set_ptrs( byte_type * base, byte_type * next, byte_type * end )
    {
        m_gbase =  base;
        m_gnext = next;
        m_gend = end;
    }

    inline byte_type*
    gbase() const noexcept
    {
        return m_gbase;
    }

    inline byte_type*
    gnext() const noexcept
    {
        return m_gnext;
    }

    inline byte_type*
    gend() const noexcept
    {
        return m_gend;
    }

    inline void
    gbase( byte_type* p ) noexcept
    {
        m_gbase = p;
    }

    inline void
    gnext( byte_type* p ) noexcept
    {
        m_gnext = p;
    }

    inline void
    gend( byte_type* p ) noexcept
    {
        m_gend = p;
    }

    // derived implementations override the following virtuals:

    virtual position_type
    really_seek( seek_anchor where, offset_type offset, std::error_code& err );

    virtual position_type
    really_tell( seek_anchor where, std::error_code& err );
    
    virtual size_type
    really_underflow( std::error_code& err );

    position_type               m_gbase_offset;
    byte_type*                  m_gbase;
    byte_type*                  m_gnext;
    byte_type*                  m_gend;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IBSTREAMBUF_H