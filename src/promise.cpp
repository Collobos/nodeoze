#include <nodeoze/promise.h>
#include <nodeoze/runloop.h>
#include <nodeoze/test.h>
#include <cassert>

using namespace nodeoze;

static int
func()
{
	return 42;
}