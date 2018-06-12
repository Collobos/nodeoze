#include <chrono>
#include <thread>
#include <nodeoze/bstream/bstreambuf.h>
#include <nodeoze/test.h>

using namespace nodeoze;
using namespace bstream;

void
bstream::obstreambuf::putn( const byte_type* src, size_type n, std::error_code& err )
{
    make_writable();
    if ( ! dirty() )
    {
        touch( err );
        if ( err ) goto exit;
        dirty_start( pnext() );
    }
    if ( n <= static_cast< size_type >( pend() - pnext() ) ) // optimize for common case ( no overflow )
    {
        ::memcpy( pnext(), src, n );
        m_pnext += n;
        dirty( true );
    }
    else 
    {
        overflow( n, err );
        if ( err ) goto exit;
        if ( n <= static_cast< size_type >( pend() - pnext() ) ) // try it in one go
        {
            if ( ! dirty() )
            {
                dirty_start( pnext() );
            }
            ::memcpy( pnext(), src, n );
            m_pnext += n;
            dirty( true );
        }
        else // put it in chunks
        {
            auto remaining = n;
            auto p = src;
            while ( remaining > 0 )
            {
                if ( pnext() >= pend() ) // should be false on first iteration
                {
                    assert( pnext() == pend() );
                    overflow( remaining, err );
                    if ( err ) goto exit;
                }

                assert( pend() - pnext() > 0 );

                size_type chunk_size = std::min( remaining, static_cast< size_type >( pend() - pnext() ) );
                if ( ! dirty() )
                {
                    dirty_start( pnext() );
                }
                ::memcpy( pnext(), p, chunk_size );
                remaining -= chunk_size;
                p += chunk_size;
                m_pnext += chunk_size;
                dirty( true );
            }
        }
    }
    
exit:
    return;
}

void
bstream::obstreambuf::filln( const byte_type fill_byte, size_type n, std::error_code& err )
{
    make_writable();
    if ( ! dirty() )
    {
        touch( err );
        if ( err ) goto exit;
        dirty_start( pnext() );
    }
    if ( n <= static_cast< size_type >( pend() - pnext() ) ) // optimize for common case ( no overflow )
    {
        ::memset( pnext(), fill_byte, n );
        m_pnext += n;
        dirty( true );
    }
    else 
    {
        overflow( n, err );
        if ( err ) goto exit;
        if ( n <= static_cast< size_type >( pend() - pnext() ) ) // try it in one go
        {
            if ( ! dirty() )
            {
                dirty_start( pnext() );
            }
            ::memset( pnext(), fill_byte, n );
            m_pnext += n;
            dirty( true );
        }
        else // put it in chunks
        {
            auto remaining = n;
            while ( remaining > 0 )
            {
                if ( pnext() >= pend() ) // should be false on first iteration
                {
                    assert( pnext() == pend() );
                    overflow( remaining, err );
                    if ( err ) goto exit;
                }

                assert( pend() - pnext() > 0 );

                size_type chunk_size = std::min( remaining, static_cast< size_type >( pend() - pnext() ) );
                if ( ! dirty() )
                {
                    dirty_start( pnext() );
                }
                ::memset( pnext(), fill_byte, chunk_size );
                remaining -= chunk_size;
                m_pnext += chunk_size;
                dirty( true );
            }
        }
    }
    
exit:
    return;
}

bool
bstream::obstreambuf::really_make_writable()
{
    return true;
}

void
bstream::obstreambuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto hwm = get_high_watermark();
    auto pos = ppos();
    assert( hwm < pend() - pbase() && pos <= pend() - pbase() );
    assert( last_touched() != pos );

    if ( hwm < pos )
    {
        pnext( pbase() + ( hwm - pbase_offset() ) );

        filln( pos - hwm, 0, err );
        if ( err ) goto exit;

        flush( err );
        if ( err ) goto exit;

        assert( ppos() == pos );
        assert( get_high_watermark() == pos );
        assert( last_touched() == pos );
    }
    else
    {
        last_touched( pos );
        assert( hwm >= pos );
    }

exit:
    return;
}

position_type
bstream::obstreambuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    flush( err );
    if ( err ) goto exit;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            // high watermark will be current because of flush() above
            auto end_pos = get_high_watermark();
            result = end_pos + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 || result > pend() - pbase() )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
        goto exit;
    }

    pnext( pbase() + ( result - pbase_offset() ) );

exit:
    return result;
}

position_type
bstream::obstreambuf::really_tell( seek_anchor where, std::error_code& err )
{
    clear_error( err );

    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos();
        }
        break;
        
        case seek_anchor::end:
        {
            if ( dirty() )
            {
                set_high_watermark(); // Note: hwm may advance ahead of last_flushed and dirty_start here. This should be ok.
            }
            result = get_high_watermark();
        }
        break;

        case seek_anchor::begin:
        {
            result = 0;
        }
        break;
    }

    return result;
}

void
bstream::obstreambuf::really_overflow( size_type, std::error_code& err )
{
    err = make_error_code( std::errc::no_buffer_space );
}

void
bstream::obstreambuf::really_flush( std::error_code& err )
{
    clear_error( err );
    assert( dirty() && pnext() > dirty_start() );
}

/* 
bool
bstream::obmembuf::really_make_writable()
{
    bool result = m_buf.is_writable();
    if ( ! result )
    {
        m_buf.make_unique();
    }
    return result;
}

void
bstream::obmembuf::really_flush( std::error_code& err )
{
    clear_error( err );
    assert( dirty() && pnext() > dirty_start() );
}

void
bstream::obmembuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto hwm = get_high_watermark();
    auto pos = ppos();
    assert( hwm < pend() - pbase() && pos <= pend() - pbase() );
    assert( last_touched() != pos );

    if ( hwm < pos )
    {
        dirty_start( pbase() + hwm );
        size_type n = static_cast< size_type >( pos - hwm );
        ::memset( dirty_start(), 0, n );
        dirty( true );

        flush( err );
        if ( err ) goto exit;

        assert( ppos() == pos );
        assert( get_high_watermark() == pos );
        assert( last_touched() == pos );
    }
    else
    {
        last_touched( pos );
        assert( hwm >= pos );
    }
    
exit:
    return;
}
 */
bool
bstream::obmembuf::really_make_writable()
{
    bool result = m_buf.is_writable();
    if ( ! result )
    {
        m_buf.make_unique();
    }
    return result;
}

void
bstream::obmembuf::really_flush( std::error_code& err )
{
    clear_error( err );
    assert( dirty() && pnext() > dirty_start() );
}

void
bstream::obmembuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto hwm = get_high_watermark();
    auto pos = ppos();
    assert( hwm < pend() - pbase() && pos <= pend() - pbase() );
    assert( last_touched() != pos );

    if ( hwm < pos )
    {
        dirty_start( pbase() + hwm );
        size_type n = static_cast< size_type >( pos - hwm );
        ::memset( dirty_start(), 0, n );
        dirty( true );

        flush( err );
        if ( err ) goto exit;

        assert( ppos() == pos );
        assert( get_high_watermark() == pos );
        assert( last_touched() == pos );
    }
    else
    {
        last_touched( pos );
        assert( hwm >= pos );
    }
    
exit:
    return;
}

position_type
bstream::obmembuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    flush( err );
    if ( err ) goto exit;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            auto end_pos = get_high_watermark(); // high watermark is current from flush() above
            result = end_pos + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
    }
    else if ( result >= static_cast< position_type >( pend() - pbase() ) )
    {
        resize( result );
        auto new_base = m_buf.rdata();
        set_ptrs( new_base, new_base + result, new_base + m_buf.size() );
    }
    else
    {
        pnext( pbase() + result );
    }

exit:
    return result;
}

void
bstream::obmembuf::really_overflow( size_type n, std::error_code& err )
{
    clear_error( err );
    assert( std::less_equal< byte_type * >()( pnext(), pend() ) );
    assert( ( pnext() - pbase() ) + n > m_buf.size() );
    auto pos = ppos();
    size_type required = ( pnext() - pbase() ) + n;
    resize( required );
    auto new_base = m_buf.rdata();
    set_ptrs( new_base, new_base + pos, new_base + m_buf.size() );
}

bool
bstream::obfilebuf::really_make_writable()
{
    return true;
}

void
bstream::obfilebuf::really_flush( std::error_code& err )
{
    clear_error( err );
    auto pos = ppos();
    assert( dirty() && pnext() > dirty_start() );
    assert( dirty_start() == pbase() );
    if ( last_touched() != pbase_offset() )
    {
        auto seek_result = ::lseek( m_fd, pbase_offset(), SEEK_SET );
        if ( seek_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        assert( seek_result == pbase_offset() );
    }

    {
        size_type n = static_cast< size_type >( pnext() - pbase() );
        auto write_result = ::write( m_fd, pbase(), n );
        if ( write_result < 0 )
        {
            err = std::error_code{ errno, std::generic_category() };
            goto exit;
        }
        assert( static_cast< size_type >( write_result ) == n );
        pbase_offset( pos );
        pnext( pbase() );
    }
exit:
    return;
}

void
bstream::obfilebuf::really_touch( std::error_code& err )
{
    clear_error( err );
    auto pos = ppos();
    assert( pbase_offset() == pos && pnext() == pbase() );
    assert( last_touched() != pos );

    auto result = ::lseek( m_fd, pos, SEEK_SET );
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        goto exit;
    }
    last_touched( pos );
    
exit:
    return;
}

position_type
bstream::obfilebuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    flush( err );
    if ( err ) goto exit;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ppos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            auto end_pos = get_high_watermark();
            result = end_pos + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 )
    {
        err = make_error_code( std::errc::invalid_argument );
        result = invalid_position;
        goto exit;
    }

    pbase_offset( result );
    pnext( pbase() );

exit:
    return result;
}

void
bstream::obfilebuf::really_overflow( size_type, std::error_code& err )
{
    clear_error( err );
    assert( pbase_offset() == ppos() && pnext() == pbase() );
}

position_type
bstream::ibstreambuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos() + offset;
        }
        break;

        case seek_anchor::end:
        {
            result = ( gend() - gbase() ) + offset;
        }
        break;

        case seek_anchor::begin:
        {
            result = offset;
        }
        break;
    }

    if ( result < 0 || result > ( gend() - gbase() ) )
    {
        err = make_error_code( std::errc::invalid_seek );
        result = invalid_position;
        goto exit;
    }

    gnext( gbase() + result );

exit:
    return result;

}

position_type
bstream::ibstreambuf::really_tell( seek_anchor where, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos();
        }
        break;
        
        case seek_anchor::end:
        {
            result = static_cast< position_type >( gend() - gbase() );
        }
        break;

        case seek_anchor::begin:
        {
            result = 0;
        }
        break;
    }

    return result;
}

size_type
bstream::ibstreambuf::really_underflow( std::error_code& err )
{
    clear_error( err );
    return 0UL;
}

position_type
bstream::ibfilebuf::really_seek( seek_anchor where, offset_type offset, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = ::lseek( m_fd, offset, SEEK_CUR );
        }
        break;

        case seek_anchor::end:
        {
            result = ::lseek( m_fd, offset, SEEK_END );
        }
        break;

        case seek_anchor::begin:
        {
            result = ::lseek( m_fd, offset, SEEK_SET );
        }
        break;
    }
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        result = invalid_position;
        goto exit;
    }
    gbase_offset( result );
    reset_ptrs();
exit:
    return result;
}

position_type
bstream::ibfilebuf::really_tell( seek_anchor where, std::error_code& err )
{
    clear_error( err );
    position_type result = invalid_position;

    switch ( where )
    {
        case seek_anchor::current:
        {
            result = gpos();
        }
        break;

        case seek_anchor::end:
        {
            auto save = gbase_offset();
            result = ::lseek( m_fd, 0, SEEK_END );
            if ( result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                result = invalid_position;
                goto exit;
            }
            auto restore_result = ::lseek( m_fd, save, SEEK_SET );
            if ( restore_result < 0 )
            {
                err = std::error_code{ errno, std::generic_category() };
                result = invalid_position;
                goto exit;
            }
            assert( restore_result == save );
        }
        break;

        case seek_anchor::begin:
        {
            result = 0;
        }
        break;
    }
    if ( result < 0 )
    {
        err = std::error_code{ errno, std::generic_category() };
        result = invalid_position;
        goto exit;
    }
exit:
    return result;
}

size_type
bstream::ibfilebuf::really_underflow( std::error_code& err )
{
    assert( gnext() == gend() );
    gbase_offset( gpos() );
    gnext( gbase() );
    size_type available = load_buffer( err );
    if ( err ) 
    {
        available = 0;
    }
    return available;
}

#ifndef DOCTEST_CONFIG_DISABLE

class bstream::detail::obs_test_probe
{
public:
	obs_test_probe( bstream::obstreambuf& target )
	: 
	m_target{ target }
	{}

    position_type base_offset()
    {
        return m_target.m_pbase_offset;
    }

    position_type hwm()
    {
        return m_target.m_high_watermark;
    }

    bool dirty()
    {
        return m_target.m_dirty;
    }

    bool is_writable()
    {
        return m_target.m_writable;
    }

    void* base()
    {
        return m_target.m_pbase;
    }

    void* next()
    {
        return m_target.m_pnext;
    }

    void* end()
    {
        return m_target.m_pend;
    }

private:
	bstream::obstreambuf& m_target;

};

class bstream::detail::ibs_test_probe
{
public:
	ibs_test_probe( bstream::ibstreambuf& target )
	: 
	m_target{ target }
	{}

    position_type base_offset()
    {
        return m_target.m_gbase_offset;
    }    

    const void* base()
    {
        return m_target.m_gbase;
    }

    const void* next()
    {
        return m_target.m_gnext;
    }

    const void* end()
    {
        return m_target.m_gend;
    }

private:
	bstream::ibstreambuf& m_target;

};


#endif

TEST_CASE( "nodeoze/smoke/obstreambuf/basic" )
{
    // std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

    // buffer tbuf{ 16, buffer::policy::no_copy_on_write };

    bstream::obmembuf obuf{ 16, buffer::policy::no_copy_on_write };

    bstream::detail::obs_test_probe probe{ obuf };

    std::error_code err;

    buffer tbuf0{ "zooble" };
    buffer tbuf1{ "gorn" };
    buffer tbuf2{ "black" };
    obuf.putn( tbuf0.const_data(), tbuf0.size(), err );
    CHECK( ! err );
    CHECK( obuf.tell( bstream::seek_anchor::current, err ) == 6 );
    CHECK( ! err );

    {
        buffer view = obuf.get_buffer();
        auto end_pos = probe.hwm();
        CHECK( end_pos == view.size() );
//        view.dump( std::cout );
    }

    {
        obuf.seek( 8, err );
        CHECK( ! err );
        obuf.putn( tbuf1.const_data(), tbuf1.size(), err );
        CHECK( !err );
        CHECK( obuf.tell( bstream::seek_anchor::current, err ) == 12 );
        CHECK( ! err );
    }
    {
        buffer view = obuf.get_buffer();
        auto end_pos = probe.hwm();
        CHECK( end_pos == view.size() );
//        view.dump( std::cout );
    }

    {
        obuf.putn( tbuf2.const_data(), tbuf2.size(), err );
        CHECK( ! err );
    }

    {
        buffer view = obuf.get_buffer();;
        auto end_pos = probe.hwm();
        auto tell_end = obuf.tell( bstream::seek_anchor::end, err );
        CHECK( ! err );
        CHECK( tell_end == end_pos );
        CHECK( end_pos == view.size() );
//        view.dump( std::cout );
    }

}

TEST_CASE( "nodeoze/smoke/ibstreambuf/basic" )
{
    // std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

    buffer buf{ "0123456789ABCDEF"};
    bstream::ibstreambuf ibuf{ buf.rdata(), buf.size() };

    bstream::detail::ibs_test_probe probe{ ibuf };
    std::error_code err;

    int index = 0;
    while ( true )
    {
        auto b = ibuf.get( err );
        if ( index < 16 )
        {
            CHECK( b == buf.at( index ) );
            CHECK( ! err );
        }
        else
        {
            CHECK( err );
//            std::cout << err.message() << std::endl;
            break;
        }
        ++index;
    }

    CHECK( ibuf.seek( bstream::seek_anchor::begin, 3, err ) == 3 );
    CHECK( ! err );
    auto b = ibuf.get( err );
    CHECK( ! err );
    CHECK( b == buf[3] );
    CHECK( ibuf.seek( bstream::seek_anchor::current, 5, err  ) == 9 );
    b = ibuf.get( err );
    CHECK( ! err );
    CHECK( b == buf[9] );

    CHECK( ibuf.seek( bstream::seek_anchor::end, -1, err ) == 15 );
    b = ibuf.get( err );
    CHECK( ! err );
    CHECK( b == buf[15]);

    b = ibuf.get( err );
    CHECK( err );
    CHECK( err == bstream::errc::read_past_end_of_stream );

    CHECK( ibuf.seek( bstream::seek_anchor::begin, 0, err ) == 0 );
    CHECK( ! err );

    auto bf = ibuf.getn( 7, err );
    CHECK( !  err );

    CHECK( bf.size() == 7 );
    CHECK( ibuf.tell( bstream::seek_anchor::current, err ) == 7 );
    CHECK( ! err );
    CHECK( bf.to_string() == std::string{"0123456"} ); 

    CHECK( ibuf.seek( bstream::seek_anchor::current, -8, err ) == bstream::invalid_position );
    CHECK( err );
    CHECK( err == std::errc::invalid_seek );

    CHECK( ibuf.seek( bstream::seek_anchor::end, -4, err ) == 12 );
    CHECK( ! err );
    
    bf = ibuf.getn( 5, err );
    CHECK( bf.size() == 4 );
    CHECK( bf.to_string() == "CDEF" );
    CHECK( ! err );
}

TEST_CASE( "nodeoze/smoke/obfilebuf/basic" )
{
    // std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

    buffer buf( 256 );
    for ( auto i = 0u; i < buf.size(); ++i )
    {
        buf.put( i, static_cast< bstream::byte_type >( i ) );
    }
    std::error_code err;
    bstream::obfilebuf obf{ "filebuftest", bstream::open_mode::truncate, err, 32 };
    CHECK( ! err );

    bstream::detail::obs_test_probe probe{ obf };
    CHECK( probe.base_offset() == 0 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 0 );
    CHECK( ! probe.dirty() );
    CHECK( probe.end() == reinterpret_cast< bstream::byte_type* >( probe.base() ) + 32 );
    
    obf.putn( buf.rdata(), 32, err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 0 );
    CHECK( probe.next() == reinterpret_cast< bstream::byte_type* >( probe.base() ) + 32 );
    CHECK( probe.hwm() == 0 );
    CHECK( probe.dirty() );

    obf.flush( err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 32 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 32 );
    CHECK( ! probe.dirty() );

    obf.close( err );
    CHECK( ! err );

    obf.open( "filebuftest", bstream::open_mode::at_end, err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 32 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 32 );
    CHECK( ! probe.dirty() );

    auto pos = obf.tell( bstream::seek_anchor::current, err );
    CHECK( ! err );
    CHECK( pos == 32 );

    pos = obf.seek( bstream::seek_anchor::current, 32, err );
    CHECK( ! err );
    CHECK( pos == 64 );
    CHECK( probe.base_offset() == 64 );
    CHECK( probe.next() == probe.base() );
    CHECK( probe.hwm() == 32 );
    CHECK( ! probe.dirty() );

    obf.putn( buf.rdata() + 32, 48, err );
    CHECK( ! err );
    CHECK( probe.base_offset() == 96 );
    CHECK( probe.next() == reinterpret_cast< bstream::byte_type* >( probe.base() ) + 16 );
    CHECK( probe.hwm() == 96 );
    CHECK( probe.dirty() );

    obf.close( err );
    CHECK( ! err );
}

TEST_CASE( "nodeoze/smoke/ibfilebuf/basic" )
{
    std::error_code err;
    bstream::ibfilebuf ibf{ "filebuftest", err, 0, 32 };
    CHECK( ! err );

    auto end_pos = ibf.tell( bstream::seek_anchor::end, err );
    CHECK( ! err );
    CHECK( end_pos == 112 );

    auto pos = ibf.tell( bstream::seek_anchor::current, err );
    CHECK( ! err );
    CHECK( pos == 0 );

    buffer buf = ibf.getn( end_pos, err );
    CHECK( ! err );
    CHECK( buf.size() == 112 );

    ibf.close( err );
    CHECK( ! err );
//    buf.dump( std::cout );
}