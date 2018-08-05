#include <chrono>
#include <thread>
#include <nodeoze/bstream/ibstreambuf.h>
#include <nodeoze/bstream/obstreambuf.h>
#include <nodeoze/bstream/ibmembuf.h>
#include <nodeoze/bstream/obmembuf.h>
#include <nodeoze/bstream/ibfilebuf.h>
#include <nodeoze/bstream/obfilebuf.h>
#include <nodeoze/test.h>

using namespace nodeoze;
using namespace bstream;

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
    obuf.putn( tbuf0.data(), tbuf0.size(), err );
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
        obuf.putn( tbuf1.data(), tbuf1.size(), err );
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
        obuf.putn( tbuf2.data(), tbuf2.size(), err );
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
    bstream::ibstreambuf ibuf{ buf.data(), buf.size() };

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
    
    obf.putn( buf.data(), 32, err );
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

    obf.putn( buf.data() + 32, 48, err );
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
    std::this_thread::sleep_for( std::chrono::seconds( 10 ) );

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