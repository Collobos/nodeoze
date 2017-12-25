#ifndef _nodeoze_zip_h
#define _nodeoze_zip_h

#include <functional>
#include <cstdint>
#include <memory>

namespace nodeoze {

class zip
{
public:

	typedef std::function< void ( const std::uint8_t *buf, std::size_t len ) > extract_f;
	typedef std::shared_ptr< zip > ptr;

	virtual ~zip()
	{
	}

	static zip::ptr
	create( const std::uint8_t *data, std::size_t len );

	virtual bool
	lookup( const std::string &name, std::uint32_t &index ) = 0;

	virtual bool
	extract( std::uint32_t index, extract_f func ) = 0;
};

}

#endif
