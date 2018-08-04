#ifndef NODEOZE_BSTREAM_IBMEMBUF_H
#define NODEOZE_BSTREAM_IBMEMBUF_H

#include <system_error>
#include <cstdint>
#include <assert.h>
#include <cstdlib>
#include <algorithm>
#include <nodeoze/bstream/ibstreambuf.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/types.h>

namespace nodeoze 
{
namespace bstream 
{

class ibmembuf : public ibstreambuf
{
public:

    using ibstreambuf::getn;

    inline
    ibmembuf( buffer const& buf, bool share_buffer = true )
    :
    ibstreambuf{},
    m_buf{ buf },
    m_share_buffer{ share_buffer }
    {
        set_ptrs( m_buf.rdata(), m_buf.rdata(), m_buf.rdata() + m_buf.size() );
    }

    inline
    ibmembuf( buffer&& buf, bool share_buffer = true )
    :
    ibstreambuf{},
    m_buf{ std::move( buf ) },
    m_share_buffer{ share_buffer }
    {
        set_ptrs( m_buf.rdata(), m_buf.rdata(), m_buf.rdata() + m_buf.size() );
    }

    inline buffer
    get_buffer()
    {
        return m_buf;
    }

	inline buffer&
	get_buffer_ref()
	{
		return m_buf;
	}

    inline buffer
    get_slice( size_type n )
    {
        size_type available = static_cast< size_type >( gend() - gnext() );
        size_type slice_size = std::min( available, n );
        if ( slice_size < 1 )
        {
            return buffer{};
        }
        else
        {
            auto pos = gpos();
            gbump( n );
            return m_buf.slice( pos, n );
        }
    }

    virtual buffer
    getn( size_type n, std::error_code& err ) override
    {
        clear_error( err );
        if ( m_share_buffer )
        {
            return get_slice( n );
        }
        else
        {
            return ibstreambuf::getn( n, err );
        }
    }

    virtual buffer
    getn( size_type n ) override
    {
        if ( m_share_buffer )
        {
            return get_slice( n );
        }
        else
        {
            return ibstreambuf::getn( n );
        }
        buffer buf{ n };
        auto got = getn( buf.rdata(), n );
        if ( got < n )
        {
            buf.size( got );
        }
        return buf;
    }

protected:

    inline
    ibmembuf( size_type size, bool share_buffer = true )
    :
    ibstreambuf{},
    m_buf{ size, buffer::policy::exclusive },
    m_share_buffer{ share_buffer }
    {
        set_ptrs( m_buf.rdata(), m_buf.rdata(), m_buf.rdata() + m_buf.size() );
    }

    buffer      m_buf;
    const bool  m_share_buffer;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IBMEMBUF_H