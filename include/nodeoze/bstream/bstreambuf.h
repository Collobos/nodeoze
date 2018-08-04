#ifndef NODEOZE_BSTREAM_BSTREAMBUF_H
#define NODEOZE_BSTREAM_BSTREAMBUF_H

#include <system_error>
#include <nodeoze/buffer.h>
#include <cstdint>
#include <assert.h>
#include <cstdlib>
#include <algorithm>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/types.h>


#ifndef NODEOZE_BSTREAM_DEFAULT_OBMEMBUF_SIZE
#define NODEOZE_BSTREAM_DEFAULT_OBMEMBUF_SIZE  16384UL
#endif

#ifndef NODEOZE_BSTREAM_DEFAULT_OBFILEBUF_SIZE
#define NODEOZE_BSTREAM_DEFAULT_OBFILEBUF_SIZE  16384UL
#endif

#ifndef NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE
#define NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE  16384UL
#endif

namespace nodeoze 
{
namespace bstream 
{

#ifndef DOCTEST_CONFIG_DISABLE
namespace detail
{
	class obs_test_probe;
    class ibs_test_probe;
}
#endif

/** Base buffer class for binary output streams
 * 
 * This class is designed to be the base class for all binary output stream buffer classes.
 * A binary output stream buffer is associated with a binary output stream object. The stream object
 * delegates write operations to the stream buffer. The stream buffer acts as an intermediary between
 * the stream object and the controlled output sequence.
 * 
 * The controlled output sequence is an abstract sequence of bytes of arbitrary length. The
 * stream buffer maintains an internal byte sequence in memory. The internal byte sequence
 * may hold the output sequence in its entirety, or it may hold a sub-sequence. The buffer
 * maintains four values that internal byte sequence and its relationship to 
 * the output sequence. There are two basic types of values to be considered: positions
 * and pointers. Positions are integral values that indicate an offset relative to the beginning
 * of the output sequence. Pointers are addresses of bytes in the internal byte sequence. 
 * 
 * pbase
 * pbase is a pointer the the first byte in the buffer's internal sequence.
 * 
 * pnext
 * pnext is a pointer the the next available (unwritten) byte in the internal sequence.
 * 
 * pend
 * pend is a pointer to the end of the internal sequence; specifically, it points the 
 * byte following the last useable byte in the internal sequence.
 * 
 * pbase_offset
 * pbase_offset is the position (that is, the offset from the beginning of the output sequence)
 * of the first byte in the internal sequence (at pbase).
 * 
 * The following invariants hold true:
 * pnext >= pbase
 * pend >= pnext
 * pbase_offset >= 0
 * 
 * Also, any pointer value p in the range (pbase <= p < pend) has a corresponding position, computed as 
 * pbase_offset + ( p - pbase ). Certain of these positions are explicitly represented in the stream
 * buffer interface:
 * 
 * ppos
 * ppos is the position corresponding to the pnext pointer, sometimes referred to as the current position
 * of the stream buffer (i.e., the position at which the next write will take place).
 * 
 * Other important operations:
 * 
 * flush
 * The flush operation synchronizes the internal sequence with the output sequence. Derived implementations
 * must provide an implementation of the virtual member function really_flush() that
 * effects synchronization for the specific output sequence type. It is important to note
 * that flush may (but does not necessarily) change the internal sequence pointers or positions.
 * 
 * dirty
 * dirty is a boolean value associated with the stream buffer. dirty is true if the buffer's internal
 * sequence contains values that have not been synchronized (flushed) with the output sequence.
 * After a flush operation, dirty is always false.
 * 
 * dirty_start
 * if dirty is true, dirty_start is a pointer indicating the first byte in the internal sequence
 * that is not synchronized with the output sequence. Unsynchronized byte are always contiguous, 
 * including bytes from dirty_start to pnext - 1. 
 * 
 * Stream buffers support the ability to set the current position explicitly, with a seek operation,
 * A derived implementation may choose not to support seeking, in which seek operations will result
 * in an error condition. The ability to seek forces additional complexity on the stream buffer's
 * interface and behavior.
 * 
 * high_water_mark
 * high_water_mark is a positional value, indicating the largest position in the output 
 * sequence that contains a valid value (essentially, the size of the output sequence).
 * If the current position (ppos) is equal to the high_water_mark, a write operation
 * will cause the high_water_mark value to increase.
 * 
 * touch
 * Some derived implementations (specifically, output file buffers) may permit the 
 * existence of "holes"--regions in the output sequence to which values have not been explicitly
 * written. When a seek operation moves the current position past the high_water_mark position, 
 * and a write occurs, the region of the output sequnce between the previous high_water_mark 
 * and the position of the seek/write operations potentially constitutes such a hole. The 
 * In general, the abstract behavior of any output sequence type should be to fill holes with
 * zero-valued bytes. However, a particular implementation may choose to either write zero-valued
 * bytes to the region, or mark the region as a hole, in which case any future attempt to
 * obtain values from the region of the output sequence will return zero-valued bytes.
 * 
 * To permit implementations freedom of choice in the matter, stream buffers support a touch
 * operation. When a seek operation sets the current position beyond the high_water_mark, the
 * state of the buffer (except for the current position) does not change until a write occurs
 * at the new position. Immediately prior to the actual write, the stream buffer invokes touch
 * (which, in turn, invokes the virtual member function really_touch), allowing the derived
 * implementation to create and/or fill the resulting hole. In essence, the touch operation 
 * announces to the derived implementation that a write operation at the current position
 * is imminent. touch forces a synchronization with the output sequence.
 * 
 * In order to prevent unnecessary invocations of touch, the stream buffer maintains a positional
 * value last_touched. Whenever a synchronization operation (touch or flush) occurs, last_touched 
 * is set to the current position after the synchronization. 
 * 
 * 
 */

class obstreambuf
{
public:

#ifndef DOCTEST_CONFIG_DISABLE
public:
	friend class detail::obs_test_probe;
#endif

    static constexpr buffer::size_type default_buffer_size = NODEOZE_BSTREAM_DEFAULT_OBMEMBUF_SIZE;

    inline
    obstreambuf( byte_type* data, size_type size )
    :
    m_pbase_offset{ 0 },
    m_high_watermark{ 0 },
    m_last_touched{ 0 },
    m_pbase{ data },
    m_pnext{ data },
    m_pend{ data + size },
    m_dirty_start{ data },
    m_dirty{ false },
    m_writable{ true }
    {
    }

    obstreambuf( obstreambuf const& ) = delete;
    obstreambuf& operator=( obstreambuf&& ) = delete;
    obstreambuf& operator=( obstreambuf const& ) = delete;
    
    virtual ~obstreambuf() {}

protected:

    inline
    obstreambuf()
    :
    m_pbase_offset{ 0 },
    m_high_watermark{ 0 },
    m_last_touched{ 0 },
    m_pbase{ nullptr },
    m_pnext{ nullptr },
    m_pend{ nullptr },
    m_dirty_start{ nullptr },
    m_dirty{ false },
    m_writable{ true }
    {}

    inline
    obstreambuf( obstreambuf&& rhs )
    : 
    m_pbase_offset{ rhs.m_pbase_offset },
    m_high_watermark{ rhs.m_high_watermark },
    m_last_touched{ rhs.m_last_touched },
    m_pbase{ rhs.m_pbase },
    m_pnext{ rhs.m_pnext },
    m_pend{ rhs.m_pend },
    m_dirty_start{ rhs.m_dirty_start },
    m_dirty{ rhs.m_dirty },
    m_writable{ rhs.m_writable }
    {}


public:

    inline void
    flush( std::error_code& err )
    {
        clear_error( err );
        if ( dirty() )
        {
            assert( writable() );
            really_flush( err );
            if ( ! err )
            {
                set_high_watermark();
                last_touched( ppos() );
                dirty( false );
            }
        }
    }

    inline void
    flush()
    {
        if ( dirty() )
        {
            assert( writable() );
            std::error_code err;
            really_flush( err );
            if ( err )
            {
                throw std::system_error{ err };
            }
            set_high_watermark();
            last_touched( ppos() );
            dirty( false );
        }
    }

    inline void
    put( byte_type byte, std::error_code& err )
    {
        make_writable();
        if ( ! dirty() )
        {
            touch( err );
            if ( err ) goto exit;
        }
        if ( pnext() >= pend() )
        {
            assert( pnext() == pend() );
            overflow( 1, err );
            if ( err ) goto exit;
            assert( ! dirty() );
        }
        if ( ! dirty() )
        {
            dirty_start( pnext() );
        }
        *m_pnext++ = byte;
        dirty( true );

    exit:
        return;
    }

    inline void
    put( byte_type byte )
    {
        make_writable();
        if ( ! dirty() )
        {
            touch();
        }
        if ( pnext() >= pend() )
        {
            assert( pnext() == pend() );
            overflow( 1 );
            assert( ! dirty() );
        }
        if ( ! dirty() )
        {
            dirty_start( pnext() );
        }
        *m_pnext++ = byte;
        dirty( true );
    }

    void
    putn( const byte_type* src, size_type n, std::error_code& err );

    inline void
    putn( const byte_type* src, size_type n )
    {
        std::error_code err;
        putn( src, n, err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }

    void
    filln( const byte_type fill_byte, size_type n, std::error_code& err );

    inline void
    filln( const byte_type fill_byte, size_type n )
    {
        std::error_code err;
        filln( fill_byte, n, err );
        if ( err )
        {
            throw std::system_error{ err };
        }
    }


    inline position_type
    seek( position_type pos, std::error_code& err )
    {
        return seek( bstream::seek_anchor::begin, pos, err );
    }

    inline position_type
    seek( position_type pos )
    {
        return seek( bstream::seek_anchor::begin, pos );
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
    tell( std::error_code& err )
    {
        return really_tell( seek_anchor::current, err );
    }

    inline position_type
    tell( seek_anchor where, std::error_code& err )
    {
        return really_tell( where, err );
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

    inline void
    touch( std::error_code& err )
    {
        assert( ! dirty() );
        assert( writable() );
        auto pos = ppos();
       
        if ( last_touched() != pos )
        {
            really_touch( err );
            if ( err ) goto exit;
        }

        assert( ppos() == pos );
        assert( pos == last_touched() );
        assert( ! dirty() );
    exit:
        return;
    }

    inline void
    touch()
    {
        assert( ! dirty() );
        assert( writable() );
        auto pos = ppos();
       
        if ( last_touched() != pos )
        {
            std::error_code err;
            really_touch( err );
            if ( err )
            {
                throw std::system_error{ err };
            }
        }

        assert( ppos() == pos );
        assert( pos == last_touched() );
        assert( ! dirty() );
    }

    inline void
    set_ptrs( byte_type * base, byte_type * next, byte_type * end )
    {
        m_pbase = base;
        m_pnext = next;
        m_pend = end;
    }

    inline void
    pbase_offset( position_type offset )
    {
        m_pbase_offset = offset;
    }

    inline position_type
    pbase_offset() const
    {
        return m_pbase_offset;
    }

    inline void
    overflow( size_type requested, std::error_code& err )
    {
        flush( err );
        if ( err ) goto exit;

        really_overflow( requested, err );
        if ( err ) goto exit;

        assert( pend() > pnext() );

    exit:
        return;
    }

    inline void
    overflow( size_type requested )
    {
        flush();
        std::error_code err;

        really_overflow( requested, err );
        if ( err )
        {
            throw std::system_error{ err };
        }

        assert( pend() > pnext() );
    }

    inline position_type
    get_high_watermark() const noexcept
    {
        return m_high_watermark;
    }

    inline position_type
    set_high_watermark() 
    {
        if ( dirty() && ( ppos() > m_high_watermark ) )
        {
            m_high_watermark = ppos();
        }
        return m_high_watermark;
    }

    inline void
    force_high_watermark( position_type hwm )
    {
        m_high_watermark = hwm;
    }

    inline void
    reset_high_water_mark()
    {
        m_high_watermark = 0;
    }

    inline position_type
    ppos() const noexcept
    {
        return pbase_offset() + ( pnext() - pbase() );
    }

    inline bool
    dirty() const noexcept
    {
        return m_dirty;
    }

    inline void
    dirty( bool value ) noexcept
    {
        m_dirty = value;
    }

    inline position_type
    last_touched() const noexcept
    {
        return m_last_touched;
    }

    inline void
    last_touched( position_type touched )
    {
        m_last_touched = touched;
    }

    inline byte_type*
    pbase() const noexcept
    {
        return m_pbase;
    }

    inline void
    pbase( byte_type* p )
    {
        m_pbase = p;
    }

    inline byte_type*
    pnext() const noexcept
    {
        return m_pnext;
    }

    inline void
    pnext( byte_type* p )
    {
        m_pnext = p;
    }

    inline byte_type*
    pend() const noexcept
    {
        return m_pend;
    }

    inline void
    pend( byte_type* p )
    {
        m_pend = p;
    }

    inline byte_type*
    dirty_start() const noexcept
    {
        return m_dirty_start;
    }

    inline void
    dirty_start( byte_type* start ) noexcept
    {
        m_dirty_start = start;
    }

    inline bool
    writable() const noexcept
    {
        return m_writable;
    }

    inline void
    writable( bool value )
    {
        m_writable = value;
    }

    inline void
    make_writable()
    {
        if ( ! writable() )
        {
            really_make_writable();
            writable( true );
        }
    }

protected: // to be overridden by derived classes

    virtual bool
    really_make_writable();

    virtual void
    really_touch( std::error_code& err );

    virtual position_type
    really_seek( seek_anchor where, offset_type offset, std::error_code& err );

    virtual position_type
    really_tell( seek_anchor where, std::error_code& err );

    virtual void
    really_overflow( size_type, std::error_code& err );
    
    virtual void
    really_flush( std::error_code& err );


private:

    position_type       m_pbase_offset;
    position_type       m_high_watermark;
    position_type       m_last_touched;
    byte_type*          m_pbase;
    byte_type*          m_pnext;
    byte_type*          m_pend;
    byte_type*          m_dirty_start;
    bool                m_dirty;
    bool                m_writable;

};

class obmembuf : public obstreambuf
{
public:

    inline
    obmembuf( size_type size = obstreambuf::default_buffer_size, 
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

class obfilebuf : public obstreambuf
{
public:

/*     enum class open_mode
    {
        truncate,
        append,
        at_end,
        at_begin,
    };

 */

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

    inline buffer
    getn( size_type n, std::error_code& err )
    {
        buffer buf{ n };
        auto got = getn( buf.data(), n, err );
        if ( got < n )
        {
            buf.size( got );
        }
        return buf;
    }

    inline buffer
    getn( size_type n )
    {
        buffer buf{ n };
        auto got = getn( buf.data(), n );
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

class ibmembuf : public ibstreambuf
{
public:

    inline
    ibmembuf( buffer const& buf )
    :
    ibstreambuf{},
    m_buf{ buf }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
    }

    inline
    ibmembuf( buffer&& buf )
    :
    ibstreambuf{},
    m_buf{ std::move( buf ) }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
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

protected:

    inline
    ibmembuf( size_type size )
    :
    ibstreambuf{},
    m_buf{ size, buffer::policy::exclusive }
    {
        set_ptrs( m_buf.data(), m_buf.data(), m_buf.data() + m_buf.size() );
    }

    buffer      m_buf;
};

class ibfilebuf : public ibmembuf
{
public:

    inline
    ibfilebuf( size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE )
    :
    ibmembuf{ buffer_size },
    m_filename{},
    m_is_open{ false },
    m_flags{ O_RDONLY },
    m_fd{ -1 }
    {}

    inline
    ibfilebuf( std::string const& filename, std::error_code& err, int flag_overrides = 0, size_type buffer_size = NODEOZE_BSTREAM_DEFAULT_IBFILEBUF_SIZE )
    :
    ibmembuf{ buffer_size },
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
    ibmembuf{ buffer_size },
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

    std::string         m_filename;
    bool                m_is_open;
    int                 m_flags;
    int                 m_fd;
    
};

}
}

#endif // NODEOZE_BSTREAM_BSTREAMBUF_H