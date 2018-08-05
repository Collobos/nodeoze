#ifndef NODEOZE_BSTREAM_OBMEMBUF_H
#define NODEOZE_BSTREAM_OBMEMBUF_H

#include <system_error>
#include <cstdint>
#include <assert.h>
#include <cstdlib>
#include <algorithm>
#include <nodeoze/bstream/obstreambuf.h>
#include <nodeoze/buffer.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/types.h>


#ifndef NODEOZE_BSTREAM_DEFAULT_OBMEMBUF_SIZE
#define NODEOZE_BSTREAM_DEFAULT_OBMEMBUF_SIZE  16384UL
#endif

namespace nodeoze 
{
namespace bstream 
{

class obmembuf : public obstreambuf
{
public:

    inline
    obmembuf( size_type size = NODEOZE_BSTREAM_DEFAULT_OBMEMBUF_SIZE,
                 buffer::policy pol = buffer::policy::copy_on_write )
    :
    obstreambuf{},
    m_buf{ size, pol }
    {
        reset_ptrs();
    }

    inline
    obmembuf( buffer&& buf )
    :
    obstreambuf{},
    m_buf{ std::move( buf ) }
    {
        reset_ptrs();
    }

    obmembuf( obmembuf&& ) = delete;
    obmembuf( obmembuf const& ) = delete;
    obmembuf& operator=( obmembuf&& ) = delete;
    obmembuf& operator=( obmembuf const& ) = delete;
    
	inline obmembuf& 
	clear() noexcept
	{
		reset_ptrs();
		reset_high_water_mark();
        last_touched( 0UL );
        dirty( false );
		return *this;
	}

	inline buffer 
	get_buffer( bool force_copy = false )
	{
		if ( ! force_copy && m_buf.is_copy_on_write() )
		{
			writable( false );
		}
        if ( dirty() )
        {
            set_high_watermark();
        }
		return m_buf.slice( 0, get_high_watermark(), force_copy );
	}

	inline buffer&
	get_buffer_ref()
	{
		return m_buf;
	}

	inline buffer
	release_buffer()
	{
        if ( dirty() )
        {
            set_high_watermark();
        }
		m_buf.size( get_high_watermark() );
		reset_high_water_mark();
        last_touched( 0UL );
        dirty( false );
        set_ptrs( nullptr, nullptr, nullptr );
		return std::move( m_buf );
	}

protected:

    virtual bool
    really_make_writable() override;

    virtual void
    really_flush( std::error_code& err ) override;

    virtual void
    really_touch( std::error_code& err ) override;

    inline void
    resize( size_type size )
    {
        size_type cushioned_size = ( size * 3 ) / 2;
	    // force a hard lower bound to avoid non-resizing dilemma in resizing, when cushioned == size == 1
        m_buf.size( std::max( 16UL, cushioned_size ) );
    }

    virtual position_type
    really_seek( seek_anchor where, offset_type offset, std::error_code& err ) override;

    // base class implementation of really_tell needs no override here

    virtual void
    really_overflow( size_type n, std::error_code& err ) override;

    inline void
    reset_ptrs()
    {
        auto base = m_buf.data();
        set_ptrs( base, base, base + m_buf.size() );
    }

    buffer          m_buf;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_OBMEMBUF_H