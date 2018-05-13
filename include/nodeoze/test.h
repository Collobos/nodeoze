#ifndef _nodeoze_test_h
#define _nodeoze_test_h

/*
 * doctest #includes windows.h, which causes problems
 * for us. so we'll include winsock2.h to fix the
 * problems
 */

#if defined( WIN32 )

#	include <winsock2.h>
#	undef max
#	undef min

#endif

#include <nodeoze/doctest.h>

#endif
