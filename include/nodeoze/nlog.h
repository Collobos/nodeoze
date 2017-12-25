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
 
#ifndef _nodeoze_log_h
#define _nodeoze_log_h

#include <nodeoze/nprintf.h>
#include <nodeoze/nstring.h>
#include <nodeoze/nlimiter.h>
#include <nodeoze/ntime.h>
#include <nodeoze/nuri.h>
#include <sstream>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <assert.h>
#include <map>

#if defined( WIN32 )

#	include <winsock2.h>
#	include <windows.h>
#	include <stdio.h>
#	define nlog( LEVEL, MESSAGE, ... ) if ( LEVEL <= nodeoze::log::level() ) { try { nodeoze::log::put( LEVEL, __FILE__, __FUNCTION__, __LINE__, MESSAGE, __VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s:%d\n", __FUNCTION__, __LINE__ ); } }
#	if defined( DEBUG )
#		define ndlog( MESSAGE, ... ) if ( nodeoze::log::level_t::debug <= nodeoze::log::level() ) { try { nodeoze::log::put( nodeoze::log::level_t::debug, __FILE__, __FUNCTION__, __LINE__, MESSAGE, __VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s:%d\n", __FUNCTION__, __LINE__ ); } }
#	else
#		define ndlog( MESSAGE, ... )
#	endif

#	define mlog( MARKER, LEVEL, MESSAGE, ...) {	static_assert(std::is_convertible<decltype(MARKER), nodeoze::log::marker>::value, "argument must be type log::marker"); if (MARKER || LEVEL <= nodeoze::log::level_t::warning ) { nlog ( LEVEL, MESSAGE, __VA_ARGS__ ); } }

#	define rlog( MESSAGE, ...) { try { nodeoze::log::put( __FILE__, __FUNCTION__, __LINE__, MESSAGE, __VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s%d\n", __FUNCTION__, __LINE__ ); } }

#elif defined( __clang__ )

#	define nlog( LEVEL, MESSAGE, ... ) if ( LEVEL <= nodeoze::log::level() ) { try { nodeoze::log::put( LEVEL, __FILE__, __PRETTY_FUNCTION__, __LINE__, MESSAGE, ##__VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s:%d\n", __PRETTY_FUNCTION__, __LINE__ ); } };
#	if defined( DEBUG )
#		define ndlog( MESSAGE, ... ) if ( nodeoze::log::level_t::debug <= nodeoze::log::level() ) { try { nodeoze::log::put( nodeoze::log::level_t::debug, __FILE__, __PRETTY_FUNCTION__, __LINE__, MESSAGE, ##__VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s:%d\n", __PRETTY_FUNCTION__, __LINE__ ); } };
#	else
#		define ndlog( MESSAGE, ... )
#	endif

#	define mlog( MARKER, LEVEL, MESSAGE, ...) {	static_assert(std::is_convertible<decltype(MARKER), nodeoze::log::marker>::value, "argument must be type log::marker"); if (MARKER || LEVEL <= nodeoze::log::level_t::warning ) { nlog ( LEVEL, MESSAGE, ##__VA_ARGS__ ); } }

#	define rlog( MESSAGE, ...) { try { nodeoze::log::put( __FILE__, __PRETTY_FUNCTION__, __LINE__, MESSAGE, ##__VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s%d\n", __PRETTY_FUNCTION__, __LINE__ ); } }

#endif

namespace nodeoze {

class any;

class log_marker;

class log
{
public:

	enum class level_t
	{
		error			= 1,
		warning			= 2,
		info			= 3,
		verbose			= 10,
		voluminous		= 20,
		debug			= 30
	};
	
	typedef std::function< void ( level_t level, const std::string& message ) > sink_f;
	typedef std::function< void ( level_t l ) >									set_f;
	
	static const std::string change_event;

	class marker
	{
	friend class log;
	private:
		struct poison_constructor {};

	public:

		class marker_impl;

		class marker_impl
		{
		friend class marker;
		friend class log;
		
		private:
			struct poison_constructor {};
			
		protected:
		
			typedef std::shared_ptr<marker_impl> ptr;
			
		public:
			marker_impl(const poison_constructor&, ptr _parent, const std::string& _name, bool _enabled = true)
			: marker_impl(_parent, _name, _enabled)
			{
			}

		protected:
		
			marker_impl() = delete;
			
			marker_impl(bool _enabled) : m_enabled(_enabled) { }
		
			marker_impl(const marker_impl& rhs) = delete;
			
			marker_impl(marker_impl&& rhs) = delete;
			
			marker_impl& operator=(const marker_impl& rhs) = delete;
			
			marker_impl& operator=(marker_impl&& rhs) = delete;
			
			marker_impl(ptr _parent, const std::string& _name, bool _enabled = true)
			:
			m_name(_name),
			m_parent(_parent),
			m_path(),
			m_path_created(false),
			m_enabled(_enabled)
			{}
			
			static log::marker&
			get_marker_internal( const log::marker& parent, const std::string& name)
			{
				auto _parent = parent.m_impl;
				auto it = _parent->m_children.find(name);
				if (it != _parent->m_children.end())
				{
					return _parent->m_children.at(name);
				}
				else
				{
					auto result = _parent->m_children.emplace( std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(log::marker::poison_constructor(), std::make_shared<marker_impl>(poison_constructor(), _parent, name)));
					((void)(result));
					assert( result.second );

					return _parent->m_children.at(name);
				}
			}
			
			static const log::marker&
			get_marker_internal( const log::marker& parent, const std::vector<std::string> marker_path)
			{
				std::vector<std::string> cleaned;
				for (auto &name : marker_path)
				{
					if (name.length() > 0)
					{
						cleaned.push_back(name);
					}
				}
				if (cleaned.size() < 1)
				{
					return parent;
				}
				else
				{
					log::marker next_parent = parent;
					auto i = 0u;
					while (i < cleaned.size() - 1)
					{
						next_parent = marker_impl::get_marker_internal(next_parent, cleaned[i++]);
					}
					return marker_impl::get_marker_internal(next_parent, cleaned[i]);
				}
			}

			void
			set_state(bool enabled)
			{
				m_enabled = enabled;
				for (auto it = m_children.begin(); it != m_children.end(); ++it)
				{
					it->second.m_impl->set_state(enabled);
				}
			}
			
			void
			enable()
			{
				set_state(true);
			}
				
			void
			disable()
			{
				set_state(false);
			}
			
			const std::string& get_path()
			{
				if (!m_path_created)
				{
					if (is_root())
					{
						m_path = "";
					}
					else if (m_parent->is_root())
					{
						m_path.append(m_name);
					}
					else
					{
						m_path = m_parent->get_path();
						m_path.append(".").append(m_name);
					}
					m_path_created = true;
				}
				return m_path;
			}
			
			bool
			is_root()
			{
				return !m_parent;
			}

			std::string												m_name;
			ptr														m_parent;
			mutable std::unordered_map<std::string, log::marker>	m_children;
			std::string												m_path;
			bool													m_path_created;
			bool													m_enabled;
		};

	protected:
		marker() = delete;
		
		typedef std::shared_ptr<marker_impl> marker_impl_ptr;
	
		marker(marker_impl_ptr _impl) : m_impl(_impl)
		{ }
	
		marker_impl_ptr m_impl;

		void
		deflate( any &node ) const;
		
	public:
	
		marker(const poison_constructor&, marker_impl_ptr _impl)
		: marker(_impl)
		{ }
	
		inline explicit operator bool() const
		{
			return m_impl->m_enabled;
		}
		
		log::marker&
		enable()
		{
			return set_state(true);
		}
		
		log::marker&
		disable()
		{
			return set_state(false);
		}
		
		const std::string&
		get_path()
		{
			return m_impl->get_path();
		}
		
		log::marker&
		set_state(bool enabled)
		{
			m_impl->set_state(enabled);
			return *this;
		}
		
	};

	static log::marker&
	root_marker();
	
	static void
	enable(log::marker& marker)
	{
		marker.enable();
	}
	
	static void
	disable(log::marker& marker)
	{
		marker.disable();
	}

	static log::marker&
	get_marker( const log::marker& parent, const std::string& name)
	{
		auto segments = nodeoze::string::split(name, '.');
		return const_cast< log::marker& >( log::marker::marker_impl::get_marker_internal(parent, segments) );
	}
	
	static log::marker&
	get_marker( const log::marker& parent, const std::string& name, bool enabled)
	{
		log::marker& marker = get_marker(parent, name);
		return marker.set_state(enabled);
	}

	static log::marker&
	get_marker(const std::string& name)
	{
		return get_marker(root_marker(), name);
	}

	static log::marker&
	get_marker(const std::string& name, bool enabled)
	{
		log::marker& marker = get_marker(root_marker(), name);
		return marker.set_state(enabled);
	}

	static log::marker&
	get_marker(const log::marker& parent, const std::vector<std::string> &marker_path)
	{
		return const_cast< log::marker& >( log::marker::marker_impl::get_marker_internal(parent, marker_path ) );
	}

	static log::marker&
	get_marker(const log::marker& parent, const std::vector<std::string> &marker_path, bool enabled)
	{
		return get_marker(parent, marker_path).set_state(enabled);
	}

	static log::marker&
	get_marker(const std::vector<std::string> &marker_path)
	{
		return const_cast< log::marker& >( log::marker::marker_impl::get_marker_internal(root_marker(), marker_path ) );
	}

	static log::marker&
	get_marker(const std::vector<std::string> &marker_path, bool enabled)
	{
		return get_marker(root_marker(), marker_path).set_state(enabled);
	}

	static bool
	update_marker_states(const any& root);

	static void
	save_marker_states(any& root);
	
	static void
	init( const std::string &name );
	
	static void
	cleanup( const std::string &name );
	
	inline static void
	set_rlog( const nodeoze::uri &resource )
	{
		m_rlog = resource;
	}

	inline static level_t
	level()
	{
		return m_level;
	}

	static void
	set_level( level_t l );
	
	inline static double
	throttle()
	{
		return m_limiter.rate();
	}
	
	inline static void
	set_throttle( double val )
	{
		m_limiter.set_rate( val );
	}
	
	template< typename ...Params >
	static void
	put( level_t l, const char *filename, const char *function, int line, const char *format, const Params &... params )
	{
		std::ostringstream os;
		
		os << getpid() << " " << std::to_string( std::chrono::system_clock::now() ) << " " << prune_filename( filename ) << ":" << line << " " << prune_function( function ) << " ";
			
		nodeoze::printf( os, format, params... );
		
		put( l, os );
	}
	
	static void
	put( level_t l, std::ostringstream &os );
	
	template< typename ...Params >
	static void
	put( const char *filename, const char *function, int line, const char *format, const Params &... params )
	{
		std::ostringstream os;
		
		os << getpid() << " " << std::to_string( std::chrono::system_clock::now() ) << " " << prune_filename( filename ) << ":" << line << " " << prune_function( function ) << " ";
			
		nodeoze::printf( os, format, params... );
		
		put( os );
	}
	
	static void
	put( std::ostringstream &os );
	
	static inline void
	add_sink( const std::string &name, sink_f sink )
	{
		sinks()[ name ] = std::make_pair( true, sink );
	}
	
	static inline void
	remove_sink( const std::string &name )
	{
		auto it = sinks().find( name );
		
		if ( it != sinks().end() )
		{
			sinks().erase( it );
		}
	}
	
protected:

	static std::map< std::string, std::pair< bool, sink_f > >&
	sinks()
	{
		static auto global = new std::map< std::string, std::pair< bool, sink_f > >();
		return *global;
	}
	
	static int
	getpid();

	static void
	init_limiter();
	
	static void
	put_system_log( level_t l, const std::string &message );
	
	static void
	put_console_log( const std::string &message );
	
	static std::string
	make_pretty( level_t l, const std::string &message );

	static std::string
	prune_filename( const char *filename );

	static std::string
	prune_function( const char *filename );
	
	static void
	invoke_sinks( level_t level, const std::string &message );
	
	static limiter												m_limiter;
	static level_t												m_level;
	static std::recursive_mutex									*m_mutex;
	static std::string											m_name;
	static nodeoze::uri											m_rlog;
};

}

namespace std {

inline ostream&
operator<<( ostream &os, const vector< string > &strings )
{
	auto first = true;
	
	os << "{ ";
	
	for ( auto &s : strings )
	{
		if ( !first )
		{
			os << ", ";
		}
		
		os << s;
		
		first = false;
	}
	
	return os << " }";
}


inline ostream&
operator<<( ostream &os, const unordered_set< string > &strings )
{
	auto first = true;
	
	os << "{ ";
	
	for ( auto &s : strings )
	{
		if ( !first )
		{
			os << ", ";
		}
		
		os << s;
		
		first = false;
	}
		
	return os << " }";
}


inline ostream&
operator<<( ostream &os, const set< string > &strings )
{
	auto first = true;
	
	os << "{ ";
	
	for ( auto &s : strings )
	{
		if ( !first )
		{
			os << ", ";
		}
		
		os << s;
		
		first = false;
	}
		
	return os << " }";
}

}

#endif
