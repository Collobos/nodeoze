#ifndef _nodeoze_fs_h
#define _nodeoze_fs_h

#include <nodeoze/stream.h>

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
