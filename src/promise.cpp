#include <nodeoze/promise.h>
#include <nodeoze/runloop.h>
#include <nodeoze/test.h>
#include <cassert>

using namespace nodeoze;

static int
func()
{
	return 42;
}


static promise< void >
make_v( std::chrono::milliseconds delay )
{
	auto p = promise< void >();

	runloop::shared().schedule_oneshot_timer( delay, [=]( runloop::event event ) mutable
	{
		nunused( event );
		p.resolve();
	} );

	return p;
}


static promise< int >
make_p( int i, std::chrono::milliseconds delay )
{
	auto p = promise< int >();

	runloop::shared().schedule_oneshot_timer( delay, [=]( runloop::event event ) mutable
	{
		nunused( event );
		p.resolve( std::move( i ) );
	} );

	return p;
}


static promise< int >
make_e( int err, std::chrono::milliseconds delay )
{
	auto p = promise< int >();

	runloop::shared().schedule_oneshot_timer( delay, [=]( runloop::event event ) mutable
	{
		nunused( event );
		p.reject( std::error_code( err, std::generic_category() ), reject_context );
	} );

	return p;
}

TEST_CASE( "nodeoze/smoke/promise" )
{
	SUBCASE( "synchronous chaining" )
	{
		auto done = std::make_shared< bool >( false );
		promise< void > p;
		
		p.then( [=]()
		{
			return 7;
		} )
		.then( [=]( int i )
		{
			CHECK( i == 7 );
			return "blob";
		} )
		.then( [=]( const std::string &s )
		{
			CHECK( s == "blob" );
			return 7.5;
		} )
		.then( [=]( double f )
		{
			CHECK( f == 7.5 );

			promise< bool > p1;

			p1.resolve( true );

			return p1;
		} )
		.then( [=]( bool b )
		{
			CHECK( b == true );
			return func();
		} )
		.then( [=]( int i )
		{
			CHECK( i == 42 );
		},
		[=]( std::error_code err )
		{
			nunused( err );
			assert( 0 );
		} )
		.finally( [=]() mutable
		{
			*done = true;
		} );

		CHECK( !*done );

		p.resolve();

		CHECK( *done );
	}
	
	SUBCASE( "synchronous chaining with error hander" )
	{
		auto done = std::make_shared< bool >( false );
		promise< void > p;
		
		p.then( [=]()
		{
			return 7;
		} )
		.then( [=]( int i )
		{
			CHECK( i == 7 );

			promise< std::string > p1;

			p1.reject( std::error_code( 42, std::generic_category() ), reject_context );

			return p1;
		} )
		.then( [=]( const std::string &s )
		{
			nunused( s );
			assert( 0 );
			return 7.5;
		} )
		.then( [=]( double f )
		{
			nunused( f );
			assert( 0 );
			promise< bool > p1;

			p1.resolve( true );

			return p1;
		} )
		.then( [=]( bool b )
		{
			nunused( b );
			assert( 0 );
			return func();
		} )
		.then( [=]( int i )
		{
			nunused( i );
			assert( 0 );
		},
		[=]( const std::error_code &err )
		{
			CHECK( err.value() == 42 );
			CHECK( err.category() == std::generic_category() );
		} )
		.finally( [=]() mutable
		{
			*done = true;
		} );

		CHECK( !*done );

		p.resolve();

		CHECK( *done );
	}
	
	SUBCASE( "asynchronous chaining" )
	{
		promise< int > p;
		auto done = std::make_shared< bool >( false );

		p.then( [=]( int i )
		{
			CHECK( i == 7 );
			return "blob";
		} )
		.then( [=]( const std::string &s )
		{
			CHECK( s == "blob" );
			return 7.5;
		} )
		.then( [=]( double f )
		{
			CHECK( f == 7.5 );

			promise< bool > p1;

			runloop::shared().dispatch( [=]() mutable
			{
				p1.resolve( true );
			} );

			return p1;
		} )
		.then( [=]( bool b )
		{
			CHECK( b == true );
			return func();
		} )
		.then( [=]( int i ) mutable
		{
			CHECK( i == 42 );
			*done = true;
		},
		[=]( std::error_code err )
		{
			nunused( err );
			assert( 0 );
		} );

		runloop::shared().dispatch( [=]() mutable
		{
			p.resolve( 7 );
		} );

		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );

		CHECK( *done );
	}

	SUBCASE( "asynchronous chaining with error hander" )
	{
		promise< int > p;
		auto done = std::make_shared< bool >( false );

		p.then( [=]( int i )
		{
			CHECK( i == 7 );

			promise< std::string > p1;

			runloop::shared().dispatch( [=]() mutable
			{
				p1.reject( std::error_code( 42, std::generic_category() ), reject_context );
			} );

			return p1;
		} )
		.then( [=]( const std::string &s )
		{
			nunused( s );
			assert( 0 );
			return 7.5;
		} )
		.then( [=]( double f )
		{
			nunused( f );
			assert( 0 );
			promise< bool > p1;

			p1.resolve( true );

			return p1;
		} )
		.then( [=]( bool b )
		{
			nunused( b );
			assert( 0 );

			return func();
		} )
		.then( [=]( int i )
		{
			nunused( i );
			assert( 0 );
		},
		[=]( const std::error_code &err )
		{
			CHECK( err.value() == 42 );
			CHECK( err.category() == std::generic_category() );
			*done = true;
		} );

		runloop::shared().dispatch( [=]() mutable
		{
			p.resolve( 7 );
		} );

		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );

		CHECK( *done );
	}
	
	SUBCASE( "synchronous all <void>" )
	{
		auto done = std::make_shared< bool >( false );

		promise< void >::all(
		{
			make_v( std::chrono::milliseconds( 300 ) ),
			make_v( std::chrono::milliseconds( 200 ) ),
			make_v( std::chrono::milliseconds( 100 ) )
		} ).then( [=]() mutable
		{
			*done = true;
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			assert( 0 );
		} );

		while ( *done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}

	SUBCASE( "synchronous all <int>" )
	{
		auto done = std::make_shared< bool >( false );

		promise< int >::all(
		{
			make_p( 10, std::chrono::milliseconds( 300 ) ),
			make_p( 20, std::chrono::milliseconds( 200 ) ),
			make_p( 30, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( auto results ) mutable
		{
			CHECK( results.size() == 3 );
			CHECK( results[ 0 ] == 10 );
			CHECK( results[ 1 ] == 20 );
			CHECK( results[ 2 ] == 30 );
			*done = true;
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			assert( 0 );
		} );

		while ( *done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}

	SUBCASE( "any with no errors" )
	{
		auto done = std::make_shared< bool >( false );

		promise< int >::any(
		{
			make_p( 10, std::chrono::milliseconds( 300 ) ),
			make_p( 20, std::chrono::milliseconds( 100 ) ),
			make_p( 30, std::chrono::milliseconds( 200 ) )
		} ).then( [=]( int val ) mutable
		{
			CHECK( val == 20 );
			*done = true;
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			assert( 0 );
		} );

		while ( !*done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}

	SUBCASE( "any with all errors" )
	{
		auto done = std::make_shared< bool >( false );

		promise< int >::any(
		{
			make_e( 70, std::chrono::milliseconds( 300 ) ),
			make_e( 71, std::chrono::milliseconds( 200 ) ),
			make_e( 72, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( int val ) mutable
		{
			nunused( val );
			assert( 0 );
		},
		[=]( std::error_code err ) mutable
		{
			CHECK( err.value() == 70 );
			*done = true;
		} );

		while ( !*done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}

	SUBCASE( "any with errors" )
	{
		auto done = std::make_shared< bool >( false );

		promise< int >::any(
		{
			make_e( 72, std::chrono::milliseconds( 100 ) ),
			make_p( 20, std::chrono::milliseconds( 200 ) ),
			make_e( 72, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( int val ) mutable
		{
			CHECK( val == 20 );
			*done = true;
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			assert( 0 );
		} );

		while ( !*done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}
	
	SUBCASE( "race with no errors< void >" )
	{
		auto done = std::make_shared< bool >( false );

		promise< void >::race(
		{
			make_v( std::chrono::milliseconds( 300 ) ),
			make_v( std::chrono::milliseconds( 100 ) ),
			make_v( std::chrono::milliseconds( 200 ) )
		} ).then( [=]() mutable
		{
			*done = true;
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			assert( 0 );
		} );

		while ( !*done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}

	SUBCASE( "race with no errors< int >" )
	{
		auto done = std::make_shared< bool >( false );

		promise< int >::race(
		{
			make_p( 10, std::chrono::milliseconds( 300 ) ),
			make_p( 20, std::chrono::milliseconds( 100 ) ),
			make_p( 30, std::chrono::milliseconds( 200 ) )
		} ).then( [=]( int val ) mutable
		{
			CHECK( val == 20 );
			*done = true;
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			assert( 0 );
		} );

		while ( !*done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}

	SUBCASE( "race with errors" )
	{
		auto done = std::make_shared< bool >( false );

		promise< int >::race(
		{
			make_e( 72, std::chrono::milliseconds( 300 ) ),
			make_p( 20, std::chrono::milliseconds( 200 ) ),
			make_e( 75, std::chrono::milliseconds( 100 ) )
		} ).then( [=]( int val ) mutable
		{
			nunused( val );
			assert( 0 );
		},
		[=]( std::error_code err ) mutable
		{
			nunused( err );
			CHECK( err.value() == 75 );
			*done = true;
		} );

		while ( !*done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}
	
	SUBCASE( "all with no promises" )
	{
		promise< int >::all( {} )
		.then( [=]( auto val ) mutable
		{
			CHECK( val.size() == 0 );
		},
		[=]( std::error_code err ) mutable
		{
			CHECK( err.value() == static_cast< int >( std::errc::invalid_argument ) );
		} );
	}
	
	SUBCASE( "any void with first succeed" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.resolve();
		p2.reject( make_error_code( std::errc::not_connected ), reject_context );
		p3.reject( make_error_code( std::errc::not_connected ), reject_context );

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			good = true;
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with first succeed" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.resolve( 7 );
		p2.reject( make_error_code( std::errc::not_connected ), reject_context );
		p3.reject( make_error_code( std::errc::not_connected ), reject_context );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int val ) mutable
		{
			CHECK( val == 7 );
			good = true;
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any void with second succeed" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.reject( make_error_code( std::errc::not_connected ), reject_context );
		p2.resolve();
		p3.reject( make_error_code( std::errc::not_connected ), reject_context );

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			good = true;
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with second succeed" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.reject( make_error_code( std::errc::not_connected ), reject_context );
		p2.resolve( 8 );
		p3.reject( make_error_code( std::errc::not_connected ), reject_context );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int val ) mutable
		{
			CHECK( val == 8 );
			good = true;
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any void with third succeed" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.reject( make_error_code( std::errc::not_connected ), reject_context );
		p2.reject( make_error_code( std::errc::not_connected ), reject_context );
		p3.resolve();

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			good = true;
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = false;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with third succeed" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.reject( make_error_code( std::errc::not_connected ), reject_context );
		p2.reject( make_error_code( std::errc::not_connected ), reject_context );
		p3.resolve( 9 );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int val ) mutable
		{
			CHECK( val == 9 );
			good = true;
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = false;
		} );

		CHECK( good );
	}

	SUBCASE( "any void with all fail" )
	{
		auto p1 = promise< void >();
		auto p2 = promise< void >();
		auto p3 = promise< void >();

		p1.reject( make_error_code( std::errc::not_connected ), reject_context );
		p2.reject( make_error_code( std::errc::not_connected ), reject_context );
		p3.reject( make_error_code( std::errc::not_connected ), reject_context );

		auto good = false;

		promise< void >::any( { p1, p2, p3 } )
		.then( [&]() mutable
		{
			CHECK( false );
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = true;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any int with all fail" )
	{
		auto p1 = promise< int >();
		auto p2 = promise< int >();
		auto p3 = promise< int >();

		p1.reject( make_error_code( std::errc::not_connected ), reject_context );
		p2.reject( make_error_code( std::errc::not_connected ), reject_context );
		p3.reject( make_error_code( std::errc::not_connected ), reject_context );

		auto good = false;

		promise< int >::any( { p1, p2, p3 } )
		.then( [&]( int val ) mutable
		{
			nunused( val );
			CHECK( false );
		},
		[&]( auto err ) mutable
		{
			nunused( err );
			good = true;
		} );

		CHECK( good );
	}
	
	SUBCASE( "any with no promises" )
	{
		promise< int >::any( {} ).then( [=]( int val ) mutable
		{
			nunused( val );
			assert( 0 );
		},
		[=]( std::error_code err ) mutable
		{
			CHECK( err.value() == static_cast< int >( std::errc::invalid_argument ) );
		} );
	}
	
	SUBCASE( "race with no promises" )
	{
		promise< int >::race( {} ).then( [=]( int val ) mutable
		{
			nunused( val );
			assert( 0 );
		},
		[=]( std::error_code err ) mutable
		{
			CHECK( err.value() == static_cast< int >( std::errc::invalid_argument ) );
		} );
	}
	
	SUBCASE( "simple void" )
	{
		auto string = std::string( "blob" );
		promise< void > p;

		p.then( [=]()
		{
			return string;
		} )
		.then( [=]( const std::string &s )
		{
			CHECK( s == string );
			
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
		},
		[=]( std::error_code err )
		{
			nunused( err );
			assert( 0 );
		} );

		p.resolve();
	}
	
	SUBCASE( "complex void" )
	{
		std::vector< std::string > strings = { "a", "b", "c" };
		
		promise< void > p;
		
		auto func1 = []()
		{
			auto ret = promise< void >();
			
			ret.resolve();
			
			return ret;
		};
		
		auto func2 = []()
		{
			auto strings	= std::vector< std::string >( { "a", "b", "c" } );
			auto ret		= promise< std::vector< std::string > >();
			
			ret.resolve( strings );
			
			return ret;
		};
			

		func1()
		.then( [=]()
		{
			return func2();
		} )
		.then( [=]( std::vector< std::string > incoming )
		{
			CHECK( incoming == strings );
			
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
			promise< void > p1;

			p1.resolve();

			return p1;
		} )
		.then( [=]()
		{
		},
		[=]( std::error_code err )
		{
			nunused( err );
			assert( 0 );
		} );

		p.resolve();
	}
	
	SUBCASE( "leaks" )
	{
		auto clean = std::make_shared< bool >( false );
		
		struct leaker
		{
			std::shared_ptr< bool > m_flag;
			
			leaker( std::shared_ptr< bool > flag )
			:
				m_flag( flag )
			{
			}
			
			~leaker()
			{
				*m_flag = true;
			}
			
			void
			func()
			{
			}
		};
		
		{
			auto ptr = std::make_shared< leaker >( clean );
			
			auto func1 = []() mutable
			{
				auto ret = promise< void >();
				
				runloop::shared().schedule_oneshot_timer( std::chrono::milliseconds( 10 ), [=]( auto event ) mutable
				{
					nunused( event );
					ret.resolve();
				} );

				return ret;
			};
			
			auto func2 = []() mutable
			{
				auto ret = promise< void >();
				
				runloop::shared().schedule_oneshot_timer( std::chrono::milliseconds( 10 ), [=]( auto event ) mutable
				{
					nunused( event );
					ret.resolve();
				} );

				return ret;
			};
			
			auto func3 = []() mutable
			{
				auto ret = promise< void >();
				
				runloop::shared().schedule_oneshot_timer( std::chrono::milliseconds( 10 ), [=]( auto event ) mutable
				{
					nunused( event );
					ret.resolve();
				} );

				return ret;
			};

			auto ret = promise< std::shared_ptr< leaker > >();
			
			REQUIRE( *clean == false );
			
			func1()
			.then( [=]() mutable
			{
				return func2();
			} )
			.then( [=]() mutable
			{
				return func3();
			} )
			.then( [=]() mutable
			{
				ret.resolve( ptr );
			},
			[=]( auto err ) mutable
			{
				nunused( err );
			} );
			
			REQUIRE( !ret.is_finished() );
			REQUIRE( *clean == false );
			
			while ( !ret.is_finished() )
			{
				runloop::shared().run( runloop::mode_t::once );
			}
		}
		
		REQUIRE( *clean == true );
	}

	SUBCASE( "timeout" )
	{
		auto err	= std::error_code();
		auto func	= []()
		{
			auto ret = promise< void >();

			runloop::shared().schedule_oneshot_timer( std::chrono::milliseconds( 20 ), [=]( auto event ) mutable
			{
				nunused( event );

				if ( !ret.is_finished() )
				{
					ret.resolve();
				}
			} );

			return ret;
		};

		auto done = false;

		func()
		.timeout( std::chrono::milliseconds( 10 ) )
		.then( [&]() mutable
		{
			done = true;
		},
		[&]( auto _err ) mutable
		{
			done = true;
			err = _err;
		} );

		while ( !done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}

		CHECK( err == std::errc::timed_out );
	}
}
