#include <nodeoze/database.h>
#include <nodeoze/runloop.h>
#include <nodeoze/bitmap.h>
#include <nodeoze/test.h>
#include <sqlite3.h>

#if defined( __APPLE__ )
#	include <sys/types.h>
#	include <sys/stat.h>
#endif

using namespace nodeoze;

class init
{
public:

	init()
	{
		sqlite3_config( SQLITE_CONFIG_LOG, []( void *p_arg, int code, const char *msg )
		{
			nunused( p_arg );
	
		} , nullptr );
	}
} g_init;

#if defined( __APPLE__ )
#	pragma mark nodeoze::database implementation
#endif

database::database()
:
	m_db( nullptr ),
	m_last_oid( 0 )
{
}


database::database( const std::string &name )
:
	m_db( nullptr ),
	m_last_oid( 0 )
{
	open( name );
}


database::~database()
{
	close();
}


std::error_code
database::open( const std::string &name )
{
	if ( !name.empty() )
	{
		reset_all_prepared();
		
#if defined( UNICODE )
		m_err = make_error_code( static_cast< errc >( sqlite3_open16( widen( name ).c_str(), &m_db ) ) );
#else
		m_err = make_error_code( static_cast< errc >( sqlite3_open( name.c_str(), &m_db ) ) );
#endif

		if ( !m_err )
		{
			if ( name != ":memory:" )
			{
#if defined( __APPLE__ )
				chmod( name.c_str(), 0666 );
#endif
			}

			typedef void(*preupdate_hook_type)( void*, sqlite3*, int, char const*, char const*, sqlite3_int64, sqlite3_int64 );

			sqlite3_preupdate_hook( m_db, reinterpret_cast< preupdate_hook_type >( on_preupdate ), this );
			exec( "PRAGMA foreign_keys = ON;" );
		}
	}
	
	return m_err;
}


bool
database::is_open() const
{
	return ( m_db != nullptr ) ? true : false;
}


std::error_code
database::close()
{
	m_err = std::error_code();

	reset_all_prepared();
	
	remove_all_listeners( "insert" );
	remove_all_listeners( "update" );
	remove_all_listeners( "delete" );
	
	if ( m_db )
	{
		auto err = sqlite3_close( m_db );

		if ( err == SQLITE_OK )
		{
			m_db = nullptr;
		}
		else
		{
			m_err = make_error_code( static_cast< database::errc >( err ) );
		}
	}
	
	return m_err;
}


std::error_code
database::exec( const std::string &str )
{
	m_err = make_error_code( static_cast< errc >( sqlite3_exec( m_db, str.c_str(), nullptr, nullptr, nullptr ) ) );
	
	return m_err;
}


database::statement
database::select( const std::string &str )
{
	sqlite3_stmt *stmt;

	auto err = make_error_code( static_cast< errc >( sqlite3_prepare_v2( m_db, str.c_str(), -1, &stmt, nullptr ) ) );

	return ( !err ) ? statement( stmt ) : statement( err );
}


database::statement
database::prepare( const std::string &str )
{
	sqlite3_stmt	*stmt;
	statement		ret;
	
	m_err = make_error_code( static_cast< errc >( sqlite3_prepare_v2( m_db, str.c_str(), -1, &stmt, nullptr ) ) );

	if ( !m_err )
	{
		ret = statement( stmt );
		m_prepared_statements.push_back( ret );
	}
	else
	{
		ret = statement( m_err );
	}
	
	return ret;
}


void
database::reset_all_prepared()
{
	m_prepared_statement_map.clear();

	for ( auto &stmt : m_prepared_statements )
	{
		stmt.reset_prepared();
	}
}


scoped_operation
database::continuous_select( const std::string &columns, const std::string &table, const std::string &where, continuous_select_handler_f handler )
{
	struct context_s
	{
		listener_id_type	insert;
		listener_id_type	update;
		listener_id_type	remove;
		bitmap				bitmap;
	};

	auto os			= std::ostringstream();
	auto operation	= scoped_operation();
	auto context 	= std::make_shared< context_s >();
	auto check_stmt	= std::shared_ptr< sqlite3_stmt >( nullptr );
	auto checker	= []( sqlite3_stmt *check_stmt, auto oid ) mutable
	{
		auto err = std::error_code();

		if ( check_stmt )
		{
			sqlite3_reset( check_stmt );

			err = make_error_code( static_cast< errc >( sqlite3_bind_int64( check_stmt, 1, oid ) ) );

			if ( err == errc::ok )
			{
				err = make_error_code( static_cast< errc >( sqlite3_step( check_stmt ) ) );
			}
		}

		return err;
	};

	os << "SELECT " << columns << " FROM " << table << " " << where << ";";

	auto stmt = select( os.str() );
	
	while ( stmt.step() )
	{
		auto oid = stmt.int64_at_column( 0 );

		handler( action_type::insert, oid, stmt );
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

	sqlite3_stmt *try_stmt;
	
	auto err = make_error_code( static_cast< errc >( sqlite3_prepare_v2( m_db, os.str().c_str(), -1, &try_stmt, nullptr ) ) );
	ncheck_error( !err, exit );

	check_stmt = std::shared_ptr< sqlite3_stmt >( try_stmt, []( auto stmt )
	{
		assert( stmt );

		sqlite3_finalize( stmt );
	} );

	context->insert = on( "insert", [=]( std::string target, std::int64_t oid, statement /* stmt */ ) mutable
	{
		if ( table == target )
		{
			runloop::shared().dispatch( [=]() mutable
			{
				if ( check_stmt )
				{
					auto action = action_type::insert;
					auto stmt	= statement( check_stmt.get(), false );
					auto err	= checker( check_stmt.get(), oid );

					if ( err == errc::row )
					{
						assert( !context->bitmap.is_bit_set( oid ) );
						context->bitmap.set_bit( oid );
									
						handler( action, oid, stmt );
					}
					else if ( ( err == errc::done ) && context->bitmap.is_bit_set( oid ) )
					{
						context->bitmap.clear_bit( oid );
						handler( action_type::remove, oid, stmt );
					}
				}
			} );
		}
	} );

	context->update = on( "update", [=]( std::string target, std::int64_t oid, statement before, statement after ) mutable
	{
		nunused( before );
		nunused( after );

		if ( table == target )
		{
			runloop::shared().dispatch( [=]() mutable
			{
				if ( check_stmt )
				{
					auto action = action_type::update;
					auto stmt	= statement( check_stmt.get(), false );
					auto err	= checker( check_stmt.get(), oid );

					if ( err == errc::row )
					{
						if ( !context->bitmap.is_bit_set( oid ) )
						{
							context->bitmap.set_bit( oid );
							action = action_type::insert;
						}
										
						handler( action, oid, stmt );
					}
					else if ( err == errc::done )
					{
						if ( context->bitmap.is_bit_set( oid ) )
						{
							context->bitmap.clear_bit( oid );
							handler( action_type::remove, oid, stmt );
						}
					}
				}
			} );
		}
	} );

	context->remove = on( "delete", [=]( std::string target, std::int64_t oid, statement /* stmt */ ) mutable
	{
		if ( table == target )
		{
			runloop::shared().dispatch( [=]() mutable
			{
				if ( check_stmt )
				{
					if ( context->bitmap.is_bit_set( oid ) )
					{
						auto stmt = statement( check_stmt.get(), false );

						context->bitmap.clear_bit( oid );
						handler( action_type::remove, oid, stmt );
					}
				}
			} );
		}
	} );

exit:
	
	return scoped_operation::create( [=]( void *v ) mutable
	{
		nunused( v );

		remove_listener( "insert", context->insert );
		remove_listener( "update", context->update );
		remove_listener( "delete", context->remove );

		check_stmt.reset();
	} );
}



std::error_code
database::start_transaction()
{
	return exec( "BEGIN TRANSACTION;" );
}
	

std::error_code
database::end_transaction()
{
	return exec( "COMMIT;" );
}

	
std::error_code
database::cancel_transaction()
{
	return exec( "ROLLBACK;" );
}


std::uint32_t
database::version() const
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
database::set_version( std::uint32_t version )
{
	int return_std_error_code;

	char				*error = nullptr;
	std::ostringstream	os;

	os << "PRAGMA user_version = " << version << ";";

	int ret = sqlite3_exec( m_db, os.str().c_str(), 0, 0, &error );
}


std::error_code
database::backup( const std::string &dest, backup_hook_f hook )
{
	sqlite3_backup	*backup		= nullptr;
	sqlite3			*dest_db	= nullptr;
	auto			ret			= std::error_code();

	ret = make_error_code( static_cast< errc >( sqlite3_open( dest.c_str(), &dest_db ) ) );

	if ( ret )
	{
		goto exit;
	}

	if ( hook )
	{
		std::string s = hook();

		char *error = nullptr;
	
		ret = make_error_code( static_cast< errc >( sqlite3_exec( dest_db, s.c_str(), 0, 0, &error ) ) );
	
		if ( ret )
		{
			goto exit;
		}
	}

	backup = sqlite3_backup_init( dest_db, "main", m_db, "main" );

	if ( !backup )
	{
		ret = make_error_code( static_cast< errc >( sqlite3_errcode( dest_db ) ) );
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
database::restore( const std::string &from, restore_hook_f hook )
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

	ret = make_error_code( static_cast< errc >( sqlite3_close( m_db ) ) );

	if ( ret )
	{
		goto exit;
	}

	ret = make_error_code( static_cast< errc >( sqlite3_open( filename.c_str(), &m_db ) ) );

	if ( ret )
	{
		goto exit;
	}

	if ( hook )
	{
		std::string s = hook();

		char *error = nullptr;
	
		ret = make_error_code( static_cast< errc >( sqlite3_exec( m_db, s.c_str(), 0, 0, &error ) ) );
	
		if ( ret )
		{
			goto exit;
		}
	}

exit:

	return ret;
}


std::error_code
database::backup_database( const filesystem::path& backup_path, const std::vector<std::string>& statements, std::size_t step_size, int sleep_ms, std::atomic<bool>& exiting )
{
	auto ret = std::error_code();
	
	sqlite3* backup_db;
	sqlite3_backup* backup_info;
	
	ret = make_error_code( static_cast< errc >( sqlite3_open( backup_path.c_str(), &backup_db ) ) );
	
	ncheck_error( !ret, exit );

	for ( auto statement : statements )
	{
		char *error	= nullptr;
		
		ret = make_error_code( static_cast< errc >( sqlite3_exec( backup_db, statement.c_str(), nullptr, nullptr, &error ) ) );

		if ( ret )
		{
			int emit_error;

			if ( error )
			{
				sqlite3_free( error );
			}
			
			goto close_backup;
		}
	}

	backup_info = sqlite3_backup_init( backup_db, "main", m_db, "main" );

	ncheck_error_action( backup_info, ret = make_error_code( std::errc::invalid_argument ), close_backup );
	
	do
	{
		ret = make_error_code( static_cast< errc >( sqlite3_backup_step(backup_info, static_cast< int >( step_size ) ) ) );
		
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

	ret = make_error_code( static_cast< errc >( sqlite3_backup_finish( backup_info ) ) );
	
close_backup:

	ret = make_error_code( static_cast< errc >( sqlite3_close( backup_db ) ) );
	
exit:

	return ret;
}


database::oid_type
database::last_oid() const
{
	return m_last_oid;
}


std::unordered_map< std::string, database::statement >::iterator
database::tracked_prepared_statement( const std::string &str )
{
	sqlite3_stmt	*stmt;
	auto			ret = m_prepared_statement_map.end();

	auto err = make_error_code( static_cast< database::errc >( sqlite3_prepare_v2( m_db, str.c_str(), -1, &stmt, nullptr ) ) );

	if ( !err )
	{
		m_prepared_statement_map.emplace( std::piecewise_construct, std::forward_as_tuple( str ), std::forward_as_tuple( stmt ) );
		ret = m_prepared_statement_map.find( str );
	}

	return ret;
}


database::statement
database::transient_prepared_statement( const std::string &str )
{
	sqlite3_stmt	*stmt;
	statement		ret;
	
	m_err = make_error_code( static_cast< errc >( sqlite3_prepare_v2( m_db, str.c_str(), -1, &stmt, nullptr ) ) );

	if ( !m_err )
	{
		ret = statement( stmt );
		m_prepared_statements.push_back( ret );
	}
	else
	{
		ret = statement( m_err );
	}
	
	return ret;
}

void
database::on_preupdate( void *v, sqlite3 *db, int op, char const *z_db, char const *z_name, std::int64_t key1, std::int64_t key2 )
{
	nunused( z_name );

	database *self = reinterpret_cast< database* >( v );

	if ( std::strcmp( z_db, "main" ) == 0 )
	{
		statement	before( db, sqlite3_preupdate_old );
		statement	after( db, sqlite3_preupdate_new );
		std::string db( z_name );

		switch ( op )
		{
			case SQLITE_INSERT:
			{
				self->emit( "insert", db, key2, after );
			}
			break;
			
			case SQLITE_UPDATE:
			{
				self->emit( "update", db, key2, before, after );
			}
			break;
			
			case SQLITE_DELETE:
			{
				self->emit( "delete", db, key1, before );
			}
			break;
		}
		
		self->m_last_oid = key2;
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
database::last_error()
{
	return m_err;
}

void
database::clear_error()
{
	m_err = make_error_code( database::errc::ok );
}


#if defined( __APPLE__ )
#	pragma mark database::statement implementation
#endif

database::statement::statement( sqlite3_stmt *stmt, bool finalize )
:
	m_shared( new shared )
{
	m_shared->stmt		= stmt;
	m_shared->finalize	= finalize;
	m_shared->db		= nullptr;
	m_shared->getter	= nullptr;
	m_shared->refs		= 1;
}
	

database::statement::statement( sqlite3 * db, preupdate_getter_type getter )
:
	m_shared( new shared )
{
	m_shared->stmt		= nullptr;
	m_shared->finalize	= false;
	m_shared->db		= db;
	m_shared->getter	= getter;
	m_shared->refs		= 1;
}


database::statement::statement( std::error_code err )
:
	m_shared( new shared )
{
	m_shared->stmt		= nullptr;
	m_shared->finalize	= false;
	m_shared->db		= nullptr;
	m_shared->getter	= nullptr;
	m_shared->err		= err;
	m_shared->refs		= 1;
}


void
database::statement::finalize()
{
	if ( m_shared->stmt )
	{
		sqlite3_finalize( m_shared->stmt );
		m_shared->stmt = nullptr;
	}
}


std::error_code
database::statement::last_error() const
{
	auto err = m_shared->err;;
		
	if ( err )
	{
		const_cast< statement* >( this )->reset_prepared();
	}
		
	return err;
}
	

bool
database::statement::step()
{
	bool ok = true;

	if ( !m_shared->stmt )
	{
		m_shared->err = make_error_code( database::errc::internal_error );
		ok = false;
	}
	else
	{
		auto err = make_error_code( static_cast< errc >( sqlite3_step( m_shared->stmt ) ) );
			
		if ( err == errc::row )
		{
			m_shared->err = make_error_code( errc::ok );
			ok = true;
		}
		else if ( err == errc::ok || err == errc::done )
		{
			m_shared->err = make_error_code( errc::ok );
			ok = false;
		}
		else
		{
			m_shared->err = err;
			ok = false;
		}
	}

	return ok;
}

	
std::error_code
database::statement::reset_prepared()
{
	if ( m_shared->stmt )
	{
		sqlite3_reset( m_shared->stmt );
		sqlite3_clear_bindings( m_shared->stmt );
		m_shared->err = make_error_code( database::errc::ok );
	}
	else
	{
		m_shared->err = make_error_code( database::errc::internal_error );
	}
		
	return m_shared->err;
}
	

/*
 * exec_prepared should be used on prepared statments that return no data--more specifically, 
 * on update or insert statements. The expected result is, in general, SQLITE_DONE, although
 * SQLITE_OK and SQLITE_ROW are not considered errors. After stepping, the statement is unconditionally
 * reset and bindings are cleared.
 *
 */
std::error_code
database::statement::exec_prepared()
{
	if ( m_shared->stmt )
	{
		auto result = SQLITE_OK;
		auto count	= 0;

		do
		{
			result = sqlite3_step( m_shared->stmt );
		}
		while ( ( result == SQLITE_SCHEMA ) && ( ++count < 10 ) );

		sqlite3_reset( m_shared->stmt );
		sqlite3_clear_bindings( m_shared->stmt );

		if ( result == SQLITE_DONE || result == SQLITE_OK || result == SQLITE_ROW )
		{
			m_shared->err = make_error_code( database::errc::ok );
		}
		else
		{
			m_shared->err = make_error_code( static_cast< database::errc >( result ) );
		}
	}
	else
	{
		m_shared->err = make_error_code( database::errc::internal_error );
	}

	return m_shared->err;
}


bool
database::statement::bool_at_column( int col ) const
{
	auto ret = false;

	if ( m_shared->stmt )
	{
		ret = sqlite3_column_int( m_shared->stmt, col ) ? true : false;
	}
	else if ( m_shared->getter )
	{
		sqlite3_value *v;
		m_shared->getter( m_shared->db, col, &v );
		ret = sqlite3_value_int( v ) ? true : false;
	}

	return ret;
}


int
database::statement::int_at_column( int col ) const
{
	int ret = 0;

	if ( m_shared->stmt )
	{
		ret = static_cast< int >( sqlite3_column_int( m_shared->stmt, col ) );
	}
	else if ( m_shared->getter )
	{
		sqlite3_value *v;
		m_shared->getter( m_shared->db, col, &v );
		ret = static_cast< int >( sqlite3_value_int( v ) );
	}

	return ret;
}


std::int64_t
database::statement::int64_at_column( int col ) const
{
	std::int64_t ret = 0;

	if ( m_shared->stmt )
	{
		ret = static_cast< std::int64_t >( sqlite3_column_int64( m_shared->stmt, col ) );
	}
	else if ( m_shared->getter )
	{
		sqlite3_value *v;
		m_shared->getter( m_shared->db, col, &v );
		ret = static_cast< std::int64_t >( sqlite3_value_int64( v ) );
	}

	return ret;
}


std::uint64_t
database::statement::uint64_at_column( int col ) const
{
	std::uint64_t ret = 0;

	if ( m_shared->stmt )
	{
		ret = static_cast< std::uint64_t >( sqlite3_column_int64( m_shared->stmt, col ) );
	}
	else if ( m_shared->getter )
	{
		sqlite3_value *v;
		m_shared->getter( m_shared->db, col, &v );
		ret = static_cast< std::uint64_t >( sqlite3_value_int64( v ) );
	}

	return ret;
}


std::chrono::system_clock::time_point
database::statement::time_at_column( int col ) const
{
	std::chrono::system_clock::time_point ret;

	if ( m_shared->stmt )
	{
		ret = std::chrono::system_clock::from_time_t( static_cast< std::time_t >( sqlite3_column_int64( m_shared->stmt, col ) ) );
	}
	else if ( m_shared->getter )
	{
		sqlite3_value *v;
		m_shared->getter( m_shared->db, col, &v );
		ret = std::chrono::system_clock::from_time_t( static_cast< std::time_t >( sqlite3_value_int64( v ) ) );
	}

	return ret;
}


std::string
database::statement::text_at_column( int col ) const
{
	std::string ret;

	if ( m_shared->stmt )
	{
		auto text = reinterpret_cast< const char* >( sqlite3_column_text( m_shared->stmt, col ) );
		ret = ( text ) ? text : std::string();
	}
	else if ( m_shared->getter )
	{
		sqlite3_value *v;
		m_shared->getter( m_shared->db, col, &v );
		auto text = reinterpret_cast< const char* >( sqlite3_value_text( v ) );
		ret = ( text ) ? text : std::string();
	}

	return ret;
}


buffer
database::statement::blob_at_column( int col ) const
{
	buffer ret;

	if ( m_shared->stmt )
	{
		auto blob = sqlite3_column_blob( m_shared->stmt, col );

		if ( blob )
		{
			std::size_t bytes = sqlite3_column_bytes( m_shared->stmt, col );
			ret = buffer( blob, bytes );
		}
	}
	else if ( m_shared->getter )
	{
		sqlite3_value *v;

		m_shared->getter( m_shared->db, col, &v );
	
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

	return ret;
}


bool
database::statement::set_bool( bool value, const std::string& var_name )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );
	
	if ( m_shared->stmt )
	{
		auto index = sqlite3_bind_parameter_index( m_shared->stmt, var_name.c_str() );
		if ( index < 1 )
		{
			ok = false;
			m_shared->err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			auto result = sqlite3_bind_int( m_shared->stmt, index, value ? 1 : 0 );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_bool( bool value, int var_index )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto result = sqlite3_bind_int( m_shared->stmt, var_index, value ? 1 : 0 );
		if ( result != SQLITE_OK )
		{
			ok = false;
			m_shared->err = make_error_code( static_cast< database::errc >( result ) );
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_int( int value, const std::string& var_name )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto index = sqlite3_bind_parameter_index( m_shared->stmt, var_name.c_str() );
		if ( index < 1 )
		{
			ok = false;
			m_shared->err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			auto result = sqlite3_bind_int( m_shared->stmt, index, value );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_int( int value, int var_index )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto result = sqlite3_bind_int( m_shared->stmt, var_index, value );
		if ( result != SQLITE_OK )
		{
			ok = false;
			m_shared->err = make_error_code( static_cast< database::errc >( result ) );
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_int64( std::int64_t value, const std::string& var_name )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto index = sqlite3_bind_parameter_index( m_shared->stmt, var_name.c_str() );
		if ( index < 1 )
		{
			ok = false;
			m_shared->err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			auto result = sqlite3_bind_int64( m_shared->stmt, index, value );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_int64( std::int64_t value, int var_index )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto result = sqlite3_bind_int64( m_shared->stmt, var_index, value );
		if ( result != SQLITE_OK )
		{
			ok = false;
			m_shared->err = make_error_code( static_cast< database::errc >( result ) );
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_uint64( std::uint64_t value, const std::string& var_name )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto index = sqlite3_bind_parameter_index( m_shared->stmt, var_name.c_str() );
		if ( index < 1 )
		{
			ok = false;
			m_shared->err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			auto result = sqlite3_bind_int64( m_shared->stmt, index, value );
			if (  result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_uint64( std::uint64_t value, int var_index )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );
	
	if ( m_shared->stmt )
	{
		auto result = sqlite3_bind_int64( m_shared->stmt, var_index, value );
		
		if ( result != SQLITE_OK )
		{
			ok = false;
			m_shared->err = make_error_code( static_cast< database::errc >( result ) );
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_time( std::chrono::system_clock::time_point value, const std::string& var_name )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto index = sqlite3_bind_parameter_index( m_shared->stmt, var_name.c_str() );
		
		std::int64_t timeval = std::chrono::system_clock::to_time_t( value );
		
		if ( index < 1 )
		{
			ok = false;
			m_shared->err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			auto result = sqlite3_bind_int64( m_shared->stmt, index, timeval );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_time( std::chrono::system_clock::time_point value, int var_index )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		std::int64_t timeval = std::chrono::system_clock::to_time_t( value );
		auto result = sqlite3_bind_int64( m_shared->stmt, var_index, timeval );
		if ( result != SQLITE_OK )
		{
			ok = false;
			m_shared->err = make_error_code( static_cast< database::errc >( result ) );
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_text( const std::string& value, const std::string& var_name )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto index = sqlite3_bind_parameter_index( m_shared->stmt, var_name.c_str() );
		if ( index < 1 )
		{
			ok = false;
			m_shared->err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			auto result = sqlite3_bind_text( m_shared->stmt, index, value.c_str(), -1, SQLITE_TRANSIENT );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_text( const std::string& value, int var_index )
{
	if ( m_shared->stmt )
	{
		m_shared->err = make_error_code( static_cast< errc >( sqlite3_bind_text( m_shared->stmt, var_index, value.c_str(), -1, SQLITE_TRANSIENT ) ) );
	}
	else
	{
		m_shared->err = make_error_code( database::errc::internal_error );
	}

	return ( !m_shared->err ) ? true : false;;
}


bool
database::statement::set_blob( nodeoze::buffer& value, const std::string& var_name )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		auto index = sqlite3_bind_parameter_index( m_shared->stmt, var_name.c_str() );
		if ( index < 1 )
		{
			ok = false;
			m_shared->err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			if ( value.size() < 1 )
			{
				auto result = sqlite3_bind_blob( m_shared->stmt, index, nullptr, 0, nullptr );
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_shared->err = make_error_code( static_cast< database::errc >( result ) );
				}
			}
			else
			{
				auto bufsize = value.size();
				auto bufptr = value.detach();
				
				auto result = sqlite3_bind_blob( m_shared->stmt, index, bufptr, static_cast<int>(bufsize), [] ( void * ptr )
				{
					buffer::default_dealloc( reinterpret_cast< buffer::elem_type* >( ptr ) );
				} );
				
				if ( result != SQLITE_OK )
				{
					ok = false;
					m_shared->err = make_error_code( static_cast< database::errc >( result ) );
				}
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


bool
database::statement::set_blob( nodeoze::buffer& value, int var_index )
{
	auto ok = true;
	
	m_shared->err = make_error_code( database::errc::ok );

	if ( m_shared->stmt )
	{
		if ( value.size() < 1 )
		{
			auto result = sqlite3_bind_blob( m_shared->stmt, var_index, nullptr, 0, nullptr );
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
		else
		{
			auto bufsize = value.size();
			auto bufptr = value.detach();
				
			auto result = sqlite3_bind_blob( m_shared->stmt, var_index, bufptr, static_cast<int>(bufsize), [] ( void * ptr )
			{
				buffer::default_dealloc( reinterpret_cast< buffer::elem_type* >( ptr ) );
			} );
			
			if ( result != SQLITE_OK )
			{
				ok = false;
				m_shared->err = make_error_code( static_cast< database::errc >( result ) );
			}
		}
	}
	else
	{
		ok = false;
		m_shared->err = make_error_code( database::errc::internal_error );
	}
	return ok;
}


class database_category : public std::error_category
{
public:

	virtual const char*
	name() const noexcept override
    {
		return "database";
    }
	
    virtual std::string
	message( int value ) const override
    {
		std::ostringstream os;
		
		switch ( static_cast< database::errc >( value ) )
        {
			case database::errc::ok:
			{
				os << "ok";
			}
			break;
			
			case database::errc::error:
			{
				os << "sql error or missing database";
			}
			break;
			
			case database::errc::internal_error:
			{
				os << "internal logic error";
			}
			break;
			
			case database::errc::permission_denied:
			{
				os << "access permission denied";
			}
			break;
			
			case database::errc::abort:
			{
				os << "callback routine requested an abort";
			}
			break;
			
			case database::errc::busy:
			{
				os << "database file is locked";
			}
			break;
			
			case database::errc::locked:
			{
				os << "a table in the database is locked";
			}
			break;
			
			case database::errc::out_of_memory:
			{
				os << "malloc failed";
			}
			break;
			
			case database::errc::read_only:
			{
				os << "attempt to write a readonly database";
			}
			break;
			
			case database::errc::interrupt:
			{
				os << "operation terminated by interrupt";
			}
			break;
			
			case database::errc::io_error:
			{
				os << "some kind of disk i/o error occurred";
			}
			break;
			
			case database::errc::corrupt:
			{
				os << "the disk image is malformed";
			}
			break;
			
			case database::errc::not_found:
			{
				os << "unknown opcode";
			}
			break;
			
			case database::errc::full:
			{
				os << "insertion failed because database is full";
			}
			break;
			
			case database::errc::cannot_open:
			{
				os << "unable to open database file";
			}
			break;
			
			case database::errc::protocol_error:
			{
				os << "database lock protocol error";
			}
			break;
			
			case database::errc::empty:
			{
				os << "database is empty";
			}
			break;
			
			case database::errc::schema_changed:
			{
				os << "database schema changed";
			}
			break;
			
			case database::errc::too_big:
			{
				os << "string or blob exceeds size limit";
			}
			break;
			
			case database::errc::constraint_violation:
			{
				os << "abort due to constraint violation";
			}
			break;
			
			case database::errc::mismatch:
			{
				os << "data type mismatch";
			}
			break;
			
			case database::errc::misuse:
			{
				os << "library used incorrectly";
			}
			
			case database::errc::no_lfs:
			{
				os << "uses os features not supported on host";
			}
			break;
			
			case database::errc::auth_denied:
			{
				os << "authorization denied";
			}
			break;
			
			case database::errc::format:
			{
				os << "auxiliary database format error";
			}
			break;
			
			case database::errc::range:
			{
				os << "2nd parameter to bind out of range";
			}
			break;
			
			case database::errc::not_a_db_file:
			{
				os << "not a database file";
			}
			break;
			
			case database::errc::notice:
			{
				os << "notifications from log";
			}
			break;
			
			case database::errc::warning:
			{
				os << "warnings from log";
			}
			break;
			
			case database::errc::row:
			{
				os << "another row is ready";
			}
			break;
			
			case database::errc::done:
			{
				os << "finished executing";
			}
			break;
		}
		
		return os.str();
    }
};

const std::error_category&
database::error_category()
{
	static class database_category instance;
    return instance;
}


TEST_CASE( "nodeoze/smoke/database" )
{
	database db( ":memory:" );
	db.exec( "CREATE TABLE test(oid INTEGER PRIMARY KEY AUTOINCREMENT, message TEXT, count INTEGER);" );
	db.exec( "INSERT into test VALUES( NULL, \"hello world\", 1 );" );

	SUBCASE( "select" )
	{
		auto stmt = db.select( "SELECT * FROM test WHERE count = 1;" );
		CHECK( stmt );
		CHECK( stmt.step() );
		CHECK( stmt.int64_at_column( 0 ) == 1 );
		CHECK( stmt.text_at_column( 1 ) == "hello world" );
		CHECK( stmt.int_at_column( 2 ) == 1 );
	}

	SUBCASE( "simple continuous select" )
	{
		bool	got_callback = false;
		int		check = 0;
		
		auto select = db.continuous_select( "*", "test", "WHERE count = 1", [&]( database::action_type action, database::oid_type oid, database::statement &stmt ) mutable
		{
			nunused( oid );
			
			got_callback = true;
			
			switch ( check )
			{
				case 0:
				{
					CHECK( action == database::action_type::insert );
					CHECK( stmt.int64_at_column( 0 ) == 1 );
					CHECK( stmt.text_at_column( 1 ) == "hello world" );
					CHECK( stmt.int_at_column( 2 ) == 1 );
				}
				break;
				
				case 1:
				{
					CHECK( action == database::action_type::insert );
					CHECK( stmt.int64_at_column( 0 ) == 3 );
					CHECK( stmt.text_at_column( 1 ) == "hello world 3" );
					CHECK( stmt.int_at_column( 2 ) == 1 );
				}
				break;
			}
		} );
		
		CHECK( got_callback );
		
		check++;
		got_callback = false;

		db.exec( "INSERT into test VALUES( NULL, \"hello world 2\", 2 );" );
		
		CHECK( !got_callback );
		
		db.exec( "INSERT into test VALUES( NULL, \"hello world 3\", 1 );" );
		
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		
		CHECK( got_callback );

		select.reset();
	}

	SUBCASE( "value" )
	{
		auto s = database::value( std::string( "test" ) );
		CHECK( s == "'test'" );
		s = database::value( std::string( "test" ), false );
		CHECK( s == "test" );
	}

	SUBCASE( "prepare template func" )
	{
		auto err = db.prepare( std::string( "SELECT * from test WHERE oid = 1" ), [&]( auto &statement ) mutable
		{
			CHECK( statement.step() );

			return std::error_code();
		} );

		CHECK( !err );
	}

	SUBCASE( "prepare template func reentrant" )
	{
		db.exec( "INSERT into test VALUES( NULL, \"hello world 2\", 2 );" );

		CHECK( db.count( "test" ) == 2 );

		auto err = db.prepare( std::string( "SELECT * from test WHERE message like ?" ), [&]( auto &statement ) mutable
		{
			statement.set_text( "%hello%", 1 );

			CHECK( statement.step() );
			CHECK( statement.int64_at_column( 0 ) == 1 );

			auto err = db.prepare( std::string( "SELECT * from test WHERE message like ?" ), [&]( auto &statement ) mutable
			{
				statement.set_text( "%hello%", 1 );

				CHECK( statement.step() );
				CHECK( statement.int64_at_column( 0 ) == 1 );
				CHECK( statement.step() );
				CHECK( statement.int64_at_column( 0 ) == 2 );

				return std::error_code();
			} );

			CHECK( !err );

			CHECK( statement.step() );
			CHECK( statement.int64_at_column( 0 ) == 2 );

			return std::error_code();
		} );

		CHECK( !err );
	}
}

TEST_CASE( "nodeoze/scalability/database" )
{
	database db( ":memory:" );
	db.exec( "CREATE TABLE test(oid INTEGER PRIMARY KEY AUTOINCREMENT, message TEXT, count INTEGER);" );
	db.exec( "INSERT into test VALUES( NULL, \"hello world\", 1 );" );

	SUBCASE( "heavy continuous select" )
	{
		const int						tries			= 10000;
		int								got_callback	= 0;
		int								check			= 0;
		std::vector< scoped_operation >	selects;

		for ( auto i = 0; i < tries; i++ )
		{
			selects.emplace_back( db.continuous_select( "*", "test", "WHERE count = 1", [&]( database::action_type action, database::oid_type oid, database::statement &stmt ) mutable
			{
				nunused( oid );
				
				got_callback++;
				
				switch ( check )
				{
					case 0:
					{
						CHECK( action == database::action_type::insert );
						CHECK( stmt.int64_at_column( 0 ) == 1 );
						CHECK( stmt.text_at_column( 1 ) == "hello world" );
						CHECK( stmt.int_at_column( 2 ) == 1 );
					}
					break;
					
					case 1:
					{
						CHECK( action == database::action_type::insert );
						CHECK( stmt.int64_at_column( 0 ) == 3 );
						CHECK( stmt.text_at_column( 1 ) == "hello world 3" );
						CHECK( stmt.int_at_column( 2 ) == 1 );
					}
					break;
				}
			} ) );
		}
		
		CHECK( got_callback == tries );
		
		check++;
		got_callback = 0;
		
		db.exec( "INSERT into test VALUES( NULL, \"hello world 2\", 2 );" );
		
		CHECK( got_callback == 0 );
		
		db.exec( "INSERT into test VALUES( NULL, \"hello world 3\", 1 );" );
		
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		
		CHECK( got_callback == tries );
	}
}
