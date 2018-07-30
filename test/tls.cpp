#include <nodeoze/tls.h>
#include <nodeoze/test.h>

using namespace nodeoze;

TEST_CASE( "nodeoze/smoke/tls" )
{
	SUBCASE( "session" )
	{
		std::string key;
		std::string cert;

		tls::create_self_signed_cert( "nodeoze", "usa", "collobos software inc", "nodeoze@collobos.com", key, cert );
		auto server = tls::server::create( tls::server::options( key, cert ) );
		REQUIRE( server );

		auto client = tls::client::create( tls::client::options() );
		REQUIRE( client );
		client->on( "error", [&]( std::error_code err ) mutable
		{
			fprintf( stderr, "client got error %d (%s)\n", err.value(), err.message().c_str() );
			CHECK( !err );
		} );

		client->pipe( server );

		server->pipe( client );

		server->on( "data", [&]( buffer buf ) mutable
		{
			fprintf( stderr, "server received %d bytes\n", buf.size() );
		} );

		auto ret = client->encrypt( "hello world", [&]()
		{
		} );
		CHECK( ret );

		bool done = false;

		while ( !done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}
}