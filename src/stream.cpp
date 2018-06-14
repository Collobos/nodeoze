#include <nodeoze/stream.h>
#include <nodeoze/stream2.h>
#include <nodeoze/test.h>

using namespace nodeoze;

const char nodeoze::ostream::m_digits[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

#if defined( __APPLE__ )
#	pragma mark stream::base
#endif

stream::base::base()
{
}


stream::base::~base()
{
}

#if defined( __APPLE__ )
#	pragma mark stream::readable
#endif

stream::readable::readable()
{
	on( "newListener", [=]( const char *key, std::size_t num_listeners ) mutable
	{
		if ( ( num_listeners == 1 ) && ( strcmp( key, "data" ) == 0 ) )
		{
			really_read();
		}

	} );

	on( "removeListener", [=]( const char *key, std::size_t num_listeners ) mutable
	{
		if ( ( num_listeners == 0 ) && ( strcmp( key, "data" ) == 0 ) )
		{
			really_pause();
		}
	} );
}


stream::readable::~readable()
{
}


void
stream::readable::push( buffer b ) 
{
	emit( "data", b );
}


void
stream::readable::really_read()
{
}


void
stream::readable::really_pause()
{
}

#if defined( __APPLE__ )
#	pragma mark stream::writable
#endif

stream::writable::writable()
{
}


stream::writable::~writable()
{
}


promise< void >
stream::writable::write( buffer b )
{
	auto ret = promise< void >();

	if ( !m_writing )
	{
		start_write( b, ret );
	}
	else
	{
		m_queue.emplace( std::make_pair( std::move( b ), ret ) );
	}

	return ret;
}


void
stream::writable::start_write( buffer b, promise< void > ret )
{
	m_writing = true;

	really_write( b )
	.then( [=]() mutable
	{
		ret.resolve();
	},
	[=]( auto err ) mutable
	{
		ret.reject( err, reject_context );
	} )
	.finally( [=]() mutable
	{
		if ( m_queue.size() > 0 )
		{
			auto pair = m_queue.front();

			m_queue.pop();

			start_write( pair.first, pair.second );
		}
		else
		{
			m_writing = false;

			emit( "drain" );
		}
	} );
}


promise< void >
stream::writable::really_write( buffer b )
{
	nunused( b );

	promise< void > ret;

	ret.resolve();

	return ret;
}


stream::duplex::duplex()
{
}


stream::duplex::~duplex()
{
}


stream::transform::transform()
{
}


stream::transform::~transform()
{
}



TEST_CASE( "nodeoze/smoke/stream" )
{
	SUBCASE( "pipe" )
	{
		auto readable = std::make_shared< stream::readable >();
		auto writable = std::make_shared< stream::writable >();

		readable->pipe( writable );

		readable->push( "hello world" );
	}

	SUBCASE( "int" )
	{
		nodeoze::ostringstream os;
		
		os << 550;
		
		CHECK( os.str() == "550" );
	}
}

