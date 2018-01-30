/*
 * Copyright (c) 2013-2017, Collobos Software Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <nodeoze/napplication.h>
#include <nodeoze/nunicode.h>
#include <nodeoze/ntest.h>
#include <nodeoze/nany.h>
#include <nodeoze/nlog.h>
#include <limits>
#include <sstream>
#include <vector>

using namespace nodeoze;


static application::option::setup setup( application::init_options );

std::unordered_map< std::string, application::option* > *application::option::m_instances = nullptr;

application *application::m_shared = nullptr;

bool
application::parse_command_line( int argc, std::tchar_t **argv )
{
	bool ok = true;
	
	for ( auto &it : option::instances() )
	{
		if ( it.second->name() != "--test" )
		{
			it.second->clear();
		}
	}
	
	for ( auto i = 0; i < argc; )
	{
#if defined( WIN32 )
		std::string param( narrow( argv[ i ] ) );
#else
		std::string param( argv[ i ] );
#endif
		if ( is_switch( param ) )
		{
			auto it = option::instances().find( param );
		
			if ( it != option::instances().end() )
			{
				it->second->set_is_set( true );
				
				while ( ++i < argc )
				{
#if defined( WIN32 )
					param = narrow( argv[ i ] );
#else
					param = argv[ i ];
#endif

					if ( it->second->is_variable_size() || !is_switch( param ) )
					{
						if ( !it->second->push_back( param ) )
						{
							fprintf( stderr, "syntax error for option '%s'\n", it->second->name().c_str() );
							ok = false;
							break;
						}
					}
					else
					{
						break;
					}
				}
				
				if ( !*it->second )
				{
					fprintf( stderr, "syntax error for option '%s'\n", it->second->name().c_str() );
					ok = false;
					break;
				}
			}
			else if ( is_doctest_option( param ) )
			{
				i++;
			}
			else
			{
				fprintf( stderr, "unknown option '%s'\n", param.c_str() );
				ok = false;
				break;
			}
		}
		else
		{
			++i;
		}
	}
	
	if ( log_markers_option().is_set() )
	{
		auto strings = string::split( log_markers_option().string_at_index( 0 ), ',' );
		
		for ( auto &string : strings )
		{
			log::shared().enable( log::shared().get_marker( string::split( string, '.' ) ) );
		}
	}
	
	if ( unset_log_markers_option().is_set() )
	{
		auto strings = string::split( unset_log_markers_option().string_at_index( 0 ), ',' );
		
		for ( auto &string : strings )
		{
			log::shared().disable( log::shared().get_marker( string::split( string, '.' ) ) );
		}
	}
	
	return ok;
}


application::application( const std::string &name )
:
	m_okay( true ),
	m_name( name )
{
	log::shared().set_name( name );
	
	assert( m_shared == nullptr );
	m_shared = this;
	
	if ( log_markers_option().is_set() )
	{
		auto strings = string::split( log_markers_option().string_at_index( 0 ), ',' );
		
		for ( auto &string : strings )
		{
			log::shared().enable( log::shared().get_marker( string::split( string, '.' ) ) );
		}
	}
	
	if ( unset_log_markers_option().is_set() )
	{
		auto strings = string::split( unset_log_markers_option().string_at_index( 0 ), ',' );
		
		for ( auto &string : strings )
		{
			log::shared().disable( log::shared().get_marker( string::split( string, '.' ) ) );
		}
	}
	
	if ( log_level_option().is_set() )
	{
		log::shared().set_level( static_cast< log::level_t >( log_level_option().int_at_index( 0 ) ) );
	}
	else
	{
		log::shared().set_level( log::level_t::info );
	}
}


application::~application()
{
	assert( m_shared );
	m_shared = nullptr;
}


char**
application::normalize_argv( int argc, std::tchar_t **argv )
{
#if defined( UNICODE )

	char **new_argv = new char*[ argc ];
	
	for ( auto i = 0; i < argc; i++ )
	{
		new_argv[ i ] = strdup( narrow( argv[ i ] ).c_str() );
	}
	
	return new_argv;
	
#else

	nunused( argc );

	return argv;

#endif
}


bool
application::is_doctest_option( const std::string &option )
{
	return ( option.find( "--dt" ) == 0 ) ? true : false;
}


TEST_CASE( "nodeoze/smoke/application" )
{
	SUBCASE( "good command line options" )
	{
		const std::tchar_t	*argv[] =
		{
			ntext( "--log-markers" ),
			ntext( "nodeoze" ),
			ntext( "--unset-log-markers" ),
			ntext( "nodeoze" ),
			ntext( "--log-level" ),
			ntext( "10" ),
			ntext( "--log-throttle" ),
			ntext( "0" ),
			ntext( "--test" ),
			ntext( "--dt-version" )
		};
		int		argc = sizeof( argv ) / sizeof( std::tchar_t* );
		
		CHECK( application::parse_command_line( argc, ( std::tchar_t** )( argv ) ) );
		CHECK( application::log_markers_option().is_set() );
		CHECK( application::log_markers_option().string_at_index( 0 ) == "nodeoze" );
		CHECK( application::unset_log_markers_option().is_set() );
		CHECK( application::unset_log_markers_option().string_at_index( 0 ) == "nodeoze" );
		CHECK( application::log_level_option().is_set() );
		CHECK( application::log_level_option().int_at_index( 0 ) == 10 );
		CHECK( application::log_throttle_option().is_set() );
		CHECK( application::log_throttle_option().int_at_index( 0 ) == 0 );
		CHECK( application::test_option().is_set() );
	}

	SUBCASE( "bad command line options" )
	{
		const std::tchar_t	*argv1[] =
		{
			ntext( "--log-markers" ),
		};
		
		int argc = sizeof( argv1 ) / sizeof( std::tchar_t* );
		
		CHECK( !application::parse_command_line( argc, ( std::tchar_t** )( argv1 ) ) );
		
		const std::tchar_t *argv2[] =
		{
			ntext( "--log-markers" ),
			ntext( "blob1" ),
			ntext( "blob2" ),
		};
		
		argc = sizeof( argv2 ) / sizeof( std::tchar_t* );
		
		CHECK( !application::parse_command_line( argc, ( std::tchar_t** )( argv2 ) ) );
	}

	SUBCASE( "check is_switch" )
	{
		CHECK( !application::is_switch( "" ) );
		CHECK( !application::is_switch( "-" ) );
		CHECK( !application::is_switch( "--" ) );
		CHECK( application::is_switch( "--a" ) );
	}

	SUBCASE( "variable options" )
	{
		application::option o( "--install", application::option::variable_size() );
		
		const std::tchar_t	*argv1[] =
		{
			ntext( "--log-markers" ),
			ntext( "blob1" ),
			ntext( "blob2" ),
			ntext( "--install" ),
			ntext( "--log-level" ),
			ntext( "2" ),
			ntext( "3" )
		};
		
		
		int argc = sizeof( argv1 ) / sizeof( std::tchar_t* );
		
		CHECK( !application::parse_command_line( argc, ( std::tchar_t** )( argv1 ) ) );
		CHECK( application::log_markers_option().is_set() );
		CHECK( application::log_markers_option().expected_size() == 1 );
		CHECK( application::log_markers_option() );
		CHECK( o.is_set() );
		CHECK( o.expected_size() == application::option::variable_size() );
		CHECK( o.is_variable_size() );
		CHECK( o.size() == 3 );
		CHECK( !application::log_level_option().is_set() );
	}
}

