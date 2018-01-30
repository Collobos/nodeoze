#include <nodeoze/nprocess.h>
#include <nodeoze/nany.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nbuffer.h>
#include <nodeoze/nthread.h>
#include <nodeoze/ntest.h>
#include <uv.h>
#include <fstream>
#include <cassert>
#include <memory>
#include <queue>

using namespace nodeoze;

process::process( const any &root )
:
	m_owner( root[ "owner" ].to_string() ),
	m_pid( root[ "pid" ].to_uint64() )
{
}


any
process::to_any() const
{
	any root;
	
	root[ "owner" ] = m_owner;
	root[ "pid" ]	= m_pid;
	
	return root;
}


scoped_operation
always_running_process::create( const nodeoze::path &exe, const std::vector< std::string > &args, const process::env_t &env, const nodeoze::path &working_directory, process::input_f stdin_handler, process::output_f stdout_handler, process::output_f stderr_handler )
{
	auto pid		= std::make_shared< process::pid_t >( 0 );
	auto restart	= [=]() mutable
	{
		*pid = 0;

		runloop::shared().schedule_oneshot_timer( std::chrono::seconds( 2 ), [=]( auto event ) mutable
		{
			nunused( event );

			create( exe, args, env, working_directory, stdin_handler, stdout_handler, stderr_handler );
		} );
	};

	process::create( exe, args, env, working_directory, *pid, stdin_handler, stdout_handler, stderr_handler )
	.then( [=]( auto result ) mutable
	{
		nunused( result );

#if defined( __APPLE__ ) || defined( __linux__ )

		if ( result.second != SIGTERM )
		{
			mlog( marker::process, log::level_t::warning, "process % exited unexpectedly, exit status %, term signal %", exe, result.first, result.second );
		}

#else
		
		mlog( marker::process, log::level_t::warning, "process % exited unexpectedly, exit status %, term signal %", exe, result.first, result.second );

#endif

		restart();
	},
	[=]( auto err ) mutable
	{
		nunused( err );
		
		mlog( marker::process, log::level_t::error, "unable to start process %, err % (%)", exe, err.value(), err.message() );

		restart();
	} );
	
	return scoped_operation::create( [=]( void *v ) mutable
	{
		nunused( v );
		
		if ( *pid != 0 )
		{
			uv_kill( *pid, SIGTERM );
		}
	} );
}


TEST_CASE( "nodeoze/smoke/process" )
{
	SUBCASE( "create" )
	{
		auto tid = thread::id();
		CHECK( tid != 0 );
		
		auto file = path::tmp() + "test.js";
		
		auto os = std::ofstream( file.to_string() );
		
		os << "let envString = process.env.NODEOZE_TEST_STRING;" << std::endl;
		os << "if ( envString === 'secret' )" << std::endl;
		os << "{" << std::endl;
		os << "process.stdin.on( 'data', ( message ) =>" << std::endl;
		os << "{" << std::endl;
		os << "process.stdout.write( message.toString() );" << std::endl;
		os << "process.exit( 0 );" << std::endl;
		os << "} );" << std::endl;
		os << "}" << std::endl;
		os << "else" << std::endl;
		os << "{" << std::endl;
		os << "process.exit( 1 );" << std::endl;
		os << "}" << std::endl;
		
		os.flush();
		os.close();

#if defined( WIN32 )

		auto command	= nodeoze::path( "C:\\Program Files\\nodejs\\node.exe" );

#elif defined( __APPLE__ )

		auto command	= nodeoze::path( "/usr/local/bin/node" );

#elif defined( __linux__ )

		auto command	= nodeoze::path( "/usr/bin/nodejs" );

#endif

		auto str		= std::string( "hello, how are you" );
		auto pid		= process::pid_t( 0 );
		auto done		= false;

		process::create( command, { file.to_string() }, { "NODEOZE_TEST_STRING=secret" }, path::home(), pid, [&]()
		{
			return str;
		},
		[&]( const nodeoze::buffer &buf ) mutable
		{
			CHECK( buf.to_string() == str );
		} )
		.then( [&]( auto pair ) mutable
		{
			CHECK( tid == thread::id() );
			CHECK( pair.first == 0 );
			CHECK( pair.second == 0 );
			done = true;
		},
		[&]( auto err ) mutable
		{
			fprintf( stderr, "err: %d (%s)\n", err.value(), err.message().c_str() );
			CHECK( !err );
			done = true;
		} );

		while ( !done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}
}
