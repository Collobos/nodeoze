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

#include <nodeoze/njson.h>
#include <nodeoze/ntypes.h>
#include <nodeoze/nsingleton.h>
#include <nodeoze/nprintf.h>
#include <nodeoze/nany.h>
#include <nodeoze/nuri.h>
#include <nodeoze/nstring.h>
#include <nodeoze/nerror.h>
#include <nodeoze/ntypestring.h>
#include <nodeoze/nscoped_operation.h>
#include <nodeoze/npath.h>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <atomic>


#define DECLARE_PERSISTENT_OBJECT( NAME, TABLENAME )			\
static const std::string&										\
table_name()													\
{																\
	static std::string name( #TABLENAME );						\
	return name;												\
}																\
virtual const std::string&										\
table_name_v() const											\
{																\
	return table_name();										\
}																\
static void														\
clear()															\
{																\
	return nodeoze::database::object::clear<NAME>();			\
}																\
static bool initialize();										\
virtual bool save( bool force = false ) const;

namespace nodeoze {

namespace database {

inline const std::string&
raw_change_event()
{
	static const std::string event_name = "raw_change_event";
	return event_name;
}

typedef std::vector< oid_t > oids;

enum class action_t : std::uint8_t
{
	insert = 0,
	update = 1,
	remove = 2
};

enum code_t
{
	ok						=    0,
	error					=    1,
	internal_error			=	2,
	permission_denied		=	3,
	abort					=	4,
	busy					=	5,
	locked					=	6,
	out_of_memory			=	7,
	read_only				=	8,
	interrupt				=	9,
	io_error				=	10,
	corrupt					=	11,
	not_found				=	12,
	full					=	13,
	cannot_open				=	14,
	protocol_error			=	15,
	empty					=	16,
	schema_changed			=	17,
	too_big					=	18,
	constraint_violation	=	19,
	mismatch				=	20,
	misuse					=	21,
	no_lfs					=	22,
	auth_denied				=	23,
	format					=	24,
	range					=	25,
	not_a_db_file			=	26,
	notice					=	27,
	warning					=	28,
	row						=	100,
	done					=	101
};

extern std::string
sanitize( const std::string &str );

inline std::string
each( const std::string &table, const std::string &key )
{
	std::ostringstream os;
	
	os << "JSON_EACH( " << table << ".data, '$." << key << "')";
	
	return os.str();
}

inline std::string
extract( const std::string &key )
{
	std::ostringstream os;
		
	os << "JSON_EXTRACT(data,'$." << key << "')";
		
	return os.str();
}

inline std::string
extract_each()
{
	return "JSON_EACH.value";
}
	
inline std::string
value( const nodeoze::any &val )
{
	std::stringstream os;
		
	os << "JSON( '" << database::sanitize( json::deflate_to_string( val ) ) << "' )";
		
	return os.str();
}
		
static inline std::string
value( const std::string &val )
{
	return "'" + database::sanitize( val ) + "'";
}

class statement
{
public:

	typedef void ( statement::*safe_bool_type )();

	typedef std::shared_ptr< statement > ptr;
	
	virtual ~statement() = 0;

	virtual void
	finalize() = 0;
	
	virtual std::error_code
	last_error() const = 0;
	
	virtual bool
	step() = 0;
	
	virtual std::error_code
	exec_prepared() = 0;

	virtual std::error_code
	reset_prepared() = 0;

	virtual bool
	bool_at_column( int col ) const = 0;

	virtual int
	int_at_column( int col ) const = 0;

	virtual std::int64_t
	int64_at_column( int col ) const = 0;

	virtual std::uint64_t
	uint64_at_column( int col ) const = 0;
	
	virtual std::chrono::system_clock::time_point
	time_at_column( int col ) const = 0;

	virtual std::string
	text_at_column( int col ) const = 0;
	
	virtual nodeoze::buffer
	blob_at_column( int col ) const = 0;
	
	virtual bool
	set_bool( bool value, const std::string& var_name ) = 0;
	
	virtual bool
	set_bool( bool value, int var_position ) = 0;
	
	virtual bool
	set_int( int value, const std::string& var_name ) = 0;
	
	virtual bool
	set_int( int value, int var_position ) = 0;
	
	virtual bool
	set_int64( std::int64_t value, const std::string& var_name ) = 0;
	
	virtual bool
	set_int64( std::int64_t value, int var_position ) = 0;
	
	virtual bool
	set_uint64( std::uint64_t value, const std::string& var_name ) = 0;
	
	virtual bool
	set_uint64( std::uint64_t value, int var_position ) = 0;
	
	virtual bool
	set_time( std::chrono::system_clock::time_point value, const std::string& var_name ) = 0;
	
	virtual bool
	set_time( std::chrono::system_clock::time_point value, int var_position ) = 0;
	
	virtual bool
	set_text( const std::string& value, const std::string& var_name ) = 0;
	
	virtual bool
	set_text( const std::string& value, int var_position ) = 0;
	
	virtual bool
	set_blob( nodeoze::buffer& value, const std::string& var_name ) = 0;
	
	virtual bool
	set_blob( nodeoze::buffer& value, int var_position ) = 0;
	
	virtual operator safe_bool_type () const = 0;
};


class manager
{
	NODEOZE_DECLARE_SINGLETON( manager )
	
public:

	typedef std::function< void ( action_t action, oid_t oid, statement &stmt ) >						continuous_select_handler_f;
	typedef std::function< void ( action_t action, oid_t oid, statement &before, statement &after ) >	observer_reply_f;
	typedef std::function < std::string ( void ) >														backup_hook_f;
	typedef std::function < std::string ( void ) >														restore_hook_f;
	
	virtual ~manager() = 0;
	
	virtual std::error_code
	open( const std::string &s ) = 0;
	
	virtual bool
	is_open() const = 0;
	
	virtual std::error_code
	close() = 0;
	
	template< typename ...Params >
	std::error_code
	exec( const char *format, const Params &... params )
	{
		std::ostringstream	os;
			
		nodeoze::printf( os, format, params... );
		
		return exec( os.str() );
	}

	virtual std::error_code
	exec( const std::string &str ) = 0;

	template< typename ...Params >
	statement::ptr
	select( const char *format, const Params &... params )
	{
		std::ostringstream os;
			
		nodeoze::printf( os, format, params... );
		
		return select( os.str() );
	}
	
	virtual statement::ptr
	select( const std::string &str ) = 0;
	
	template< typename ...Params >
	statement::ptr
	prepare( const char *format, const Params &... params )
	{
		std::ostringstream os;
			
		nodeoze::printf( os, format, params... );
		
		return prepare( os.str() );
	}
	
	virtual statement::ptr
	prepare( const std::string &str ) = 0;
	
	virtual std::error_code
	prepare( const std::string &str, std::function< std::error_code ( statement &stmt ) > func ) = 0;
	
	inline bool
	is_prepared( statement::ptr &stmt )
	{
		return ( stmt && *stmt );
	}
	
	virtual nodeoze::scoped_operation
	continuous_select( const std::string &columns, const std::string &from, const std::string &where, continuous_select_handler_f handler ) = 0;
	
	virtual std::error_code
	start_transaction() = 0;
	
	virtual std::error_code
	end_transaction() = 0;
	
	virtual std::error_code
	cancel_transaction() = 0;
	
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
	
	template < class T >
	std::size_t
	count()
	{
		return count( T::table_name() );
	}
	
	std::size_t
	count( const std::string &table_name )
	{
		statement::ptr		stmt;
		int					rows = 0;
		
		stmt = select( "SELECT COUNT(*) FROM %;", table_name );
			
		if ( stmt->step() )
		{
			rows = stmt->int_at_column( 0 );
		}
			
		return static_cast< std::size_t >( rows );
	}
		
	template < class T >
	static database::statement::ptr
	find()
	{
		return manager::shared().select( "SELECT * FROM %;", T::table_name() );
	}

	template < class T >
	static database::statement::ptr
	find( oid_t oid )
	{
		return manager::shared().select( "SELECT * FROM % WHERE oid = %;", T::table_name(), oid );
	}

	template < class T >
	static database::statement::ptr
	find( const std::string &key, bool val )
	{
		return manager::shared().select( "SELECT * FROM % WHERE % = %;", T::table_name(), key, val );
	}

	template < class T >
	static database::statement::ptr
	find( const std::string &key, std::uint64_t val )
	{
		return manager::shared().select( "SELECT * FROM % WHERE % = %;", T::table_name(), key, val );
	}

	template < class T >
	static database::statement::ptr
	find( const std::string &key, const char *val )
	{
		return manager::shared().select( "SELECT * FROM % WHERE % LIKE '%';", T::table_name(), key, sanitize( val ) );
	}

	template < class T >
	static database::statement::ptr
	find( const std::string &key, const std::string &val )
	{
		return manager::shared().select( "SELECT * FROM % WHERE % LIKE '%';", T::table_name(), key, sanitize( val ) );
	}
	
	virtual std::uint32_t
	version() const = 0;

	virtual void
	set_version( std::uint32_t version ) = 0;
	
	virtual nodeoze::scoped_operation
	observe( const std::string &table, observer_reply_f reply ) = 0;

	virtual std::error_code
	backup( const std::string &db, backup_hook_f hook ) = 0;

	virtual std::error_code
	restore( const std::string &db, backup_hook_f hook ) = 0;
	
	virtual std::error_code
	backup_database( const nodeoze::path& backup_path, const std::vector<std::string>& statements, std::size_t step_size, int sleep_ms, std::atomic<bool>& exiting ) = 0;
	
	virtual oid_t
	last_oid() const = 0;

	virtual std::error_code
	last_error() = 0;
	
	virtual void
	clear_error() = 0;
	
};

class object
{
public:

	typedef std::shared_ptr< object >	ptr;
	
	object( oid_t oid = 0 )
	:
		m_oid( oid ),
		m_dirty( ( oid == 0 ) ? true : false )
	{
	}

	object( const object &that )
	:
		m_oid( that.m_oid ),
		m_dirty( that.m_dirty )
	{
	}

	object( const database::statement &stmt )
	:
		m_oid( stmt.uint64_at_column( 0 ) ),
		m_dirty( false )
	{
	}
	
	virtual ~object()
	{
	}
	
	template< class T >
	inline static void
	clear()
	{
		manager::shared().exec( "DELETE FROM %;", T::table_name() );
	}
	
	virtual void
	deflate( nodeoze::any &root ) const;
	
	virtual const std::string&
	get_table_name() const = 0;

	inline bool
	operator==( const object &obj )
	{
		return ( m_oid == obj.m_oid );
	}
	
	inline void
	remove() const
	{
		if ( m_oid )
		{
			manager::shared().exec( "DELETE FROM % WHERE oid = %;", get_table_name(), m_oid );
		}
	}

	inline oid_t
	oid() const
	{
		return m_oid;
	}
	
	inline void
	set_oid( oid_t oid )
	{
		if ( m_oid != oid )
		{
			m_oid = oid;
			m_dirty = true;
		}
	}

	bool
	dirty() const
	{
		return m_dirty;
	}
	
	void
	set_dirty( bool value )
	{
		m_dirty = value;
	}

protected:

	void
	_assign( const object &rhs )
	{
		m_oid = rhs.m_oid;
	}

	mutable oid_t	m_oid;
	mutable bool	m_dirty = true;
};

inline std::ostream&
operator<<(std::ostream &os, const nodeoze::database::action_t action )
{
	switch ( action )
	{
		case nodeoze::database::action_t::insert:
		{
			os << "insert";
		}
		break;
	
		case nodeoze::database::action_t::update:
		{
			os << "update";
		}
		break;
	
		case nodeoze::database::action_t::remove:
		{
			os << "remove";
		}
		break;
		
	}
	
	return os;
}

const std::error_category&
error_category();

inline std::error_code
make_error_code( nodeoze::database::code_t val )
{
	return std::error_code( static_cast<int>( val ), nodeoze::database::error_category() );
}

inline std::error_condition
make_error_condition( nodeoze::database::code_t val )
{
	return std::error_condition( static_cast<int>( val ), nodeoze::database::error_category() );
}

}

}

namespace std
{
	template<>
	struct is_error_code_enum< nodeoze::database::code_t > : public std::true_type {};
}

#define ndb nodeoze::database::manager::shared()

#endif
