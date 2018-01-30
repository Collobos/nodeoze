#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <nodeoze/ntest.h>
#include <nodeoze/nlog.h>

using namespace nodeoze;

class static_init
{
public:

	static_init()
	{
		log::shared().set_name( "nodeoze test" );
	}
};

static static_init g_init;

