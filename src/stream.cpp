#include <nodeoze/stream2.h>
#include <nodeoze/test.h>

const char nodeoze::ostream::m_digits[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";


TEST_CASE( "nodeoze/smoke/stream" )
{
	SUBCASE( "int" )
	{
		nodeoze::ostringstream os;
		
		os << 550;
		
		CHECK( os.str() == "550" );
	}
}

