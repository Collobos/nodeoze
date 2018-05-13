#ifndef _nodeoze_file_h
#define _nodeoze_file_h

#include <nodeoze/nevent.h>

namespace nodeoze {

namespace fs {

class reader : public stream::readable
{
public:

	reader::ptr
	create( const nodeoze::path &p );

	reader();
};

class writer : public stream::writable
{
public:

	writer::ptr
	create( const nodeoze::path &p );

	writer();
};

}

}

#endif
