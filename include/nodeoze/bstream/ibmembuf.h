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
    ibmembuf( buffer const& buf, buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstreambuf{},
    m_buf{ buf, pol }
    {
        set_ptrs( m_buf.rdata(), m_buf.rdata(), m_buf.rdata() + m_buf.size() );
    }

    inline
    ibmembuf( buffer&& buf, buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstreambuf{},
    m_buf{ std::move( buf ), pol }
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
        return get_slice( n );
    }

    virtual buffer
    getn( size_type n ) override
    {
        return get_slice( n );
    }

protected:

    inline
    ibmembuf( size_type size, buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstreambuf{},
    m_buf{ size, pol }
    {
        set_ptrs( m_buf.rdata(), m_buf.rdata(), m_buf.rdata() + m_buf.size() );
    }

    buffer      m_buf;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IBMEMBUF_H