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
 
#ifndef _nodeoze_database_h
#define _nodeoze_database_h

#include <nodeoze/nevent.h>
#include <nodeoze/njson.h>
#include <nodeoze/ntypes.h>
#include <nodeoze/nprintf.h>
#include <nodeoze/nany.h>
#include <nodeoze/nuri.h>
#include <nodeoze/nstring.h>
#include <nodeoze/ntypestring.h>
#include <nodeoze/npath.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <atomic>

struct sqlite3;
struct sqlite3_stmt;
typedef struct Mem sqlite3_value;

namespace nodeoze {

class database : public event::emitter<>
{
public:

	class statement;

	typedef std::int64_t oid_type;

	enum class action_type : std::uint8_t
	{
		insert = 0,
		update = 1,
		remove = 2
	};

	typedef std::function< void ( action_type action, oid_type oid, statement &stmt ) >						continuous_select_handler_f;
	typedef std::function < std::string ( void ) >															backup_hook_f;
	typedef std::function < std::string ( void ) >															restore_hook_f;
	
	typedef std::vector< oid_type > oids;

	enum class errc
	{
		ok						= 0,
		error					= 1,
		internal_error			= 2,
		permission_denied		= 3,
		abort					= 4,
		busy					= 5,
		locked					= 6,
		out_of_memory			= 7,
		read_only				= 8,
		interrupt				= 9,
		io_error				= 10,
		corrupt					= 11,
		not_found				= 12,
		full					= 13,
		cannot_open				= 14,
		protocol_error			= 15,
		empty					= 16,
		schema_changed			= 17,
		too_big					= 18,
		constraint_violation	= 19,
		mismatch				= 20,
		misuse					= 21,
		no_lfs					= 22,
		auth_denied				= 23,
		format					= 24,
		range					= 25,
		not_a_db_file			= 26,
		notice					= 27,
		warning					= 28,
		row						= 100,
		done					= 101
	};

	static const std::error_category&
	error_category();

	static std::string
	sanitize( const std::string &str );

	static inline std::string
	each( const std::string &table, const std::string &key )
	{
		std::ostringstream os;
		
		os << "JSON_EACH( " << table << ".data, '$." << key << "')";
		
		return os.str();
	}

	static inline std::string
	extract( const std::string &key )
	{
		std::ostringstream os;
			
		os << "JSON_EXTRACT(data,'$." << key << "')";
			
		return os.str();
	}

	static inline std::string
	extract_each()
	{
		return "JSON_EACH.value";
	}
		
	static inline std::string
	value( const nodeoze::any &val )
	{
		std::stringstream os;
			
		os << "JSON( '" << database::sanitize( json::deflate_to_string( val ) ) << "' )";
			
		return os.str();
	}
			
	static inline std::string
	value( const std::string &val, bool quote = true )
	{
		return ( quote ? "'" : "" ) + database::sanitize( val ) + ( quote ? "'" : "" );
	}

	class statement
	{
	public:

		typedef int (*preupdate_getter_type )( sqlite3*, int, sqlite3_value** ); // type for conciseness

		statement( sqlite3_stmt *stmt = nullptr, bool finalize = true );

		statement( sqlite3 * db, preupdate_getter_type getter );

		statement( std::error_code err );

		statement( const statement &rhs )
		:
			m_shared( nullptr )
		{
			copy( rhs );
		}

		statement( statement &&rhs )
		:
			m_shared( nullptr )
		{
			move( std::move( rhs ) );
		}

		~statement()
		{
			unshare();
		}

		inline statement&
		operator=( const statement &rhs )
		{
			copy( rhs );

			return *this;
		}

		inline statement&
		operator=( statement &&rhs )
		{
			move( std::move( rhs ) );

			return *this;
		}

		void
		finalize();
		
		std::error_code
		last_error() const;
		
		bool
		step();
		
		std::error_code
		exec_prepared();

		std::error_code
		reset_prepared();

		bool
		bool_at_column( int col ) const;

		int
		int_at_column( int col ) const;

		std::int64_t
		int64_at_column( int col ) const;

		std::uint64_t
		uint64_at_column( int col ) const;
		
		std::chrono::system_clock::time_point
		time_at_column( int col ) const;

		std::string
		text_at_column( int col ) const;
		
		nodeoze::buffer
		blob_at_column( int col ) const;
		
		bool
		set_bool( bool value, const std::string& var_name );
		
		bool
		set_bool( bool value, int var_position );
		
		bool
		set_int( int value, const std::string& var_name );
		
		bool
		set_int( int value, int var_position );
		
		bool
		set_int64( std::int64_t value, const std::string& var_name );
		
		bool
		set_int64( std::int64_t value, int var_position );
		
		bool
		set_uint64( std::uint64_t value, const std::string& var_name );
		
		bool
		set_uint64( std::uint64_t value, int var_position );
		
		bool
		set_time( std::chrono::system_clock::time_point value, const std::string& var_name );
		
		bool
		set_time( std::chrono::system_clock::time_point value, int var_position );
		
		bool
		set_text( const std::string& value, const std::string& var_name );
		
		bool
		set_text( const std::string& value, int var_position );
		
		bool
		set_blob( nodeoze::buffer& value, const std::string& var_name );
		
		bool
		set_blob( nodeoze::buffer& value, int var_position );
		
		inline explicit operator bool () const
		{
			return ( m_shared->stmt ) ? true : false;
		}

	private:

		friend class database;

		inline void
		share( const statement &rhs )
		{
			m_shared = rhs.m_shared;

			if ( m_shared )
			{
				m_shared->refs++;
			}
		}

		inline void
		unshare()
		{
			if ( m_shared && ( --m_shared->refs == 0 ) )
			{
				if ( m_shared->stmt && m_shared->finalize )
				{
					finalize();
				}

				delete m_shared;
				m_shared = nullptr;
			}
		}

		void
		copy( const statement &rhs )
		{
			unshare();
			share( rhs );
		}

		void
		move( statement &&rhs )
		{
			unshare();

			m_shared		= rhs.m_shared;
			rhs.m_shared	= nullptr;
		}

		bool
		reentrant() const
		{
			auto ret = bool( false );

			if ( m_shared )
			{
				ret = m_shared->reentrant;
			}

			return ret;
		}

		void
		set_reentrant( bool val )
		{
			if ( m_shared )
			{
				m_shared->reentrant = val;
			}
		}

		struct shared
		{
			sqlite3_stmt			*stmt;
			bool					finalize;
			bool					reentrant;
			sqlite3					*db;
			preupdate_getter_type	getter;
			std::error_code			err;
			std::size_t				refs;
		} *m_shared;
	};

	database();

	database( const std::string &db );

	~database();
	
	std::error_code
	open( const std::string &db );
	
	bool
	is_open() const;
	
	std::error_code
	close();
	
	template< typename ...Params >
	std::error_code
	exec( const char *format, const Params &... params )
	{
		std::ostringstream	os;
			
		nodeoze::printf( os, format, params... );
		
		return exec( os.str() );
	}

	std::error_code
	exec( const std::string &str );

	template< typename ...Params >
	statement
	select( const char *format, const Params &... params )
	{
		std::ostringstream os;
			
		nodeoze::printf( os, format, params... );
		
		return select( os.str() );
	}
	
	statement
	select( const std::string &str );
	
	template < class T >
	statement
	select()
	{
		return select( "SELECT * FROM %;", T::table_name() );
	}

	template< class T >
	statement
	select( oid_type oid )
	{
		return select( "SELECT * FROM % WHERE oid = %;", T::table_name(), oid );
	}

	template< class T >
	statement
	select( const std::string &key, bool val )
	{
		return select( "SELECT * FROM % WHERE % = %;", T::table_name(), key, val );
	}

	template< class T >
	statement
	select( const std::string &key, std::uint64_t val )
	{
		return select( "SELECT * FROM % WHERE % = %;", T::table_name(), key, val );
	}

	template < class T >
	statement
	select( const std::string &key, const char *val )
	{
		return select( "SELECT * FROM % WHERE % LIKE '%';", T::table_name(), key, sanitize( val ) );
	}

	template < class T >
	statement
	select( const std::string &key, const std::string &val )
	{
		return select( "SELECT * FROM % WHERE % LIKE '%';", T::table_name(), key, sanitize( val ) );
	}
	
	template< typename ...Params >
	statement
	prepare( const char *format, const Params &... params )
	{
		std::ostringstream os;
			
		nodeoze::printf( os, format, params... );
		
		return prepare( os.str() );
	}
	
	statement
	prepare( const std::string &str );
	
	template< class F >
	std::error_code
	prepare( const std::string &str, F func )
	{
		mlog( marker::database, log::level_t::info, "%", str );
	
		auto err	= std::error_code();
		auto it		= m_prepared_statement_map.find( str );
		
		if ( it == m_prepared_statement_map.end() )
		{
			it = tracked_prepared_statement( str );
		}

		if ( it != m_prepared_statement_map.end() )
		{
			if ( !it->second.reentrant() )
			{
				it->second.set_reentrant( true );
				err = func( it->second );
				it->second.set_reentrant( false );
				it->second.reset_prepared();
			}
			else
			{
				auto stmt = transient_prepared_statement( str );
				err = func( stmt );
			}
		}
		else
		{
			err = make_error_code( std::errc::invalid_argument );
		}
		
		return err;
	}

	void
	reset_all_prepared();
	
	nodeoze::scoped_operation
	continuous_select( const std::string &columns, const std::string &from, const std::string &where, continuous_select_handler_f handler );
	
	std::error_code
	start_transaction();
	
	std::error_code
	end_transaction();
	
	std::error_code
	cancel_transaction();
	
	template< class F >
	std::error_code
	transaction( F func )
	{
		auto err = start_transaction();
		ncheck_error( !err, exit, "% (%)", err, err.message() );
		err = func();
		if ( !err )
		{
			end_transaction();
		}
		else
		{
			cancel_transaction();
		}
		
		ncheck_error( !err, exit, "% (%)", err, err.message() );

	exit:
	
		return err;
	}
	
	std::size_t
	count( const std::string &table_name )
	{
		statement	stmt;
		int			rows = 0;
		
		stmt = select( "SELECT COUNT(*) FROM %;", table_name );
			
		if ( stmt.step() )
		{
			rows = stmt.int_at_column( 0 );
		}
			
		return static_cast< std::size_t >( rows );
	}
		
	std::uint32_t
	version() const;

	void
	set_version( std::uint32_t version );
	
	std::error_code
	backup( const std::string &db, backup_hook_f hook );

	std::error_code
	restore( const std::string &db, backup_hook_f hook );
	
	std::error_code
	backup_database( const nodeoze::path& backup_path, const std::vector<std::string>& statements, std::size_t step_size, int sleep_ms, std::atomic<bool>& exiting );
	
	oid_type
	last_oid() const;

	std::error_code
	last_error();
	
	void
	clear_error();

private:

	std::unordered_map< std::string, statement >::iterator
	tracked_prepared_statement( const std::string &str );
	
	statement
	transient_prepared_statement( const std::string &str );

	static void
	on_preupdate( void *v, sqlite3 *db, int op, char const *z_db, char const *z_name, std::int64_t key1, std::int64_t key2 );
	
	static void
	log_callback( void *p_arg, int code, const char *msg );

	sqlite3											*m_db;
	oid_type										m_last_oid;
	std::error_code									m_err;
	
	std::unordered_map< std::string, statement >	m_prepared_statement_map;
	deque< database::statement >					m_prepared_statements;
};

inline std::ostream&
operator<<(std::ostream &os, const nodeoze::database::action_type action )
{
	switch ( action )
	{
		case nodeoze::database::action_type::insert:
		{
			os << "insert";
		}
		break;
	
		case nodeoze::database::action_type::update:
		{
			os << "update";
		}
		break;
	
		case nodeoze::database::action_type::remove:
		{
			os << "remove";
		}
		break;
		
	}
	
	return os;
}

inline std::error_code
make_error_code( nodeoze::database::errc val )
{
	return std::error_code( static_cast< int >( val ), nodeoze::database::error_category() );
}

}

namespace std
{
	template<>
	struct is_error_code_enum< nodeoze::database::errc > : public std::true_type {};
}

#endif
