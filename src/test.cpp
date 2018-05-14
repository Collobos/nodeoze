#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <nodeoze/test.h>
#include <nodeoze/log.h>

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

