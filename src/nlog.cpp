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
 
#include <nodeoze/nlog.h>
#include <nodeoze/nprocess.h>
#include <nodeoze/nsocket.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/ntls.h>
#include <nodeoze/nws.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <mutex>
#include <nodeoze/nany.h>

using namespace nodeoze;

#if defined( DEBUG )
limiter													log::m_limiter( 0.0, 1.0 );
#else
limiter													log::m_limiter( 100.0, 1.0 );
#endif
log::level_t											log::m_level = log::level_t::info;
std::recursive_mutex									*log::m_mutex;
std::string												log::m_name;
uri														log::m_rlog;

static ip::tcp::socket			*g_socket		= nullptr;
static std::queue< buffer >		*g_buffers;
static bool						g_connecting	= false;

int
log::getpid()
{
	return process::self().pid();
}

static void
setup_socket( const uri &resource, const std::string &message, std::recursive_mutex *mutex )
{
	if ( resource )
	{
		std::lock_guard< std::recursive_mutex > lock( *mutex );
		
		if ( g_buffers == nullptr )
		{
			g_buffers = new std::queue< buffer >();
		}
		
		g_buffers->emplace( message );
		
		if ( !g_connecting )
		{
			g_connecting = true;
			
			auto socket = new ip::tcp::socket();
			
			socket->push( tls::filter::create( role_t::client ) );
			socket->push( ws::filter::create_client( resource ) );
			
			socket->recv( [=]( std::error_code err, nodeoze::buffer &buf ) mutable
			{
				nunused( buf );
				
				if ( err )
				{
					std::lock_guard< std::recursive_mutex > lock( *mutex );
					
					if ( socket )
					{
						auto tmp = socket;
						g_socket = socket = nullptr;
					
						runloop::shared().dispatch( [=]() mutable
						{
							delete tmp;
						} );
					}
				}
			} );
			
			ip::address::resolve( resource.host() ).then( [=]( std::vector< ip::address > addresses ) mutable
			{
				if ( socket != nullptr )
				{
					if ( !addresses.empty() )
					{
						ip::endpoint endpoint( addresses[ 0 ], resource.port() );
						
						socket->connect( endpoint )
						.then( [=]() mutable
						{
							g_connecting = false;
							
							if ( socket != nullptr )
							{
								g_socket = socket;
									
								while ( !g_buffers->empty() )
								{
									g_socket->send( std::move( g_buffers->front() ) )
									.then( [=]() mutable
									{
									} );
										
									g_buffers->pop();
								}
							}
							else
							{
								std::lock_guard< std::recursive_mutex > lock( *mutex );
									
								auto tmp = socket;
								socket = nullptr;
							
								runloop::shared().dispatch( [=]() mutable
								{
									delete tmp;
								} );
							}
						} );
					}
				}
			},
			[=]( std::error_code err ) mutable
			{
				nunused( err );
			} );
		}
	}
}


#if 0
void
log::on_set( cookie::ref *cookie, set_f handler )
{
	std::lock_guard<std::recursive_mutex> lock( *m_mutex );
	
	if ( cookie )
	{
		*cookie = std::make_shared< nodeoze::cookie >( [=]( nodeoze::cookie::naked_ptr p )
		{
			std::lock_guard<std::recursive_mutex> lock( *m_mutex );
		
			for ( auto it = m_set_handlers->begin(); it != m_set_handlers->end(); it++ )
			{
				if ( it->first == p )
				{
					m_set_handlers->erase( it );
					break;
				}
			}
		} );

	   m_set_handlers->push_back( std::make_pair( cookie->get(), handler ) );
	}
	else
	{
	   m_set_handlers->push_back( std::make_pair( nullptr, handler ) );
	}
}
#endif


void
log::init_limiter()
{
	m_limiter.on_turned_on( []()
	{
		std::ostringstream os;
		os << process::self().pid() << " " << std::to_string( std::chrono::system_clock::now() ) << " throttling logs";
		put_console_log( make_pretty( level_t::warning, os.str() ) );
	} );
}


void
log::put( log::level_t l, std::ostringstream &os )
{
	if ( m_mutex )
	{
		std::lock_guard< std::recursive_mutex > lock( *m_mutex );
		std::string								message( os.str() );
		auto									ok = m_limiter.proceed();
		
		put_system_log( l, message );
		
		if ( ok )
		{
			auto pretty = make_pretty( l, message );
			put_console_log( pretty );
			invoke_sinks( l, pretty );
		}
	}
}


void
log::put( std::ostringstream &os )
{
	if ( m_mutex )
	{
		std::lock_guard< std::recursive_mutex > lock( *m_mutex );
		std::string								message( os.str() );
		
		if ( g_socket )
		{
			g_socket->send( nodeoze::buffer( message ) )
			.then( [=]() mutable
			{
			} );
		}
		else
		{
			setup_socket( m_rlog, message, m_mutex );
		}
	}
}


std::string
log::make_pretty( level_t l, const std::string &message )
{
	std::ostringstream os;
	
	switch ( l )
	{
		case log::level_t::info:
		{
			os << std::setw( 10 ) << "INFO" << " " << message;
		}
		break;
				
		case log::level_t::warning:
		{
			os << std::setw( 10 ) << "WARNING" << " " << message;
		}
		break;
				
		case log::level_t::error:
		{
			os << std::setw( 10 ) << "ERROR" << " " << message;
		}
		break;
				
		case log::level_t::verbose:
		{
			os << std::setw( 10 ) << "VERBOSE" << " " << message;
		}
		break;
				
		case log::level_t::voluminous:
		{
			os << std::setw( 10 ) << "VOLUMINOUS" << " " << message;
		}
		break;
				
		case log::level_t::debug:
		{
			os << std::setw( 10 ) << "DEBUG" << " " << message;
		}
		break;
	}
	
	return os.str();
}


void
log::set_level( log::level_t level )
{
	std::lock_guard<std::recursive_mutex> lock( *m_mutex );
	
	if ( m_level != level )
	{
		m_level = level;
		
/*
		notification::publish( notification::local, was_changed_event, nullptr, any::null() );
*/
	}
}

std::string
log::prune_filename( const char *filename )
{
	if ( filename && ( strlen( filename ) > 0 ) )
	{
		for ( auto i = strlen( filename ) - 1; i > 0; i-- )
		{
			if ( ( filename[ i ] == '/' ) || ( filename[ i ] == '\\' ) )
			{
				return filename + i + 1;
			}
		}
	}

	return filename;
}

std::string
log::prune_function( const char *function )
{
#if defined( __clang__ )

	std::string tmp( function );

    auto end    = tmp.find( '(' );
    auto begin  = tmp.rfind( ' ', end ) + 1;

    return tmp.substr( begin, end - begin ) + "()";
	
#else

	return function;
	
#endif
}


void
log::invoke_sinks( level_t level, const std::string &message )
{
	for ( auto it = sinks().begin(); it != sinks().end(); it++ )
	{
		if ( it->second.first )
		{
			it->second.first = false;
			it->second.second( level, message );
			it->second.first = true;
		}
	}
}


log::marker&
log::root_marker()
{
	static log::marker _root_marker(std::make_shared<log::marker::marker_impl>(log::marker::marker_impl::poison_constructor(), nullptr, "_root"));
	return _root_marker;
}

void
log::marker::deflate( any &array ) const
{
//	object::deflate( root );

	if (!this->m_impl->is_root())
	{
		nodeoze::any this_marker;
		this_marker["name"] = m_impl->get_path();
		this_marker["enabled"] = m_impl->m_enabled;
		array.emplace_back( std::move( this_marker ) );
		
	}

	for (auto it = m_impl->m_children.begin(); it != m_impl->m_children.end(); ++it)
	{
		it->second.deflate(array);
	}
}

bool
log::update_marker_states(const any& array)
{
	if (array.is_array())
	{
		for ( auto i = 0u; i < array.size(); ++i )
		{
			auto node = array[i];
			if (node.is_object())
			{
				if (node.is_member("name"))
				{
					auto path = node["name"].to_string();
					if (node.is_member("enabled"))
					{
						auto is_enabled = node["enabled"].to_bool();
						log::get_marker(path, is_enabled);
					}
					else
					{
						return false;
					}
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	
	}
	else
	{
		return false;
	}
	return true;
}

void
log::save_marker_states(any& root)
{
	// the set_type should clear any existing value (including an array) and construct an array
//	root.set_type(any::type_t::array);
	root_marker().deflate(root);
}


void
nrequire_failed_test()
{
}
