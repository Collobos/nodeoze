#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <nodeoze/ntest.h>
#include <nodeoze/nlog.h>

using namespace nodeoze;

class static_init
{
public:

	static_init()
	{
		log::init( "nodeoze test" );
		log::set_level( log::level_t::info );
	}
};

static static_init g_init;

