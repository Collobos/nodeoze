#include <nodeoze/json.h>
#include <nodeoze/test.h>

using namespace nodeoze;

TEST_CASE( "nodeoze/smoke/json" )
{
	SUBCASE( "inflate" )
	{
		auto obj = json::inflate( "{ \"a\" : [ 1, 2, 3 ], \"b\" : { \"c\" : true } }" );
		REQUIRE( obj.type() == any::type_t::object );
		REQUIRE( obj.is_member( "a" ) );
		REQUIRE( obj[ "a" ].is_array() );
		REQUIRE( obj[ "a" ].size() == 3 );
		REQUIRE( obj.is_member( "b" ) );
		REQUIRE( obj[ "b" ].is_object() );
		REQUIRE( obj[ "b" ].size() == 1 );
		REQUIRE( obj[ "b" ][ "c" ] == true );
	}
	
	SUBCASE( "deflate" )
	{
		auto obj = any::build(
		{
			{ "a", { 1, 2, 3 } },
			{ "b", { { "c", true } } },
			{ "c", "\b\f\n\r\t\"\\embedded" }
		} );
		
		REQUIRE( obj.type() == any::type_t::object );
		REQUIRE( obj.is_member( "a" ) );
		REQUIRE( obj[ "a" ].is_array() );
		REQUIRE( obj[ "a" ].size() == 3 );
		REQUIRE( obj.is_member( "b" ) );
		REQUIRE( obj[ "b" ].is_object() );
		REQUIRE( obj[ "b" ].size() == 1 );
		REQUIRE( obj[ "b" ][ "c" ] == true );
		REQUIRE( obj[ "c" ].is_string() );
		REQUIRE( obj[ "c" ].to_string() == "\b\f\n\r\t\"\\embedded" );
		
		{
			std::ostringstream os;
		
			auto str = json::deflate_to_stream( os, obj ).str();
			REQUIRE( str.size() > 0 );
		
			auto obj2 = json::inflate( str );
			REQUIRE( obj == obj2 );
		}
		
		{
			ostringstream os;
		
			auto str = json::deflate_to_stream( os, obj ).str();
			REQUIRE( str.size() > 0 );
		
			auto obj2 = json::inflate( str );
			REQUIRE( obj == obj2 );
		}
	}
}
