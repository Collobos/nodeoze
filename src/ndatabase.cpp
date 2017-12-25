#include <nodeoze/ndatabase.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/ntest.h>

using namespace nodeoze;

namespace nodeoze {

namespace database {

statement::~statement()
{
}


manager::~manager()
{
}


class category : public std::error_category
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
		
		switch ( static_cast< code_t >( value ) )
        {
			case code_t::ok:
			{
				os << "ok";
			}
			break;
			
			case code_t::error:
			{
				os << "sql error or missing database";
			}
			break;
			
			case code_t::internal_error:
			{
				os << "internal logic error";
			}
			break;
			
			case code_t::permission_denied:
			{
				os << "access permission denied";
			}
			break;
			
			case code_t::abort:
			{
				os << "callback routine requested an abort";
			}
			break;
			
			case code_t::busy:
			{
				os << "database file is locked";
			}
			break;
			
			case code_t::locked:
			{
				os << "a table in the database is locked";
			}
			break;
			
			case code_t::out_of_memory:
			{
				os << "malloc failed";
			}
			break;
			
			case code_t::read_only:
			{
				os << "attempt to write a readonly database";
			}
			break;
			
			case code_t::interrupt:
			{
				os << "operation terminated by interrupt";
			}
			break;
			
			case code_t::io_error:
			{
				os << "some kind of disk i/o error occurred";
			}
			break;
			
			case code_t::corrupt:
			{
				os << "the disk image is malformed";
			}
			break;
			
			case code_t::not_found:
			{
				os << "unknown opcode";
			}
			break;
			
			case code_t::full:
			{
				os << "insertion failed because database is full";
			}
			break;
			
			case code_t::cannot_open:
			{
				os << "unable to open database file";
			}
			break;
			
			case code_t::protocol_error:
			{
				os << "database lock protocol error";
			}
			break;
			
			case code_t::empty:
			{
				os << "database is empty";
			}
			break;
			
			case code_t::schema_changed:
			{
				os << "database schema changed";
			}
			break;
			
			case code_t::too_big:
			{
				os << "string or blob exceeds size limit";
			}
			break;
			
			case code_t::constraint_violation:
			{
				os << "abort due to constraint violation";
			}
			break;
			
			case code_t::mismatch:
			{
				os << "data type mismatch";
			}
			break;
			
			case code_t::misuse:
			{
				os << "library used incorrectly";
			}
			
			case code_t::no_lfs:
			{
				os << "uses os features not supported on host";
			}
			break;
			
			case code_t::auth_denied:
			{
				os << "authorization denied";
			}
			break;
			
			case code_t::format:
			{
				os << "auxiliary database format error";
			}
			break;
			
			case code_t::range:
			{
				os << "2nd parameter to bind out of range";
			}
			break;
			
			case code_t::not_a_db_file:
			{
				os << "not a database file";
			}
			break;
			
			case code_t::notice:
			{
				os << "notifications from log";
			}
			break;
			
			case code_t::warning:
			{
				os << "warnings from log";
			}
			break;
			
			case code_t::row:
			{
				os << "another row is ready";
			}
			break;
			
			case code_t::done:
			{
				os << "finished executing";
			}
			break;
		}
		
		return os.str();
    }
};

const std::error_category&
error_category()
{
	static class category instance;
    return instance;
}

}

}

static void
setup_database()
{
	static bool first = true;

	if ( first )
	{
		database::manager::shared().open( ":memory:" );
	
		database::manager::shared().exec( "CREATE TABLE test(oid INTEGER PRIMARY KEY AUTOINCREMENT, message TEXT, count INTEGER);" );
	}
	else
	{
		database::manager::shared().exec( "DELETE FROM test;" );
	}

	database::manager::shared().exec( "INSERT into test VALUES( NULL, \"hello world\", 1 );" );
}


TEST_CASE( "nodeoze: database" )
{
	setup_database();

	SUBCASE( "select" )
	{
		auto stmt = database::manager::shared().select( "SELECT * FROM test WHERE count = 1;" );
		CHECK( stmt->step() );
		CHECK( stmt->int64_at_column( 0 ) == 1 );
		CHECK( stmt->text_at_column( 1 ) == "hello world" );
		CHECK( stmt->int_at_column( 2 ) == 1 );
	}

	SUBCASE( "simple continuous select" )
	{
		bool	got_callback = false;
		int		check = 0;
		
		auto select = database::manager::shared().continuous_select( "*", "test", "WHERE count = 1", [&]( database::action_t action, oid_t oid, database::statement &stmt ) mutable
		{
			nunused( oid );
			
			got_callback = true;
			
			switch ( check )
			{
				case 0:
				{
					CHECK( action == database::action_t::insert );
					CHECK( stmt.int64_at_column( 0 ) == 1 );
					CHECK( stmt.text_at_column( 1 ) == "hello world" );
					CHECK( stmt.int_at_column( 2 ) == 1 );
				}
				break;
				
				case 1:
				{
					CHECK( action == database::action_t::insert );
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
		
		database::manager::shared().exec( "INSERT into test VALUES( NULL, \"hello world 2\", 2 );" );
		
		CHECK( !got_callback );
		
		database::manager::shared().exec( "INSERT into test VALUES( NULL, \"hello world 3\", 1 );" );
		
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		
		CHECK( got_callback );

		select.reset();
	}

	SUBCASE( "heavy continuous select" )
	{
		const int						tries			= 10000;
		int								got_callback	= 0;
		int								check			= 0;
		std::vector< scoped_operation >	selects;

		for ( auto i = 0; i < tries; i++ )
		{
			selects.emplace_back( database::manager::shared().continuous_select( "*", "test", "WHERE count = 1", [&]( database::action_t action, oid_t oid, database::statement &stmt ) mutable
			{
				nunused( oid );
				
				got_callback++;
				
				switch ( check )
				{
					case 0:
					{
						CHECK( action == database::action_t::insert );
						CHECK( stmt.int64_at_column( 0 ) == 1 );
						CHECK( stmt.text_at_column( 1 ) == "hello world" );
						CHECK( stmt.int_at_column( 2 ) == 1 );
					}
					break;
					
					case 1:
					{
						CHECK( action == database::action_t::insert );
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
		
		database::manager::shared().exec( "INSERT into test VALUES( NULL, \"hello world 2\", 2 );" );
		
		CHECK( got_callback == 0 );
		
		database::manager::shared().exec( "INSERT into test VALUES( NULL, \"hello world 3\", 1 );" );
		
		runloop::shared().run( runloop::mode_t::nowait );
		runloop::shared().run( runloop::mode_t::nowait );
		
		CHECK( got_callback == tries );
	}
}
