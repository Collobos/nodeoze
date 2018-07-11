


TEST_CASE( "nodeoze/smoke/msgpack")
{
	SUBCASE( "inflate/deflate" )
	{
		any root;

		root[ "a" ] = true;
		root[ "b" ] = 1;
		root[ "c" ] = -1;
		root[ "d" ] = 10.234;
		root[ "e" ] = "test";

		auto buf	= mpack::deflate( root );
		auto copy	= mpack::inflate( buf.to_string() );

		CHECK( root == copy );
	}

	SUBCASE( "from javascript" )
	{
		std::vector< std::uint8_t > vec = { 132,167,106,115,111,110,114,112,99,163,50,46,48,166,109,101,116,104,111,100,217,34,47,99,111,110,100,117,99,116,111,114,47,114,101,112,101,97,116,101,114,47,117,112,100,97,116,101,95,111,98,106,101,99,116,115,166,112,97,114,97,109,115,146,217,176,83,66,66,65,81,49,82,97,86,69,85,86,67,50,56,68,72,81,73,102,65,104,56,71,97,120,103,84,85,70,78,86,82,108,90,67,81,82,69,73,97,65,99,80,66,104,48,65,65,81,107,89,66,66,48,65,65,119,78,117,71,104,82,86,86,86,120,101,88,49,49,65,82,85,66,83,82,108,120,69,70,65,53,70,81,48,74,85,71,66,70,85,83,107,78,98,81,86,100,67,88,86,53,102,97,70,86,86,82,49,81,81,67,81,77,71,66,65,99,66,65,81,85,70,65,81,119,102,69,49,70,99,88,108,120,86,86,48,66,85,86,82,85,76,85,108,74,100,81,86,89,101,69,86,53,90,82,48,85,84,68,81,69,89,69,85,82,66,86,107,65,82,68,65,100,74,145,131,163,111,105,100,29,165,116,97,98,108,101,167,115,101,114,118,105,99,101,167,112,97,116,99,104,101,115,145,131,162,111,112,163,97,100,100,164,112,97,116,104,169,47,108,111,99,97,116,105,111,110,165,118,97,108,117,101,130,168,108,97,116,105,116,117,100,101,202,66,21,204,95,169,108,111,110,103,105,116,117,100,101,202,194,244,92,36,162,105,100,6 };

		auto buf = buffer( vec.data(), vec.size() );

		auto root = mpack::inflate( buf.to_string() );

		CHECK( root.is_object() );
		CHECK( root[ "jsonrpc" ].to_string() == "2.0" );
		CHECK( root[ "method" ].to_string() == "/conductor/repeater/update_objects" );
		CHECK( root[ "params" ].is_array() );
		CHECK( root[ "params" ].size() == 2 );
		CHECK( root[ "params" ][ 0 ] == "SBBAQ1RaVEUVC28DHQIfAh8GaxgTUFNVRlZCQREIaAcPBh0AAQkYBB0AAwNuGhRVVVxeX11ARUBSRlxEFA5FQ0JUGBFUSkNbQVdCXV5faFVVR1QQCQMGBAcBAQUFAQwfE1FcXlxVV0BUVRULUlJdQVYeEV5ZR0UTDQEYEURBVkARDAdJ" );
		CHECK( root[ "params" ][ 1 ].is_array() );
		CHECK( root[ "params" ][ 1 ].size() == 1 );
		CHECK( root[ "params" ][ 1 ][ 0 ][ "oid" ] == 29 );
		CHECK( root[ "params" ][ 1 ][ 0 ][ "patches" ].is_array() );
		CHECK( root[ "params" ][ 1 ][ 0 ][ "patches" ].size() == 1 );
		CHECK( root[ "params" ][ 1 ][ 0 ][ "patches" ][ 0 ][ "op" ] == "add" );
		CHECK( root[ "params" ][ 1 ][ 0 ][ "patches" ][ 0 ][ "path" ] == "/location" );
		CHECK( root[ "params" ][ 1 ][ 0 ][ "patches" ][ 0 ][ "value" ].is_object() );
		// CHECK( root[ "params" ][ 1 ][ 0 ][ "patches" ][ 0 ][ "value" ][ "latitude" ] == 37.449582 );
		// CHECK( root[ "params" ][ 1 ][ 0 ][ "patches" ][ 0 ][ "value" ][ "longitude" ] == -122.179963 );
	}
}
