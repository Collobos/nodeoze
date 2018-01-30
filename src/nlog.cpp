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
#include <nodeoze/nthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <mutex>
#include <nodeoze/nany.h>

using namespace nodeoze;

NODEOZE_DEFINE_SINGLETON( nodeoze::log )

nodeoze::log*
log::create()
{
	return new log;
}


log::log()
:
	m_level( level_t::info )
{
}


log::~log()
{
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
log::put( level_t l, const std::string &filename, const std::string &function, std::uint32_t line, std::ostringstream &os )
{
	std::lock_guard< std::recursive_mutex > lock( m_mutex );
	auto									now = std::chrono::system_clock::now();
	auto									pid = process::self().pid();
	auto									tid = thread::id();
	std::string								message( os.str() );

	for ( auto &sink : sinks() )
	{
		sink->put( l, now, pid, tid, filename, function, line, message );
	}
}


void
log::set_level( log::level_t level )
{
	std::lock_guard<std::recursive_mutex> lock( m_mutex );
	
	if ( m_level != level )
	{
		m_level = level;
		
/*
		notification::publish( notification::local, was_changed_event, nullptr, any::null() );
*/
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
	root_marker().deflate(root);
}

#if defined( __APPLE__ )
#	pragma mark log::sink implementation
#endif

log::sink::sink()
{
}


log::sink::~sink()
{
}


std::string
log::sink::prune_filename( const std::string &filename )
{
	if ( filename.size() > 0 )
	{
		for ( auto i = filename.size() - 1; i > 0; i-- )
		{
			if ( ( filename[ i ] == '/' ) || ( filename[ i ] == '\\' ) )
			{
				return filename.c_str() + i + 1;
			}
		}
	}

	return filename;
}


std::string
log::sink::prune_function( const std::string &function )
{
#if defined( __clang__ )

	auto end    = function.find( '(' );
	auto begin  = function.rfind( ' ', end ) + 1;

	return function.substr( begin, end - begin ) + "()";
	
#else

	return function;
	
#endif
}


std::shared_ptr< log::sink >
log::sink::console()
{
	class console : public log::sink
	{
	public:

		console()
		{
		}

		virtual ~console()
		{
		}

		virtual void
		put( level_t level, std::chrono::system_clock::time_point when, std::uint32_t pid, std::uint32_t tid, const std::string &file, const std::string &func, std::uint32_t line, const std::string &message )
		{
			std::cerr << level << " " << pid << ":" << tid << " " << std::to_string( when ) << " " << prune_filename( file ) << ":" << line << " " << prune_function( func ) << " " << message << std::endl;
		}
	};

	return std::make_shared< console >();
}


void
nrequire_failed_test()
{
}
