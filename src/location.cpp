#include <nodeoze/location.h>
#include <nodeoze/connection.h>

using namespace nodeoze;

promise< location >
location::from_ip_address()
{
	auto request	= std::make_shared< http::message >( http::method_t::get, uri( "http://ipinfo.io/json" ) );
	
	/*
	 * not sure why but ipinfo.io sends us a 302 https redirect when we use our normal User-Agent string,
	 * but setting it to curl doesn't do that
	 *
	 * who knows???
	 */
	
	request->add_header_field( "User-Agent", "curl/7.54.0" );
	request->add_header_field( "Accept", "*/*" );

	auto connection = connection::create< http::connection >( request->resource() );
	auto ret		= promise< location >();

	connection->send_request( request )
	.then( [=]( http::message::ptr response ) mutable
	{
		auto root	= any();
		auto string = std::string();
		auto pos	= std::string::npos;
		auto lat	= std::string();
		auto lon	= std::string();
		auto loc	= location( 37.419723, -122.115228, 0.0, 0.0, 0.0, 0.0 );

		root = json::inflate( response->body().to_string() );
		ncheck_error_action( root.is_object(), ret.reject( make_error_code( std::errc::invalid_argument ), reject_context ), exit, "response is not JSON object" );
		string = json::deflate_to_string( root[ "loc" ] );
		ncheck_error_action( !string.empty(), ret.reject( make_error_code( std::errc::invalid_argument ), reject_context ), exit, "bad JSON string" );
		pos = string.find( ',' );
		ncheck_error_action( pos != std::string::npos, ret.reject( make_error_code( std::errc::invalid_argument ), reject_context ), exit, "bad JSON string" );
		lat = string.substr( 0, pos );
		lon = string.substr( pos + 1, string.size() - pos - 1 );
		loc.set_latitude_degrees( std::atof( lat.c_str() ) );
		loc.set_longitude_degrees( std::atof( lon.c_str() ) );

		ret.resolve( std::move( loc ) );
		
	exit:
	
		connection->close();
	},
	[=]( auto err ) mutable
	{
		ret.reject( err, reject_context );
		connection->close();
	} );

	return ret;
}
