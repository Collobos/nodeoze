#ifndef _nodeoze_keychain_win32_h
#define _nodeoze_keychain_win32_h

#include <nodeoze/nkeychain.h>

namespace nodeoze {

class keychain_win32 : public keychain
{
public:

	virtual std::error_code
	save( const uri &resource, const std::string &value );

	virtual std::error_code
	find( const uri &resource, std::string &value );
};

}

#endif
