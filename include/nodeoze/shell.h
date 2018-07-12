#pragma once

#include <nodeoze/printf.h>
#include <nodeoze/markers.h>
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
		return ::system( command.c_str() );
	}
};

}
