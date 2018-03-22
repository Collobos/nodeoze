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
 
#ifndef _nodeoze_application_h
#define _nodeoze_application_h

#include <nodeoze/nany.h>
#include <nodeoze/nstring.h>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>


namespace nodeoze {

class application
{
public:

	typedef std::function< void ( std::error_code err ) > exit_f;

	static bool
	parse_command_line( int argc, std::tchar_t **argv );
	
	static bool
	is_switch( const std::string &param )
	{
		return ( ( param.size() > 2 ) && ( param[ 0 ] == '-' ) && ( param[ 1 ] == '-' ) );
	}

	class option
	{
	public:
	
		static constexpr std::int32_t variable_size()
		{
			return std::numeric_limits< std::int32_t >::max();
		}
	
		struct setup
		{
			setup( std::function< void () > func )
			{
				func();
			}
		};

		typedef std::shared_ptr< option > ptr;
		
		static std::unordered_map< std::string, option* >&
		instances()
		{
			if ( !m_instances )
			{
				m_instances = new std::unordered_map< std::string, option* >();
			}
			
			return *m_instances;
		}
		
		option( const std::string &name, std::int32_t expected_size )
		:
			m_name( name ),
			m_expected_size( expected_size )
		{
			instances().emplace( name, this );
		}
		
		inline const std::string&
		name() const
		{
			return m_name;
		}
	
		inline std::size_t
		expected_size() const
		{
			return m_expected_size;
		}
		
		inline bool
		is_variable_size() const
		{
			return ( m_expected_size == variable_size() );
		}
		
		inline std::size_t
		size() const
		{
			return m_values.size();
		}
		
		inline bool
		is_set() const
		{
			return m_set;
		}
		
		inline void
		set_is_set( bool val )
		{
			m_set = val;
		}

		inline void
		clear()
		{
			m_set = false;
			m_values.clear();
		}

		inline const std::string&
		string_at_index( std::size_t index )
		{
			static const std::string null_string;
			
			return ( index < m_values.size() ) ? m_values[ index ] : null_string;
		}

		inline int
		int_at_index( std::size_t index )
		{
			return ( index < m_values.size() ) ? std::stoi( m_values[ index ] ) : 0;
		}
		
		inline bool
		push_back( const std::string &value )
		{
			bool ok = false;
			
			if ( m_values.size() < static_cast< std::size_t >( m_expected_size ) )
			{
				m_values.push_back( value );
				ok = true;
			}
			
			return ok;
		}
		
		inline explicit operator bool () const
		{
			return ( is_variable_size() || ( m_values.size() == m_expected_size ) );
		}
			
	private:

		static std::unordered_map< std::string, option* >	*m_instances;
		std::string											m_name;
		bool												m_set			= false;
		std::size_t											m_expected_size	= 0u;
		std::vector< std::string >							m_values;
	};
	
	ndefine_global_static( log_markers_option,			option, "--log-markers",		1 );
	ndefine_global_static( unset_log_markers_option,	option, "--unset-log-markers",	1 );
	ndefine_global_static( log_level_option,			option, "--log-level",			1 );
	ndefine_global_static( version_option,				option, "--version",			0 );
	ndefine_global_static( test_option,					option, "--test",				0 );
	
	static inline void
	init_options()
	{
		log_markers_option();
		unset_log_markers_option();
		log_level_option();
		version_option();
		test_option();
	}
	
	application( const std::string &name );

	virtual ~application() = 0;
	
	inline static application*
	_shared()
	{
		return m_shared;
	}
	
	template < class T >
	inline typename T::ptr
	lookup_option( const std::string &name )
	{
		auto			it = m_options.find( name );
		typename T::ptr opt;
	
		if ( it != m_options.end() )
		{
			opt = std::dynamic_pointer_cast< T >( *it );
		}
		else
		{
			opt = std::make_shared< T >();
		}
		
		return opt;
	}
	
	inline bool
	is_okay() const
	{
		return m_okay;
	}
	
	inline const std::string&
	name()
	{
		return m_name;
	}

	virtual void
	run( exit_f func ) = 0;

	std::int32_t
	run_tests();

protected:

	static application					*m_shared;
	bool								m_okay = true;
	std::string							m_name;
	
private:

	char**
	normalize_argv( int argc, std::tchar_t **argv );
	
	static bool
	is_doctest_option( const std::string &option );

	typedef std::unordered_map< std::string, option::ptr > options;
	
	options m_options;
};

}

#endif
