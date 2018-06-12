#include <nodeoze/bstream/ifbstream.h>
#include <nodeoze/bstream/ofbstream.h>
#include <nodeoze/test.h>

using namespace nodeoze;
using namespace bstream;
using bstream::ofbstream;

TEST_CASE( "nodeoze/smoke/bstream/fbstream/write_read" )
{
    bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::truncate );
    buffer outbuf{ "abcdefghijklmnop" };
    os.putn( outbuf );
    os.close();

    bstream::ifbstream is( "fbstream_test_file" );
    auto fsize = is.tell( seek_anchor::end );
    buffer inbuf = is.getn( fsize );
    is.close();
    CHECK( outbuf == inbuf );
}

TEST_CASE( "nodeoze/smoke/bstream/fbstream/write_read_ate" )
{
    {
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::truncate );
        buffer outbuf{ "abcdefghijklmnop" };
        os.putn( outbuf );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        buffer inbuf = is.getn( fsize );
        is.close();
        buffer expected{ "abcdefghijklmnop" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::at_end );
        auto pos = os.tell( seek_anchor::current );
        CHECK( pos == 16 );
        auto fsize = os.tell( seek_anchor::end );
        CHECK( fsize == 16 );
        buffer outbuf{ "qrstuvwxyz" };
        os.putn( outbuf );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        CHECK( fsize == 26 );
        buffer inbuf = is.getn( fsize );
        is.close();
        buffer expected{ "abcdefghijklmnopqrstuvwxyz" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        CHECK( fsize == 26 );
        is.seek( seek_anchor::begin, 16 );
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
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::append );
        auto pos = os.tell( seek_anchor::current );
        CHECK( pos == 26);
        buffer outbuf{ "0123456789" };
        os.putn( outbuf );
        auto zpos = os.seek( seek_anchor::begin, 0 );
        os.putn( outbuf );
        zpos = os.tell( seek_anchor::current );
        os.close();
    }
    {
        bstream::ofbstream os( "fbstream_test_file" );
        auto pos = os.tell( bstream::seek_anchor::current );
        CHECK( pos == 0 );
        auto fsize = os.tell( bstream::seek_anchor::end );
        CHECK( fsize == 46 );
        buffer outbuf{ "0123456789" };
        os.putn( outbuf );
        auto zpos = os.tell( bstream::seek_anchor::current );
        CHECK( zpos == 10 );
        os.close();
    }
    {
        bstream::ifbstream is( "fbstream_test_file" );
        auto fsize = is.tell( seek_anchor::end );
        CHECK( fsize == 46 );
        buffer inbuf = is.getn( fsize );
        is.close();
        buffer expected{ "0123456789klmnopqrstuvwxyz01234567890123456789" };
        CHECK( expected == inbuf );
    }
    {
        bstream::ofbstream os( "fbstream_test_file", bstream::open_mode::truncate );
        buffer outbuf{ "abcdefghijklmnop" };
        os.putn( outbuf );
        os.seek( seek_anchor::begin, 36 );
        buffer outbuf2{ "0123456789" };
        os.putn( outbuf2 );
        auto zpos = os.tell( seek_anchor::current );
        CHECK( zpos == 46 );
        os.close();
    }
    
}
