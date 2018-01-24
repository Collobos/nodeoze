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

#ifndef _nodeoze_database_sqlite_h
#define _nodeoze_database_sqlite_h

#include <nodeoze/ndatabase.h>
#include <nodeoze/nbitmap.h>
#include <sqlite3.h>
#include <cstdint>
#include <list>
#include <map>

namespace nodeoze {

namespace database {

class statement_impl : public statement
{
public:

	statement_impl( sqlite3_stmt *stmt, bool finalize = true )
	:
		m_stmt( stmt ),
		m_finalize( finalize )
	{
	}
	
	statement_impl( const statement_impl &rhs ) = delete;
	
	statement_impl( statement_impl &&rhs )
	{
		m_stmt = rhs.m_stmt;
		m_finalize = rhs.m_finalize;
		
		rhs.m_stmt = nullptr;
		rhs.m_finalize = false;
	}

	~statement_impl()
	{
		if ( m_stmt && m_finalize )
		{
			finalize();
		}
	}
	
	inline statement_impl&
	operator=( const statement_impl &rhs ) = delete;
	
	inline statement_impl&
	operator=( statement_impl &&rhs )
	{
		m_stmt = rhs.m_stmt;
		m_finalize = rhs.m_finalize;
		
		rhs.m_stmt = nullptr;
		rhs.m_finalize = false;
		
		return *this;
	}

	void
	finalize()
	{
		sqlite3_finalize( m_stmt );
		m_stmt = nullptr;
	}

	std::error_code
	last_error() const
	{
		auto status = m_status;
		
		if ( status )
		{
			const_cast< statement_impl* >( this )->reset_prepared();
		}
		
		return status;
	}
	
	bool
	step()
	{
		bool ok = true;
		if ( ! m_stmt )
		{
			m_status = make_error_code( database::code_t::internal_error );
			ok = false;
		}
		else
		{
			auto result = sqlite3_step( m_stmt );
			
			if ( result == SQLITE_ROW )
			{
				m_status = make_error_code( database::code_t::ok );
				ok = true;
			}
			else if ( result == SQLITE_OK || result == SQLITE_DONE )
			{
				m_status = make_error_code( database::code_t::ok );
				ok = false;
			}
			else
			{
				m_status = make_error_code( static_cast< database::code_t >( result ) );
				ok = false;
			}
		}
		return ok;
	}
	
	std::error_code
	reset_prepared()
	{
		if ( m_stmt )
		{
			sqlite3_reset( m_stmt );
			sqlite3_clear_bindings( m_stmt );
			m_status = make_error_code( database::code_t::ok );
		}
		else
		{
			m_status = make_error_code( database::code_t::internal_error );
		}
		
		return m_status;
	}
	
	/*
	 * exec_prepared should be used on prepared statments that return no data--more specifically, 
	 * on update or insert statements. The expected result is, in general, SQLITE_DONE, although
	 * SQLITE_OK and SQLITE_ROW are not considered errors. After stepping, the statement is unconditionally
	 * reset and bindings are cleared.
	 *
	 */
	std::error_code
	exec_prepared()
	{
		if ( m_stmt )
		{
			auto result = SQLITE_OK;
			auto count	= 0;

			do
			{
				result = sqlite3_step( m_stmt );
			}
			while ( ( result == SQLITE_SCHEMA ) && ( ++count < 10 ) );

			sqlite3_reset( m_stmt );
			sqlite3_clear_bindings( m_stmt );

			if ( result == SQLITE_DONE || result == SQLITE_OK || result == SQLITE_ROW )
			{
				m_status = make_error_code( database::code_t::ok );
			}
			else
			{
				m_status = make_error_code( static_cast< database::code_t >( result ) );
			}
		}
		else
		{
			m_status = make_error_code( database::code_t::internal_error );
		}
		return m_status;
	}

	bool
	bool_at_column( int col ) const
	{
		return ( m_stmt ) ? ( sqlite3_column_int( m_stmt, col ) ? true : false ) : false;
	}

	int
	int_at_column( int col ) const
	{
		return ( m_stmt ) ? ( int ) sqlite3_column_int( m_stmt, col ) : 0;
	}

	std::int64_t
	int64_at_column( int col ) const
	{
		return ( m_stmt ) ? ( std::int64_t ) sqlite3_column_int64( m_stmt, col ) : 0;
	}
	
	std::uint64_t
	uint64_at_column( int col ) const
	{
		return ( m_stmt ) ? ( std::uint64_t ) sqlite3_column_int64( m_stmt, col ) : 0;
	}

	std::chrono::system_clock::time_point
	time_at_column( int col ) const
	{
		return std::chrono::system_clock::from_time_t( m_stmt ? static_cast< std::time_t >( sqlite3_column_int64( m_stmt, col ) ) : 0 );
	}
	
	std::string
	text_at_column( int col ) const
	{
		auto text = ( m_stmt ) ? ( char* ) sqlite3_column_text( m_stmt, col ) : nullptr;
		return ( text ) ? text  : "";
	}
	
	nodeoze::buffer
	blob_at_column( int col ) const
	{
		if ( m_stmt )
		{
			auto blob = sqlite3_column_blob( m_stmt, col );
			if ( blob )
			{
				std::size_t bytes = sqlite3_column_bytes( m_stmt, col );
				return buffer( blob, bytes );
			}
		}
		return buffer();
	}

	virtual bool
	set_bool( bool value, const std::string& var_name )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );
		
		if ( m_stmt )
		{
			auto index = sqlite3_bind_parameter_index( m_stmt, var_name.c_str() );
			if ( index < 1 )
			{
				ok = false;
				m_status = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				auto result = sqlite3_bind_int( m_stmt, index, value ? 1 : 0 );
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_bool( bool value, int var_index )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto result = sqlite3_bind_int( m_stmt, var_index, value ? 1 : 0 );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_status = make_error_code( static_cast< database::code_t >( result ) );
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_int( int value, const std::string& var_name )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto index = sqlite3_bind_parameter_index( m_stmt, var_name.c_str() );
			if ( index < 1 )
			{
				ok = false;
				m_status = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				auto result = sqlite3_bind_int( m_stmt, index, value );
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_int( int value, int var_index )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto result = sqlite3_bind_int( m_stmt, var_index, value );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_status = make_error_code( static_cast< database::code_t >( result ) );
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_int64( std::int64_t value, const std::string& var_name )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto index = sqlite3_bind_parameter_index( m_stmt, var_name.c_str() );
			if ( index < 1 )
			{
				ok = false;
				m_status = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				auto result = sqlite3_bind_int64( m_stmt, index, value );
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_int64( std::int64_t value, int var_index )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto result = sqlite3_bind_int64( m_stmt, var_index, value );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_status = make_error_code( static_cast< database::code_t >( result ) );
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_uint64( std::uint64_t value, const std::string& var_name )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto index = sqlite3_bind_parameter_index( m_stmt, var_name.c_str() );
			if ( index < 1 )
			{
				ok = false;
				m_status = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				auto result = sqlite3_bind_int64( m_stmt, index, value );
				if (  result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_uint64( std::uint64_t value, int var_index )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );
		
		if ( m_stmt )
		{
			auto result = sqlite3_bind_int64( m_stmt, var_index, value );
			
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_status = make_error_code( static_cast< database::code_t >( result ) );
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_time( std::chrono::system_clock::time_point value, const std::string& var_name )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto index = sqlite3_bind_parameter_index( m_stmt, var_name.c_str() );
			
			std::int64_t timeval = std::chrono::system_clock::to_time_t( value );
			
			if ( index < 1 )
			{
				ok = false;
				m_status = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				auto result = sqlite3_bind_int64( m_stmt, index, timeval );
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_time( std::chrono::system_clock::time_point value, int var_index )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			std::int64_t timeval = std::chrono::system_clock::to_time_t( value );
			auto result = sqlite3_bind_int64( m_stmt, var_index, timeval );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_status = make_error_code( static_cast< database::code_t >( result ) );
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_text( const std::string& value, const std::string& var_name )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto index = sqlite3_bind_parameter_index( m_stmt, var_name.c_str() );
			if ( index < 1 )
			{
				ok = false;
				m_status = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				auto result = sqlite3_bind_text( m_stmt, index, value.c_str(), -1, SQLITE_TRANSIENT );
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_text( const std::string& value, int var_index )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto result = sqlite3_bind_text( m_stmt, var_index, value.c_str(), -1, SQLITE_TRANSIENT );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_status = make_error_code( static_cast< database::code_t >( result ) );
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_blob( nodeoze::buffer& value, const std::string& var_name )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			auto index = sqlite3_bind_parameter_index( m_stmt, var_name.c_str() );
			if ( index < 1 )
			{
				ok = false;
				m_status = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				if ( value.size() < 1 )
				{
					auto result = sqlite3_bind_blob( m_stmt, index, nullptr, 0, nullptr );
					if ( result != SQLITE_OK )
					{
						ok = false;
						m_status = make_error_code( static_cast< database::code_t >( result ) );
					}
				}
				else
				{
					auto bufsize = value.size();
					auto bufptr = value.detach();
					
					auto result = sqlite3_bind_blob( m_stmt, index, bufptr, static_cast<int>(bufsize), [] ( void * ptr )
					{
						delete static_cast< std::uint8_t * > ( ptr );
					} );
					
					if ( result != SQLITE_OK )
					{
						ok = false;
						m_status = make_error_code( static_cast< database::code_t >( result ) );
					}
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual bool
	set_blob( nodeoze::buffer& value, int var_index )
	{
		auto ok = true;
		
		m_status = make_error_code( database::code_t::ok );

		if ( m_stmt )
		{
			if ( value.size() < 1 )
			{
				auto result = sqlite3_bind_blob( m_stmt, var_index, nullptr, 0, nullptr );
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
			else
			{
				auto bufsize = value.size();
				auto bufptr = value.detach();
					
				auto result = sqlite3_bind_blob( m_stmt, var_index, bufptr, static_cast<int>(bufsize), [] ( void * ptr )
				{
					delete static_cast< std::uint8_t * > ( ptr );
				} );
				
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_status = make_error_code( static_cast< database::code_t >( result ) );
				}
			}
		}
		else
		{
			ok = false;
			m_status = make_error_code( database::code_t::internal_error );
		}
		return ok;
	}
	
	virtual operator safe_bool_type () const
	{
		return ( m_stmt != nullptr ) ? reinterpret_cast< safe_bool_type >( &statement_impl::safe_bool_func ) : 0;
	}

private:

	inline void
	safe_bool_func()
	{
	}

	sqlite3_stmt	*m_stmt;
	bool			m_finalize;
	std::error_code	m_status;
};


class statement_preupdate : public statement
{
public:

	typedef int ( *data_f )( sqlite3 *db, int col, sqlite3_value **val );

	statement_preupdate( sqlite3 *db, data_f func )
	:
		m_db( db ),
		m_func( func )
	{
	}

	~statement_preupdate()
	{
	}

	bool
	step()
	{
		assert( 0 );
		return false;
	}
	
	void
	finalize()
	{
		assert( 0 );
	}
	
	std::error_code
	exec_prepared()
	{
		assert(0);
		return make_error_code( database::code_t::internal_error );
	}

	std::error_code
	reset_prepared()
	{
		assert(0);
		return make_error_code( database::code_t::internal_error );
	}
	
	std::error_code
	last_error() const
	{
		assert(0);
		return make_error_code( database::code_t::internal_error );
	}
	
	bool
	bool_at_column( int col ) const
	{
		sqlite3_value *v;
		m_func( m_db, col, &v );
		auto val1 = sqlite3_value_int( v );
		return ( val1 ) ? true : false;
	}

	int
	int_at_column( int col ) const
	{
		sqlite3_value *v;
		m_func( m_db, col, &v );
		return sqlite3_value_int( v );
	}

	std::int64_t
	int64_at_column( int col ) const
	{
		sqlite3_value *v;
		m_func( m_db, col, &v );
		return static_cast< std::int64_t >( sqlite3_value_int64( v ) );
	}

	std::uint64_t
	uint64_at_column( int col ) const
	{
		sqlite3_value *v;
		m_func( m_db, col, &v );
		return static_cast< std::uint64_t >( sqlite3_value_int64( v ) );
	}
	
	std::chrono::system_clock::time_point
	time_at_column( int col ) const
	{
		sqlite3_value *v;
		m_func( m_db, col, &v );
		
//		return std::chrono::system_clock::time_point( std::chrono::system_clock::duration( sqlite3_value_int64( v ) ) );
		return std::chrono::system_clock::from_time_t( sqlite3_value_int64( v ) );
		
	}

	std::string
	text_at_column( int col ) const
	{
		sqlite3_value	*v;
		std::string		ret;
		
		m_func( m_db, col, &v );
		
		auto text = sqlite3_value_text( v );
		
		if ( text )
		{
			ret = std::string( reinterpret_cast< const char* >( text ) );
		}
		
		return ret;
	}
	
	nodeoze::buffer
	blob_at_column( int col ) const
	{
		sqlite3_value *v;
		m_func( m_db, col, &v );
	
		auto blob = sqlite3_value_blob( v );
		if ( blob )
		{
			auto nbytes = sqlite3_value_bytes( v );
			return buffer( blob, nbytes );
		}
		else
		{
			return buffer();
		}
	}

	virtual bool
	set_bool( bool value, const std::string& var_name )
	{
		nunused( value );
		nunused( var_name );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_bool( bool value, int var_position )
	{
		nunused( value );
		nunused( var_position );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_int( int value, const std::string& var_name )
	{
		nunused( value );
		nunused( var_name );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_int( int value, int var_position )
	{
		nunused( value );
		nunused( var_position );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_int64( std::int64_t value, const std::string& var_name )
	{
		nunused( value );
		nunused( var_name );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_int64( std::int64_t value, int var_position )
	{
		nunused( value );
		nunused( var_position );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_uint64( std::uint64_t value, const std::string& var_name )
	{
		nunused( value );
		nunused( var_name );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_uint64( std::uint64_t value, int var_position )
	{
		nunused( value );
		nunused( var_position );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_time( std::chrono::system_clock::time_point value, const std::string& var_name )
	{
		nunused( value );
		nunused( var_name );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_time( std::chrono::system_clock::time_point value, int var_position )
	{
		nunused( value );
		nunused( var_position );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_text( const std::string& value, const std::string& var_name )
	{
		nunused( value );
		nunused( var_name );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_text( const std::string& value, int var_position )
	{
		nunused( value );
		nunused( var_position );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_blob( nodeoze::buffer& value, const std::string& var_name )
	{
		nunused( value );
		nunused( var_name );
		
		assert( false );
		return false;
	}
	
	virtual bool
	set_blob( nodeoze::buffer& value, int var_position )
	{
		nunused( value );
		nunused( var_position );
		
		assert( false );
		return false;
	}
	
	virtual operator safe_bool_type () const
	{
		return 0;
	}

private:

	sqlite3 *m_db;
	data_f	m_func;
};


class manager_impl : public manager
{
public:

	manager_impl();
	
	virtual
	~manager_impl();
	
	virtual std::error_code
	open( const std::string &s );
	
	virtual bool
	is_open() const;
	
	virtual std::error_code
	close();
	
	virtual std::error_code
	exec( const std::string &str );

	virtual statement::ptr
	select( const std::string &str );
	
	virtual nodeoze::scoped_operation
	continuous_select( const std::string &columns, const std::string &from, const std::string &where, continuous_select_handler_f handler );
	
	virtual statement::ptr
	prepare( const std::string& text );

	virtual std::error_code
	prepare( const std::string &str, std::function< std::error_code ( statement &stmt ) > func );
	
	virtual std::error_code
	start_transaction()
	{
		return exec( "BEGIN TRANSACTION;" );
	}
	
	virtual std::error_code
	end_transaction()
	{
		return exec( "COMMIT;" );
	}
	
	virtual std::error_code
	cancel_transaction()
	{
		return exec( "ROLLBACK;" );
	}
	
	virtual std::uint32_t
	version() const;

	virtual void
	set_version( std::uint32_t version );
	
	virtual nodeoze::scoped_operation
	observe( const std::string &table, observer_reply_f reply );

	virtual std::error_code
	backup( const std::string &db, backup_hook_f hook );

	virtual std::error_code
	restore( const std::string &db, backup_hook_f hook );

	virtual std::error_code
	backup_database( const nodeoze::path& backup_path, const std::vector<std::string>& statements, std::size_t step_size, int sleep_ms, std::atomic<bool>& exiting  );
	
	virtual oid_t
	last_oid() const;

	virtual std::error_code
	last_error();

	void
	clear_error();
	
private:

	typedef std::unordered_map< std::uint64_t, observer_reply_f >	observers_t;
	typedef std::unordered_map< std::string, observers_t >			map_t;
	
	static void
	on_preupdate( void *v, sqlite3 *db, int op, char const *z_db, char const *z_name, sqlite3_int64 key1, sqlite3_int64 key2 );
	
	static void
	log_callback( void *p_arg, int code, const char *msg );

	sqlite3			*m_db;
	std::uint64_t	m_observer_key		= 0;
	map_t			m_observers;
	oid_t			m_last_oid;
	std::error_code	m_error;
	
	std::unordered_map< std::string, statement_impl >	m_prepared_statement_map;
	std::vector< database::statement::ptr >		m_prepared_statements;
};

}

}

#endif
