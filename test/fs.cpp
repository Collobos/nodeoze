

TEST_CASE( "nodeoze/smoke/address" )
{
	SUBCASE( "less than" )
	{
		ip::address lhs( ip::address::type_t::unknown );
		ip::address rhs( ip::address::type_t::unknown );
		
		CHECK( !( lhs < rhs ) );
		
		rhs = ip::address( ip::address::type_t::v4 );
		
		CHECK( lhs < rhs );
		
		lhs = ip::address( ip::address::type_t::v4 );
		
		CHECK( !( lhs < rhs ) );
		
		rhs = ip::address::v4_loopback();
		
		CHECK( lhs < rhs );
		
		rhs = ip::address::v6_any();
		
		CHECK( lhs < rhs );
		
		lhs = ip::address::v6_any();
		
		CHECK( !( lhs < rhs ) );
	}

	SUBCASE( "resolve correctly" )
	{
		if ( machine::self().has_internet_connection() )
		{
			runloop::shared().run( runloop::mode_t::nowait );

			auto done = std::make_shared< bool >( false );

			ip::address::resolve( "xtheband.com" )
			.then( [=]( std::vector< ip::address > addresses )
			{
				CHECK( addresses.size() > 0 );
				*done = true;
			},
			[=]( std::error_code err ) 
			{
				nunused( err );

				CHECK( 0 );
			} );

			while ( !done )
			{
				runloop::shared().run( runloop::mode_t::once );
			}
		}
	}

	SUBCASE( "resolve incorrectly" )
	{
		auto done = std::make_shared< bool >( false );

		ip::address::resolve( "thishostdoesnotexist.xyz" )
		.then( [=]( std::vector< ip::address > addresses )
		{
			*done = true;
		},
		[=]( std::error_code err ) 
		{
			CHECK( err.value() != 0 );
			*done = true;
		} );

		while ( !done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}
	
	SUBCASE( "check v4 loopback" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( 127 );
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v4() );
		CHECK( a.is_loopback_v4() );
	}
	
	SUBCASE( "check v6 loopback" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v6() );
		CHECK( a.is_loopback_v6() );
	}
	
	SUBCASE( "check v4 link local" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( 169 );
		bytes.push_back( 254 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v4() );
		CHECK( a.is_link_local_v4() );
	}
	
	SUBCASE( "check v6 link local" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( htons( 0xfe80 ) );
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v6() );
		CHECK( a.is_link_local_v6() );
	}
}
