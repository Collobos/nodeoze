#include "nkeychain_android.h"

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark keychain_android implementation
#endif

NODEOZE_DEFINE_SINGLETON( keychain )

keychain*
keychain::create()
{
	return new keychain_android;
}

keychain_android::keychain_android()
{
}


keychain_android::~keychain_android()
{
}


status_t
keychain_android::save( const uri &key, const std::string &value )
{
	return status_t::not_implemented;
}


status_t
keychain_android::find( const uri &key, std::string &value )
{
	return status_t::not_implemented;
}

