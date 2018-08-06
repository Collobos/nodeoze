#ifndef NODEOZE_BSTREAM_IBSTREAMBUF_H
#define NODEOZE_BSTREAM_IBSTREAMBUF_H

#include <nodeoze/buffer.h>
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
    ibstreambuf( byte_type * buf, size_type size )
    :
    m_gbase_offset{ 0 },
    m_gbase{ buf },
    m_gnext{ buf },
    m_gend{ buf + size }
    {}

protected:

    ibstreambuf()
    : ibstreambuf{ nullptr, 0 }
    {}

public:

    virtual ~ibstreambuf() {}

    byte_type 
    get( std::error_code& err );

    byte_type
    get();

    byte_type
    peek( std::error_code& err );

    byte_type
    peek();

    virtual buffer
    getn( size_type n, std::error_code& err );

    virtual buffer
    getn( size_type n );

    size_type 
    getn( byte_type* dst, size_type n, std::error_code& err );

    size_type 
    getn( byte_type* dst, size_type n );

    position_type
    seek( position_type position, std::error_code& err )
    {
        return really_seek( seek_anchor::begin, static_cast< offset_type >( position ), err );
    }

    position_type
    seek( position_type position );

    position_type
    seek( seek_anchor where, offset_type offset, std::error_code& err )
    {
        return really_seek( where, offset, err );
    }

    position_type
    seek( seek_anchor where, offset_type offset );

    position_type
    tell( seek_anchor where, std::error_code& err )
    {
        return really_tell( where, err );
    }

    position_type
    tell( std::error_code& err )
    {
        return really_tell( seek_anchor::current, err );
    }

    position_type
    tell( seek_anchor where = seek_anchor::current );

protected:

    byte_type*
    gbump( offset_type offset )
    {
        m_gnext += offset;
        return m_gnext;
    }

    position_type
    gpos() const
    {
        return gbase_offset() + ( gnext() - gbase() );
    }

    size_type
    underflow( std::error_code& err )
    {
        return really_underflow( err );
    }

    size_type
    underflow();

    void
    gbase_offset( position_type base_offset )
    {
        m_gbase_offset = base_offset;
    }

    position_type
    gbase_offset() const
    {
        return m_gbase_offset;
    }

    void
    set_ptrs( byte_type * base, byte_type * next, byte_type * end )
    {
        m_gbase =  base;
        m_gnext = next;
        m_gend = end;
    }

    byte_type*
    gbase() const noexcept
    {
        return m_gbase;
    }

    byte_type*
    gnext() const noexcept
    {
        return m_gnext;
    }

    byte_type*
    gend() const noexcept
    {
        return m_gend;
    }

    void
    gbase( byte_type* p ) noexcept
    {
        m_gbase = p;
    }

    void
    gnext( byte_type* p ) noexcept
    {
        m_gnext = p;
    }

    void
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