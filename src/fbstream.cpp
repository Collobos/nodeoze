#include <nodeoze/bstream/ifbstream.h>
#include <nodeoze/bstream/ofbstream.h>
#include <nodeoze/ntest.h>

using namespace nodeoze;
using bstream::ofbstream;

TEST_CASE( "nodeoze/smoke/bstream/fbstream/write_read" )
{
    bstream::ofbstream os( "fbstream_test_file", ofbstream::open_mode::truncate );
    buffer outbuf{ "abcdefghijklmnop" };
    os.putn( outbuf );
    os.close();

    bstream::ifbstream is( "fbstream_test_file" );
    auto fsize = is.size();
    buffer inbuf = is.getn( fsize );
    is.close();
    CHECK( outbuf == inbuf );
}

TEST_CASE( "nodeoze/smoke/bstream/fbstream/write_read_ate" )
{
    {
        bstream::ofbstream os( "fbstream_test_file", ofbstream::open_mode::truncate );
        buffer outbuf{ "abcdefghijklmnop" };
        os.putn( outbuf );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.size();
        buffer inbuf = is.getn( fsize );
        is.close();
        buffer expected{ "abcdefghijklmnop" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", ofbstream::open_mode::at_end );
        auto pos = os.position();
        CHECK( pos == 16 );
        auto fsize = os.size();
        CHECK( fsize == 16 );
        buffer outbuf{ "qrstuvwxyz" };
        os.putn( outbuf );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.size();
        CHECK( fsize == 26 );
        buffer inbuf = is.getn( fsize );
        is.close();
        buffer expected{ "abcdefghijklmnopqrstuvwxyz" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.size();
        CHECK( fsize == 26 );
        is.position(16);
        buffer inbuf = is.getn( 10 );
        bool caught_exception = false;
        try
        {
            is.get();
            CHECK( false ); // should never get here
        }
        catch ( std::system_error const& e )
        {
            caught_exception = true;
        }
        CHECK( caught_exception );
        is.close();
        buffer expected{ "qrstuvwxyz" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", ofbstream::open_mode::append );
        auto pos = os.position();
        CHECK( pos == 26);
        auto fsize = os.size();
        std::cout << "size before append is " << fsize << std::endl; std::cout.flush();
        buffer outbuf{ "0123456789" };
        os.putn( outbuf );
        auto zpos = os.position( 0 );
        std::cout << "zpos is " << zpos << std::endl; std::cout.flush();
        os.putn( outbuf );
        zpos = os.position();
        std::cout << "zpos is " << zpos << std::endl; std::cout.flush();
        os.close();
    }
    {
        bstream::ofbstream os( "fbstream_test_file", ofbstream::open_mode::at_begin );
        auto pos = os.position();
        CHECK( pos == 0 );
        auto fsize = os.size();
        CHECK( fsize == 46 );
        buffer outbuf{ "0123456789" };
        os.putn( outbuf );
        auto zpos = os.position();
        CHECK( zpos == 10 );
        std::cout << "zpos is " << zpos << std::endl; std::cout.flush();
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.size();
        CHECK( fsize == 46 );
        buffer inbuf = is.getn( fsize );
        is.close();
        buffer expected{ "0123456789klmnopqrstuvwxyz01234567890123456789" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", ofbstream::open_mode::truncate );
        buffer outbuf{ "abcdefghijklmnop" };
        os.putn( outbuf );
        os.position( 36 );
        buffer outbuf2{ "0123456789" };
        os.putn( outbuf2 );
        auto zpos = os.position();
        CHECK( zpos == 46 );
        os.close();
    }
    
}