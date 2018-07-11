#pragma once

#include <chrono>

namespace nodeoze {

/*
 * YO: internal
 */
class stopwatch
{
public:

	template< class F >
	static std::chrono::high_resolution_clock::duration
	run( F func )
	{
		auto t1 = std::chrono::high_resolution_clock::now();
		func();
		auto t2 = std::chrono::high_resolution_clock::now();
		return t2 - t1;
	}
};

}
