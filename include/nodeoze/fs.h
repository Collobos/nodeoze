#ifndef _nodeoze_file_h
#define _nodeoze_file_h

#include <nodeoze/event.h>

namespace nodeoze {

namespace fs {

stream::readable::ptr
create_read_stream( path p );

stream::writable::ptr
create_write_stream( path p );

}

}

#endif
