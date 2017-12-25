/*
 * Copyright (c) 2013-2017, Collobos Software Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef _nodeoze_thread_h
#define _nodeoze_thread_h

#include <thread>
#include <mutex>
#if defined( __linux__ )
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
static inline pid_t
gettid()
{
	return syscall( __NR_gettid );
}
#endif

namespace nodeoze {

class thread
{
public:

	static inline std::uint64_t
	id()
	{
#if defined( WIN32 )
		return GetCurrentThreadId();
#elif defined( __APPLE__ )
		std::uint64_t tid;
		pthread_threadid_np( nullptr, &tid );
		return tid;
#elif defined( __linux__ )
		return gettid();
#else
		return 0;
#endif
	}

	typedef std::function< void ( void ) > func_f;

	thread()
	:
		m_thread( nullptr )
	{
	}

	~thread()
	{
		if ( m_thread )
		{
			delete m_thread;
		}
	}

	template < typename ...Params >
	void
	start( Params&&... params )
	{
		m_thread = new std::thread( std::forward< Params >( params )... );
	}

	void
	join()
	{
		if ( m_thread )
		{
			m_thread->join();
		}
	}

	void
	detach()
	{
		if ( m_thread )
		{
			m_thread->detach();
		}
	}

private:

	std::thread *m_thread;
};


class guard
{
public:

	template < class F >
	guard( std::mutex &mutex, F func )
	{
		mutex.lock();
		func();
		mutex.unlock();
	}
};

}

#endif
