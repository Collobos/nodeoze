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

#include <nodeoze/runloop.h>
#include <nodeoze/macros.h>
#include <nodeoze/log.h>
#include <uv.h>
#include <assert.h>
#include <thread>

using namespace nodeoze;

struct uv_event
{
	enum class type_t
	{
		poll,
		timer,
		path
#if defined( WIN32 )
		,handle
#endif
	};
	
	uv_event( type_t t )
	:
		m_active( false ),
		m_type( t )
	{
		switch ( t )
		{
			case type_t::poll:
			{
				new ( &m_data.poll ) data::poll_s();
			}
			break;
			
			case type_t::timer:
			{
				new ( &m_data.timer ) data::timer_s();
			}
			break;
			
			case type_t::path:
			{
				new ( &m_data.path ) data::path_s();
			}
			break;
		}
	}
	
	~uv_event()
	{
		switch ( m_type )
		{
			case type_t::poll:
			{
				m_data.poll.data::poll_s::~poll_s();
			}
			break;
			
			case type_t::timer:
			{
				m_data.timer.data::timer_s::~timer_s();
			}
			break;
			
			case type_t::path:
			{
				m_data.path.data::path_s::~path_s();
			}
			break;
		}
	}
	
	union data
	{
		data()
		{
		}
		
		~data()
		{
		}
		
		struct poll_s
		{
			uv_poll_s			m_handle;
			runloop::event_f	m_callback;
			int					m_events;
		} poll;
		
		struct timer_s
		{
			uv_timer_s			m_handle;
			runloop::event_f	m_callback;
			std::uint64_t		m_msec;
			bool				m_repeat;
		} timer;
		
		struct path_s
		{
			uv_fs_event_s		m_handle;
			runloop::fs_event_f	m_callback;
			nodeoze::path		m_path;
		} path;

#if defined( WIN32 )
		struct handle_s
		{
			HANDLE				m_handle;
			runloop::event_f	m_callback;
			std::thread			*m_thread;
			HANDLE				m_stop;
		} handle;
#endif
	};
	
	data				m_data;
	bool				m_active;
	type_t				m_type;
};

NODEOZE_DEFINE_SINGLETON( runloop )

runloop*
runloop::create()
{
	return new runloop;
}


runloop::runloop()
:
	m_handle( new uv_async_s )
{
	uv_async_init( uv_default_loop(), reinterpret_cast< uv_async_t* > ( m_handle ), reinterpret_cast< uv_async_cb >( on_wakeup ) );
	reinterpret_cast< uv_async_t* >( m_handle )->data = this;
}


runloop::~runloop()
{
	nlog( log::level_t::debug, "" );
}


runloop::event
runloop::create( native_socket_type fd, int mask )
{
	uv_event	*event;
	int			err;
	
	event = new uv_event( uv_event::type_t::poll );
	err = uv_poll_init( uv_default_loop(), &event->m_data.poll.m_handle, fd );
	ncheck_error_action_quiet( err == 0, delete event; event = nullptr, exit );
	
	event->m_data.poll.m_handle.data = event;
	event->m_data.poll.m_events = 0;
	
	if ( mask & event_mask_t::read )
	{
		event->m_data.poll.m_events |= UV_READABLE;
	}
	
	if ( mask & event_mask_t::write )
	{
		event->m_data.poll.m_events |= UV_WRITABLE;
	}

exit:

	return event;
}


runloop::event
runloop::create( std::chrono::milliseconds msec )
{
	uv_event	*event;
	int			err;
	
	event = new uv_event( uv_event::type_t::timer );
	err = uv_timer_init( uv_default_loop(), &event->m_data.timer.m_handle );
	ncheck_error_action_quiet( err == 0, delete event; event = nullptr, exit );
	
	event->m_data.timer.m_handle.data	= event;
	event->m_data.timer.m_msec			= msec.count();
	event->m_data.timer.m_repeat		= true;
	
exit:

	return event;
}


runloop::event
runloop::create( const nodeoze::path &path )
{
	uv_event	*event;
	int			err;
	
	event = new uv_event( uv_event::type_t::path );
	event->m_data.path.m_path = path;
	err = uv_fs_event_init( uv_default_loop(), &event->m_data.path.m_handle );
	ncheck_error_action( err == 0, delete event; event = nullptr, exit, "uv_fs_event_init() failed" );
	
exit:

	return event;
}


#if defined( WIN32 )
runloop::event
runloop::create( HANDLE h )
{
	uv_event *event;

	event = new uv_event( uv_event::type_t::handle );
	event->m_data.handle.m_thread	= nullptr;
	event->m_data.handle.m_handle	= h;
	event->m_data.handle.m_stop		= CreateEvent( nullptr, FALSE, FALSE, nullptr );

	return event;
}
#endif


void
runloop::schedule( event e, event_f func )
{
	uv_event *event = reinterpret_cast< uv_event* >( e );
	
	ncheck_error_quiet( event, exit );
	
	schedule( e, std::chrono::milliseconds( event->m_data.timer.m_msec), func );
	
exit:

	return;
}


void
runloop::schedule( event e, std::chrono::milliseconds msec, event_f func )
{
	uv_event *event = reinterpret_cast< uv_event* >( e );
	
	ncheck_error_quiet( event, exit );
	
	switch ( event->m_type )
	{
		case uv_event::type_t::poll:
		{
			event->m_data.poll.m_callback = func;
			uv_poll_start( &event->m_data.poll.m_handle, event->m_data.poll.m_events, reinterpret_cast< uv_poll_cb >( on_poll ) );
		}
		break;
		
		case uv_event::type_t::timer:
		{
			event->m_data.timer.m_callback = func;
			uv_timer_start( &event->m_data.timer.m_handle, reinterpret_cast< uv_timer_cb >( on_timer ), msec.count(), msec.count() );
		}
		break;
		
#if defined( WIN32 )
		case uv_event::type_t::handle:
		{
			event->m_data.handle.m_thread = new std::thread( [=]()
			{
				HANDLE handles[ 2 ] = { event->m_data.handle.m_handle, event->m_data.handle.m_stop };

				auto ret = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );

				if ( ret == WAIT_OBJECT_0 )
				{
					dispatch( [=]()
					{
						func( e );

						if ( event->m_data.handle.m_thread )
						{
							event->m_data.handle.m_thread->join();
							delete event->m_data.handle.m_thread;
							schedule( e, msec, func );
						}
					} );
				}
				else if ( ret != WAIT_OBJECT_0 + 1 )
				{
					nlog( log::level_t::error, "WaitForMultipleObjects() returned % (%)", ret, ::GetLastError() );
				}
			} );
		}
		break;
#endif

		default:
		{
			assert( 0 );
		}
	}
	
	event->m_active = true;
	
exit:

	return;
}


void
runloop::schedule_oneshot_timer( std::chrono::milliseconds msec, event_f func )
{
	uv_event	*event;
	int			err;
	
	event = new uv_event( uv_event::type_t::timer );
	err = uv_timer_init( uv_default_loop(), &event->m_data.timer.m_handle );
	ncheck_error_action_quiet( err == 0, delete event; event = nullptr, exit );
	
	event->m_data.timer.m_handle.data	= event;
	event->m_data.timer.m_msec			= msec.count();
	event->m_data.timer.m_repeat		= false;
	
	schedule( event, func );
	
exit:

	return;
}


void
runloop::schedule( event e, fs_event_f func )
{
	uv_event *event = reinterpret_cast< uv_event* >( e );
	
	ncheck_error( event, exit, "null event" );
	ncheck_error( event->m_type == uv_event::type_t::path, exit, "event type is wrong" );
	
	event->m_data.path.m_callback = func;
	uv_fs_event_start( &event->m_data.path.m_handle, reinterpret_cast< uv_fs_event_cb >( on_path ), event->m_data.path.m_path.to_string().c_str(), 0 );
	event->m_active = true;
	
exit:

	return;
}


void
runloop::suspend( event e )
{
	uv_event *event = reinterpret_cast< uv_event* >( e );
	
	ncheck_error_quiet( event, exit );
	
	switch ( event->m_type )
	{
		case uv_event::type_t::poll:
		{
			uv_poll_stop( &event->m_data.poll.m_handle );
		}
		break;
		
		case uv_event::type_t::timer:
		{
			uv_timer_stop( &event->m_data.timer.m_handle );
		}
		break;
		
		case uv_event::type_t::path:
		{
			uv_fs_event_stop( &event->m_data.path.m_handle );
		}

#if defined( WIN32 )
		case uv_event::type_t::handle:
		{
			if ( event->m_data.handle.m_thread != nullptr )
			{
				SetEvent( event->m_data.handle.m_stop );
				event->m_data.handle.m_thread->join();
				delete event->m_data.handle.m_thread;
				event->m_data.handle.m_thread = nullptr;
			}
		}
		break;
#endif
	}
	
	event->m_active = false;
	
exit:

	return;
}


void
runloop::cancel( event e )
{
	uv_event *event = reinterpret_cast< uv_event* >( e );
	
	ncheck_error_quiet( event, exit );
	
	if ( event->m_active )
	{
		switch ( event->m_type )
		{
			case uv_event::type_t::poll:
			{
				uv_poll_stop( &event->m_data.poll.m_handle );
				uv_close( reinterpret_cast< uv_handle_t* >( event ), []( uv_handle_t* handle )
				{
					uv_event *event = reinterpret_cast< uv_event* >( handle );
					assert( event );
					delete event;
				} );
			}
			break;
			
			case uv_event::type_t::timer:
			{
				uv_timer_stop( &event->m_data.timer.m_handle );
				uv_close( reinterpret_cast< uv_handle_t* >( event ), []( uv_handle_t* handle )
				{
					uv_event *event = reinterpret_cast< uv_event* >( handle );
					assert( event );
					delete event;
				} );
			}
			break;
			
			case uv_event::type_t::path:
			{
				uv_fs_event_stop( &event->m_data.path.m_handle );
				uv_close( reinterpret_cast< uv_handle_t* >( event ), []( uv_handle_t *handle )
				{
					uv_event *event = reinterpret_cast< uv_event* >( handle );
					assert( event );
					delete event;
				} );
			}
			break;

#if defined( WIN32 )
			case uv_event::type_t::handle:
			{
				SetEvent( event->m_data.handle.m_stop );
				event->m_data.handle.m_thread->join();
				delete event->m_data.handle.m_thread;
				event->m_data.handle.m_thread = nullptr;
			}
			break;
#endif
		}
	}
	
exit:

	return;
}


void
runloop::run( mode_t how )
{
	drain_dispatch_queue();

	switch ( how )
	{
		case mode_t::nowait:
		{
			uv_run( uv_default_loop(), UV_RUN_NOWAIT );
		}
		break;

		case mode_t::once:
		{
			uv_run( uv_default_loop(), UV_RUN_ONCE );
		}
		break;

		case mode_t::normal:
		{
			uv_run( uv_default_loop(), UV_RUN_DEFAULT );
		}
		break;
	}
}


void
runloop::dispatch( dispatch_f f )
{
	m_queue.emplace( std::move( f ) );
	auto ret = uv_async_send( reinterpret_cast< uv_async_t* >( m_handle ) );
	ncheck_error( ret == 0, exit, "uv_async_send() failed (%)", ret );

exit:

	return;
}


void
runloop::stop()
{
	uv_stop( uv_default_loop() );
}


void
runloop::on_wakeup( void *v, int status )
{
	nunused( status );
	
	auto		handle	= reinterpret_cast< uv_async_t* >( v );
	auto		self	= reinterpret_cast< runloop* >( handle->data );

	self->drain_dispatch_queue();
}


void
runloop::on_poll( void *v, int status, int events )
{
	nunused( status );
	nunused( events );
	
	uv_event *event = reinterpret_cast< uv_event* >( v );
	
	event->m_data.poll.m_callback( event );
}

	
void
runloop::on_timer( void *v, int status )
{
	nunused( status );
	
	uv_event *event = reinterpret_cast< uv_event* >( v );
	
	event->m_data.timer.m_callback( event );
	
	if ( !event->m_data.timer.m_repeat )
	{
		runloop::shared().cancel( event );
	}
}


void
runloop::on_path( void *v, const char *filename, int events, int status )
{
	uv_event *event = reinterpret_cast< uv_event* >( v );
	
	if ( filename )
	{
		auto absolute = event->m_data.path.m_path + std::string( filename );
		event->m_data.path.m_callback( event, absolute, events, status );
	}
}
	

void
runloop::drain_dispatch_queue()
{
	dispatch_f dispatch;

	while ( m_queue.try_pop( dispatch ) )
	{
		dispatch();
	}
}
