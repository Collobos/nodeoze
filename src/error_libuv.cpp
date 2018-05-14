#include "error_libuv.h"
#include <string>
#include <uv.h>

namespace nodeoze {

namespace libuv {

class category : public std::error_category
{
public:

	virtual const char*
	name() const noexcept override
    {
		return "libuv";
    }
	
    virtual std::string
	message( int value ) const override
    {
		return uv_strerror( value );
    }
};

const std::error_category&
error_category()
{
	static auto instance = new category();
    return *instance;
}

}

}

