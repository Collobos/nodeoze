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
 
#ifndef _nodeoze_runloop_h
#define _nodeoze_runloop_h

#include <nodeoze/concurrent.h>
#include <nodeoze/singleton.h>
#include <nodeoze/path.h>
#include <functional>
#include <chrono>

#if defined( WIN32 )
#	include <winsock2.h>
#endif

namespace nodeoze {

#if defined( WIN32 )
	typedef SOCKET	native_socket_type;
#else
	typedef int		native_socket_type;
#endif

class runloop
{
	NODEOZE_DECLARE_SINGLETON( runloop )
	
public:

	typedef void *event;
	
	struct event_mask_t
	{
		static const int read	= ( 1 << 0 );
		static const int write	= ( 1 << 1 );
	};

	enum class mode_t
	{
		normal			= 0,
		once			= 1,
		nowait			= 2
	};

	typedef std::function< void ( void ) >															dispatch_f;
	typedef std::function< void ( event e ) >														event_f;
	typedef std::function< void ( event e, const nodeoze::path &path, int events, int status ) >	fs_event_f;
	
	~runloop();
	
	event
	create( native_socket_type fd, int mask );

	event
	create( std::chrono::milliseconds msec );
	
	event
	create( const nodeoze::path &path );
	
#if defined( WIN32 )
	
	event
	create( HANDLE h );

#endif
	
	void
	schedule( event e, event_f func );

	void
	schedule( event e, std::chrono::milliseconds msec, event_f func );

	void
	schedule_oneshot_timer( std::chrono::milliseconds msec, event_f func );
	
	void
	schedule( event e, fs_event_f func );
	
	void
	suspend( event e );
	
	void
	cancel( event e );
	
	void
	dispatch( dispatch_f f );

	void
	run( mode_t how = mode_t::normal );
	
	void
	stop();
	
protected:

	runloop();

	static void
	on_wakeup( void *handle, int status );

	static void
	on_poll( void *handle, int status, int events );

	static void
	on_timer( void *handle, int status );
	
	static void
	on_path( void *handle, const char *filename, int events, int status );

	void
	drain_dispatch_queue();
	
	void										*m_handle;
	concurrent::queue< dispatch_f >				m_queue;
};

}

#endif
