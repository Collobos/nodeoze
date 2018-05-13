#pragma once

#include <nodeoze/nprintf.h>
#include <nodeoze/nmarkers.h>
#include <stdlib.h>
#include <sstream>

namespace nodeoze {

class shell 
{
public:

	template< typename ...Params >
	static inline int
	execute( const char *format, const Params &... params )
	{
		std::ostringstream os;
			
		nodeoze::printf( os, format, params... );
		
		return execute( os.str() );
	}

	static inline int
	execute( const std::string &command )
	{
		auto ret = ::system( command.c_str() );
		mlog( marker::shell, log::level_t::info, "(%) %", ret, command );
		return ret;
	}
};

}
