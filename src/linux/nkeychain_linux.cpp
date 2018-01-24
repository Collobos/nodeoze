#include "nkeychain_linux.h"

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark keychain_linux implementation
#endif

NODEOZE_DEFINE_SINGLETON( keychain )

keychain*
keychain::create()
{
	return new keychain_linux;
}

keychain_linux::keychain_linux()
{
}


keychain_linux::~keychain_linux()
{
}


std::error_code
keychain_linux::save( const uri &key, const std::string &value )
{
	return make_error_code( err_t::not_implemented );
}


std::error_code
keychain_linux::find( const uri &key, std::string &value )
{
	return make_error_code( err_t::not_implemented );
}

