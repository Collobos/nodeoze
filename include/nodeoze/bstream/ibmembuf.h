#ifndef NODEOZE_BSTREAM_IBMEMBUF_H
#define NODEOZE_BSTREAM_IBMEMBUF_H

#include <nodeoze/bstream/ibstreambuf.h>

namespace nodeoze 
{
namespace bstream 
{

class ibmembuf : public ibstreambuf
{
public:

    using ibstreambuf::getn;

    ibmembuf( buffer const& buf, buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstreambuf{},
    m_buf{ buf, pol }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
    }

    ibmembuf( buffer&& buf, buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstreambuf{},
    m_buf{ std::move( buf ), pol }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
    }

    buffer
    get_buffer()
    {
        return m_buf;
    }

	buffer&
	get_buffer_ref()
	{
		return m_buf;
	}

    buffer
    get_slice( size_type n );

    virtual buffer
    getn( size_type n, std::error_code& err ) override;

    virtual buffer
    getn( size_type n ) override;

protected:

    ibmembuf( size_type size, buffer::policy pol = buffer::policy::copy_on_write )
    :
    ibstreambuf{},
    m_buf{ size, pol }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
    }

    buffer      m_buf;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_IBMEMBUF_H