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
 
#include "ndatabase_sqlite.h"
#include <nodeoze/nrunloop.h>
#include <nodeoze/nunicode.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nlog.h>
#include <nodeoze/nnotification.h>
#include <sstream>

#if defined( __APPLE__ )
#	include <sys/types.h>
#	include <sys/stat.h>
#endif

using namespace nodeoze;
using namespace nodeoze::database;

#if defined( __APPLE__ )
#	pragma mark database::manager_impl implementation
#endif


void
database::object::deflate( nodeoze::any &root ) const
{
	root[ "table" ] = get_table_name();
	root[ "oid" ]	= m_oid;
}


NODEOZE_DEFINE_SINGLETON( database::manager )

database::manager*
database::manager::create()
{
	return new manager_impl;
}

manager_impl::manager_impl()
:
	m_db( nullptr ),
	m_last_oid( 0 )
{
	sqlite3_config( SQLITE_CONFIG_LOG, log_callback, nullptr );
}


manager_impl::~manager_impl()
{
	mlog( marker::database, log::level_t::info, "closing db" );
	sqlite3_close( m_db );
}


std::error_code
manager_impl::open( const std::string &s )
{
	mlog( marker::database, log::level_t::info, "opening %", s.c_str() );
	
	m_prepared_statement_map.clear();
	
	for ( auto &stmt : m_prepared_statements )
	{
		stmt->finalize();
	}
	
#if defined( UNICODE )
	m_error = make_error_code( static_cast< code_t >( sqlite3_open16( widen( s ).c_str(), &m_db ) ) );
#else
	m_error = make_error_code( static_cast< code_t >( sqlite3_open( s.c_str(), &m_db ) ) );
#endif

	if ( !m_error )
	{
#if defined( __APPLE__ )
		chmod( s.c_str(), 0666 );
#endif

		sqlite3_preupdate_hook( m_db, on_preupdate, this );
		exec( "PRAGMA foreign_keys = ON;" );
	}
	
	return m_error;
}


bool
manager_impl::is_open() const
{
	return ( m_db != NULL ) ? true : false;
}


std::error_code
database::manager_impl::close()
{
	m_error = std::error_code();

	mlog( marker::database, log::level_t::info, "closing db" );
	
	for ( auto stmt : m_prepared_statements )
	{
		stmt->finalize();
	}
	
	m_prepared_statements.clear();
	
	m_observers.clear();
	
	if ( m_db )
	{
		auto err = sqlite3_close( m_db );

		if ( err == SQLITE_OK )
		{
			m_db = nullptr;
		}
		else
		{
			m_error = make_error_code( static_cast< database::code_t >( err ) );
		}
	}
	
	return m_error;
}


std::error_code
manager_impl::exec( const std::string &str )
{
	char		*error	= nullptr;
	
	mlog( marker::database, log::level_t::info, "m_db = %, %", m_db, str );

	int err = sqlite3_exec( m_db, str.c_str(), nullptr, nullptr, &error );
	
	mlog( marker::database, log::level_t::info, "exec result code = %(%)", err, sqlite3_errstr( err ) );
	
	if ( err )
	{
		nlog( log::level_t::error, "sqlite3_exec() failed: %, %", err, error );
		nlog( log::level_t::error, "exec string: %", str );
		m_error = make_error_code( static_cast< database::code_t >( err ) );
	}
	else
	{
		m_error = make_error_code( database::code_t::ok );
	}
	
	if ( error )
	{
		sqlite3_free( error );
	}

	return m_error;
}


database::statement::ptr
database::manager_impl::select( const std::string &str )
{
	sqlite3_stmt				*stmt;
	database::statement::ptr	ret;

	mlog( marker::database, log::level_t::info, "%", str );
	
	if ( sqlite3_prepare_v2( m_db, str.c_str(), -1, &stmt, NULL ) == SQLITE_OK )
	{
		ret.reset( new statement_impl( stmt ) );
	}
	else
	{
		mlog( marker::database, log::level_t::error, "str: %", str );
		ret.reset( new statement_impl( NULL ) );
	}
	
	return ret;
}


scoped_operation
database::manager_impl::continuous_select( const std::string &columns, const std::string &table, const std::string &where, continuous_select_handler_f handler )
{
	struct context_s
	{
		scoped_operation	operation;
		bitmap				bitmap;
	};
	
	auto os			= std::ostringstream();
	auto operation	= scoped_operation();
	auto context 	= std::make_shared< context_s >();
	auto check_stmt	= static_cast< sqlite3_stmt* >( nullptr );
	auto err 		= int( 0 );
	
	os << "SELECT " << columns << " FROM " << table << " " << where << ";";

	auto stmt = select( os.str() );
	
	while ( stmt->step() )
	{
		auto oid = stmt->int64_at_column( 0 );
		
		handler( action_t::insert, oid, *stmt );
		context->bitmap.set_bit( oid );
	}
	
	os.clear();
	os.str( "" );
	
	if ( where.empty() )
	{
		os << "SELECT " << columns << " FROM " << table << " WHERE oid = ?";
	}
	else
	{
		os << "SELECT " << columns << " FROM " << table << " " << where << " AND oid = ?";
	}
	
	err = sqlite3_prepare_v2( m_db, os.str().c_str(), -1, &check_stmt, nullptr );
	ncheck_error( err == SQLITE_OK, exit, "sqlite3_prepare_v2() failed (%)", err );
	
	context->operation = observe( table, [=]( action_t action, oid_t oid, statement &before, statement &after ) mutable
	{
		nunused( before );
		nunused( after );
	
		runloop::shared().dispatch( [=]() mutable
		{
			auto stmt = statement_impl( check_stmt, false );

			switch ( action )
			{
				case action_t::insert:
				case action_t::update:
				{
					sqlite3_reset( check_stmt );
					err = sqlite3_bind_int64( check_stmt, 1, oid );
					ncheck_error( err == SQLITE_OK, exit, "sqlite3_bind_int64() failed (%)", err );
					err = sqlite3_step( check_stmt );
					ncheck_error( ( err == SQLITE_ROW ) || ( err == SQLITE_DONE ), exit, "sqlite3_step() failed (%)", err );
					
					if ( err == SQLITE_ROW )
					{
						if ( action == action_t::insert )
						{
							assert( !context->bitmap.is_bit_set( oid ) );
							context->bitmap.set_bit( oid );
						}
						else if ( !context->bitmap.is_bit_set( oid ) )
						{
							context->bitmap.set_bit( oid );
							action = action_t::insert;
						}
						
						handler( action, oid, stmt );
					}
					else if ( context->bitmap.is_bit_set( oid ) )
					{
						context->bitmap.clear_bit( oid );
						handler( action_t::remove, oid, stmt );
					}
				}
				break;
				
				case action_t::remove:
				{
					if ( context->bitmap.is_bit_set( oid ) )
					{
						context->bitmap.clear_bit( oid );
						handler( action, oid, stmt );
					}
				}
				break;
			}

		exit:

			return;
 		} );
	} );

exit:
	
	return scoped_operation::create( [=]( void *v ) mutable
	{
		nunused( v );
	
		context->operation.reset();

		if ( check_stmt != nullptr )
		{
			sqlite3_finalize( check_stmt );
			check_stmt = nullptr;
		}

	} );
}


database::statement::ptr
database::manager_impl::prepare( const std::string &str )
{
	sqlite3_stmt				*stmt;
	database::statement::ptr	ret;

	mlog( marker::database, log::level_t::info, "%", str );
	
	auto err = sqlite3_prepare_v2( m_db, str.c_str(), -1, &stmt, NULL );

	if ( err == SQLITE_OK )
	{
		ret.reset( new statement_impl( stmt ) );
		m_error = make_error_code( database::code_t::ok );
		m_prepared_statements.push_back( ret );
	}
	else
	{
		m_error = make_error_code( static_cast< database::code_t >( err ) );
		ret.reset( new statement_impl( NULL ) );
	}
	
	return ret;
}


std::error_code
database::manager_impl::prepare( const std::string &str, std::function< std::error_code ( statement &stmt ) > func )
{
	mlog( marker::database, log::level_t::info, "%", str );
	
	auto err	= std::error_code();
	auto it		= m_prepared_statement_map.find( str );
	
	if ( it == m_prepared_statement_map.end() )
	{
		sqlite3_stmt *stmt;

		err = make_error_code( static_cast< database::code_t >( sqlite3_prepare_v2( m_db, str.c_str(), -1, &stmt, NULL ) ) );
		ncheck_error_quiet( !err, exit );

		m_prepared_statement_map.emplace( std::piecewise_construct, std::forward_as_tuple( str ), std::forward_as_tuple( stmt ) );
		it = m_prepared_statement_map.find( str );
	}
	
	err = func( it->second );
	
	it->second.reset_prepared();
	
exit:
	
	return err;
}


std::uint32_t
database::manager_impl::version() const
{
	static sqlite3_stmt	*stmt;
	std::uint32_t		version = 0;

	if ( sqlite3_prepare_v2( m_db, "PRAGMA user_version;", -1, &stmt, nullptr ) == SQLITE_OK )
	{
		while ( sqlite3_step( stmt ) == SQLITE_ROW )
		{
			version = sqlite3_column_int( stmt, 0 );
        }

		sqlite3_finalize( stmt );
	}

	return version;
}


void
database::manager_impl::set_version( std::uint32_t version )
{
	char				*error = nullptr;
	std::ostringstream	os;

	os << "PRAGMA user_version = " << version << ";";

	int ret = sqlite3_exec( m_db, os.str().c_str(), 0, 0, &error );
	
	if ( ret )
	{
		nlog( log::level_t::error, "sqlite3_exec() failed: %, %", ret, error );
	}
}


scoped_operation
database::manager_impl::observe( const std::string &table, observer_reply_f reply )
{
	auto key = ++m_observer_key;
	
	m_observers[ table ][ key ] = reply;

	return scoped_operation::create( [=]( void *v ) mutable
	{
		nunused( v );
	
		auto it = m_observers[ table ].find( key );
		
		if ( it != m_observers[ table ].end() )
		{
			m_observers[ table ].erase( it );
		}
	} );
}


std::error_code
database::manager_impl::backup( const std::string &dest, backup_hook_f hook )
{
	sqlite3_backup	*backup		= nullptr;
	sqlite3			*dest_db	= nullptr;
	auto			ret			= std::error_code();

	ret = make_error_code( static_cast< code_t >( sqlite3_open( dest.c_str(), &dest_db ) ) );

	if ( ret )
	{
		goto exit;
	}

	if ( hook )
	{
		std::string s = hook();

		char *error = NULL;
	
		ret = make_error_code( static_cast< code_t >( sqlite3_exec( dest_db, s.c_str(), 0, 0, &error ) ) );
	
		if ( ret )
		{
			goto exit;
		}
	}

	backup = sqlite3_backup_init( dest_db, "main", m_db, "main" );

	if ( !backup )
	{
		ret = make_error_code( static_cast< code_t >( sqlite3_errcode( dest_db ) ) );
		goto exit;
    }

	sqlite3_backup_step( backup, -1 );

exit:

	if ( backup )
	{
		sqlite3_backup_finish( backup );
	}

	if ( dest_db )
	{
		sqlite3_close( dest_db );
	}

	return ret;
}


std::error_code
database::manager_impl::restore( const std::string &from, restore_hook_f hook )
{
	nunused( from );
	
	std::string filename;
	auto		raw	= sqlite3_db_filename( m_db, "main" );
	auto		ret	= std::error_code();

	if ( !raw )
	{
		ret = make_error_code( std::errc::invalid_argument );
		goto exit;
	}

	filename = raw;

	ret = make_error_code( static_cast< code_t >( sqlite3_close( m_db ) ) );

	if ( ret )
	{
		goto exit;
	}

	ret = make_error_code( static_cast< code_t >( sqlite3_open( filename.c_str(), &m_db ) ) );

	if ( ret )
	{
		goto exit;
	}

	if ( hook )
	{
		std::string s = hook();

		char *error = NULL;
	
		ret = make_error_code( static_cast< code_t >( sqlite3_exec( m_db, s.c_str(), 0, 0, &error ) ) );
	
		if ( ret )
		{
			goto exit;
		}
	}

exit:

	return ret;
}


std::error_code
database::manager_impl::backup_database( const nodeoze::path& backup_path, const std::vector<std::string>& statements, std::size_t step_size, int sleep_ms, std::atomic<bool>& exiting )
{
	auto ret = std::error_code();
	
	sqlite3* backup_db;
	sqlite3_backup* backup_info;
	
	ret = make_error_code( static_cast< code_t >( sqlite3_open( backup_path.to_string().c_str(), &backup_db ) ) );
	
	ncheck_error_quiet( !ret, exit );

	for ( auto statement : statements )
	{
		char *error	= nullptr;
		
		ret = make_error_code( static_cast< code_t >( sqlite3_exec( backup_db, statement.c_str(), nullptr, nullptr, &error ) ) );

		if ( ret )
		{
			if ( error )
			{
				nlog( log::level_t::error, "error %(%) in backup file exec [ % ] ", ret, error, statement );
				sqlite3_free( error );
			}
			else
			{
				nlog( log::level_t::error, "error % in backup file exec [ % ] ", ret, statement );
			}
			
			goto close_backup;
		}
	}

	backup_info = sqlite3_backup_init( backup_db, "main", m_db, "main" );

	ncheck_error_action( backup_info, ret = make_error_code( std::errc::invalid_argument ), close_backup, "error initializing backup" );
	
	do
	{
		ret = make_error_code( static_cast< code_t >( sqlite3_backup_step(backup_info, static_cast< int >( step_size ) ) ) );
		
		if ( !ret || ( ret.value() == SQLITE_BUSY ) || ( ret.value() == SQLITE_LOCKED ) )
		{
			if ( ! exiting )
			{
				sqlite3_sleep( sleep_ms );
			}
		}
	}
	while( !ret || ( ret.value() == SQLITE_BUSY ) || ( ret.value() == SQLITE_LOCKED ) );
	
	if ( ret.value() == SQLITE_DONE )
	{
		ret = std::error_code();
	}
	else
	{
		mlog( marker::database, log::level_t::error, "backup did not complete: % (%)", ret, ret.message() );
	}

	ret = make_error_code( static_cast< code_t >( sqlite3_backup_finish( backup_info ) ) );
	
	if ( !ret )
	{
		mlog( marker::database, log::level_t::error, "backup finish result code % (%)", ret, ret.message() );
	}
	
close_backup:

	ret = make_error_code( static_cast< code_t >( sqlite3_close( backup_db ) ) );
	
	if ( !ret )
	{
		mlog( marker::database, log::level_t::error, "backup finish result code % (%)", ret, ret.message() );
	}
	
exit:

	return ret;
}


oid_t
database::manager_impl::last_oid() const
{
	return m_last_oid;
}


void
database::manager_impl::on_preupdate( void *v, sqlite3 *db, int op, char const *z_db, char const *z_name, sqlite3_int64 key1, sqlite3_int64 key2 )
{
	database::manager_impl *self = reinterpret_cast< database::manager_impl* >( v );
	
	if ( std::strcmp( z_db, "main" ) == 0 )
	{
		auto it = self->m_observers.find( z_name );
		
		if ( it != self->m_observers.end() )
		{
			statement_preupdate before( db, sqlite3_preupdate_old );
			statement_preupdate after( db, sqlite3_preupdate_new );
			
			for ( auto &pair : it->second )
			{
				switch ( op )
				{
					case SQLITE_INSERT:
					{
						pair.second( database::action_t::insert, key2, before, after );
					}
					break;
			
					case SQLITE_UPDATE:
					{
						pair.second( database::action_t::update, key2, before, after );
					}
					break;
			
					case SQLITE_DELETE:
					{
						pair.second( database::action_t::remove, key1, before, after );
					}
					break;
				}
			}
		}
		
		self->m_last_oid = key2;
	}
}

void
database::manager_impl::log_callback( void *p_arg, int code, const char *msg )
{
	nunused( p_arg );

	if ( code != SQLITE_SCHEMA )
	{
		mlog( marker::database, log::level_t::error, "sqlite error %, msg: %", code, msg );
	}
}


std::string
database::sanitize( const std::string &str )
{
	std::string output;

	for ( std::string::const_iterator it = str.begin(); it != str.end(); it++ )
    {
		if ( *it == '\'' )
		{
			output += "''";
		}
		else
		{
			output += *it;
		}
    }

    return output;
}


std::error_code
database::manager_impl::last_error()
{
	return m_error;
}

void
database::manager_impl::clear_error()
{
	m_error = make_error_code( database::code_t::ok );
}





