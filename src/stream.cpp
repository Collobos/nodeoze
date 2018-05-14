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

stream::base::base()
{
}


stream::base::~base()
{
}


stream::readable::readable()
{
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


stream::writable::writable()
{
}


stream::writable::~writable()
{
}


promise< void >
stream::writable::write( buffer b )
{
	return really_write( b );
}


promise< void >
stream::writable::really_write( buffer b )
{
	promise< void > ret;

	fprintf( stderr, "really_write: %s\n", b.to_string().c_str() );

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

