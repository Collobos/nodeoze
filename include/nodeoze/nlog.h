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

#include <nodeoze/nsingleton.h>
#include <nodeoze/unordered_map.h>
#include <nodeoze/nevent.h>
#include <nodeoze/nprintf.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nstring.h>
#include <nodeoze/ntime.h>
#include <nodeoze/nuri.h>
#include <sstream>
#include <iomanip>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <set>
#include <assert.h>
#include <map>

#if defined( WIN32 )

#	include <winsock2.h>
#	include <windows.h>
#	include <stdio.h>
#	define nlog( LEVEL, MESSAGE, ... ) if ( LEVEL <= nodeoze::log::shared().level() ) { try { nodeoze::log::shared().put( LEVEL, __FILE__, __FUNCTION__, __LINE__, MESSAGE, __VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s:%d\n", __FUNCTION__, __LINE__ ); } }

#	define mlog( MARKER, LEVEL, MESSAGE, ...) {	static_assert(std::is_convertible<decltype(MARKER), nodeoze::log::marker>::value, "argument must be type log::marker"); if (MARKER || LEVEL <= nodeoze::log::level_t::warning ) { nlog ( LEVEL, MESSAGE, __VA_ARGS__ ); } }

#elif defined( __clang__ )

#	define nlog( LEVEL, MESSAGE, ... ) if ( LEVEL <= nodeoze::log::shared().level() ) { try { nodeoze::log::shared().put( LEVEL, __FILE__, __PRETTY_FUNCTION__, __LINE__, MESSAGE, ##__VA_ARGS__ ); } catch ( ... ) { fprintf( stderr, "logging exception at %s:%d\n", __PRETTY_FUNCTION__, __LINE__ ); } };

#	define mlog( MARKER, LEVEL, MESSAGE, ...) {	static_assert(std::is_convertible<decltype(MARKER), nodeoze::log::marker>::value, "argument must be type log::marker"); if (MARKER || LEVEL <= nodeoze::log::level_t::warning ) { nlog ( LEVEL, MESSAGE, ##__VA_ARGS__ ); } }

#endif

namespace nodeoze {

class any;

class log_marker;

class log : public event::emitter<>
{
	NODEOZE_DECLARE_SINGLETON( log )

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

	class sink
	{
	public:

		static std::shared_ptr< sink >
		console();

		static std::shared_ptr< sink >
		system();

		sink();

		virtual ~sink();

		virtual void
		put( level_t level, std::chrono::system_clock::time_point when, std::uint32_t pid, std::uint32_t tid, const std::string &file, const std::string &func, std::uint32_t line, const std::string &message ) = 0;

	protected:

		std::string
		prune_filename( const std::string &filename );

		std::string
		prune_function( const std::string &function );
	};


	typedef std::function< void ( level_t l ) > set_f;
	
	class marker
	{
	private:

		struct poison_constructor {};

	public:

		friend class log;

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
			
			const std::string&
			get_path()
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

			std::string										m_name;
			ptr												m_parent;
			mutable unordered_map<std::string, log::marker>	m_children;
			std::string										m_path;
			bool											m_path_created;
			bool											m_enabled;
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
		:
			marker(_impl)
		{
		}
	
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
		
	private:

	};

	log();

	~log();

	inline const std::string&
	name() const
	{
		return m_name;
	}

	inline void
	set_name( const std::string &name )
	{
		m_name = name;
	}
	
	inline level_t
	level()
	{
		return m_level;
	}

	void
	set_level( level_t l );

	log::marker&
	root_marker();

	void
	enable(log::marker& marker)
	{
		marker.enable();
	}
	
	void
	disable(log::marker& marker)
	{
		marker.disable();
	}

	log::marker&
	get_marker( const log::marker& parent, const std::string& name)
	{
		auto segments = nodeoze::string::split(name, '.');
		return const_cast< log::marker& >( log::marker::marker_impl::get_marker_internal(parent, segments) );
	}
	
	log::marker&
	get_marker( const log::marker& parent, const std::string& name, bool enabled)
	{
		log::marker& marker = get_marker(parent, name);
		return marker.set_state(enabled);
	}

	log::marker&
	get_marker(const std::string& name)
	{
		return get_marker(root_marker(), name);
	}

	log::marker&
	get_marker(const std::string& name, bool enabled)
	{
		log::marker& marker = get_marker(root_marker(), name);
		return marker.set_state(enabled);
	}

	log::marker&
	get_marker(const log::marker& parent, const std::vector<std::string> &marker_path)
	{
		return const_cast< log::marker& >( log::marker::marker_impl::get_marker_internal(parent, marker_path ) );
	}

	log::marker&
	get_marker(const log::marker& parent, const std::vector<std::string> &marker_path, bool enabled)
	{
		return get_marker(parent, marker_path).set_state(enabled);
	}

	log::marker&
	get_marker(const std::vector<std::string> &marker_path)
	{
		return const_cast< log::marker& >( log::marker::marker_impl::get_marker_internal(root_marker(), marker_path ) );
	}

	log::marker&
	get_marker(const std::vector<std::string> &marker_path, bool enabled)
	{
		return get_marker(root_marker(), marker_path).set_state(enabled);
	}

	bool
	update_marker_states(const any& root);

	void
	save_marker_states(any& root);
	
	
	template< typename ...Params >
	void
	put( level_t l, const char *filename, const char *function, int line, const char *format, const Params &... params )
	{
		std::ostringstream os;
		
//		os << getpid() << " " << std::to_string( std::chrono::system_clock::now() ) << " " << prune_filename( filename ) << ":" << line << " " << prune_function( function ) << " ";
			
		nodeoze::printf( os, format, params... );
		
		put( l, filename, function, line, os );
	}
	
	void
	put( level_t l, const std::string &filename, const std::string &function, std::uint32_t line, std::ostringstream &os );
	
#if 0
	template< typename ...Params >
	void
	put( const char *filename, const char *function, int line, const char *format, const Params &... params )
	{
		std::ostringstream os;
		
		os << getpid() << " " << std::to_string( std::chrono::system_clock::now() ) << " " << prune_filename( filename ) << ":" << line << " " << prune_function( function ) << " ";
			
		nodeoze::printf( os, format, params... );
		
		put( os );
	}
#endif
	
	inline void
	add_sink( std::shared_ptr< sink > s )
	{
		sinks().emplace_back( s );
	}
	
protected:

	typedef std::vector< std::shared_ptr< sink > > sinks_type;

	ndefine_global_static( sinks, sinks_type );

	level_t					m_level;
	std::recursive_mutex	m_mutex;
	std::string				m_name;
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

inline std::ostream&
operator<<( std::ostream &os, nodeoze::log::level_t l )
{
	switch ( l )
	{
		case nodeoze::log::level_t::info:
		{
			os << std::setw( 10 ) << "INFO";
		}
		break;
				
		case nodeoze::log::level_t::warning:
		{
			os << std::setw( 10 ) << "WARNING";
		}
		break;
				
		case nodeoze::log::level_t::error:
		{
			os << std::setw( 10 ) << "ERROR";
		}
		break;
				
		case nodeoze::log::level_t::verbose:
		{
			os << std::setw( 10 ) << "VERBOSE";
		}
		break;
				
		case nodeoze::log::level_t::voluminous:
		{
			os << std::setw( 10 ) << "VOLUMINOUS";
		}
		break;
				
		case nodeoze::log::level_t::debug:
		{
			os << std::setw( 10 ) << "DEBUG";
		}
		break;
	}
	
	return os;
}

#endif
