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

#pragma once

#include <nodeoze/nany.h>
#include <nodeoze/nscoped_operation.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/ndeque.h>
#include <nodeoze/nlog.h>
#include <unordered_map>
#include <vector>
#include <type_traits>

namespace nodeoze {

template< class State, class Event >
class state_machine
{
	static_assert( std::is_enum< State >::value, "must be an enum type" );
	static_assert( std::is_enum< Event >::value, "must be an enum type" );

public:

	template< class T >
	struct context_t
	{
	public:

		context_t( T value )
		:
			m_value( value )
		{
		}

		context_t( T value, nodeoze::any &&data )
		:
			m_value( value ),
			m_data( std::move( data ) )
		{
		}

		context_t( T value, std::error_code err )
		:
			m_value( value ),
			m_err( err )
		{
		}

		context_t( T value, nodeoze::any &&data, std::error_code err )
		:
			m_value( value ),
			m_data( std::move( data ) ),
			m_err( err )
		{
		}

		context_t( const context_t &rhs )
		:
			m_value( rhs.m_value ),
			m_data( rhs.m_data ),
			m_err( rhs.m_err )
		{
		}

		context_t( context_t &&rhs )
		:
			m_value( rhs.m_value ),
			m_data( std::move( rhs.m_data ) ),
			m_err( rhs.m_err )
		{
		}

		inline context_t&
		operator=( const context_t &rhs )
		{
			m_value	= rhs.m_value;
			m_data	= rhs.m_data;
			m_err	= rhs.m_err;
		
			return *this;
		}

		inline context_t&
		operator=( context_t &&rhs )
		{
			m_value	= rhs.m_value;
			m_data	= std::move( rhs.m_data );
			m_err	= rhs.m_err;
		
			return *this;
		}

		inline bool
		operator==( const context_t &rhs ) const
		{
			return ( m_value == rhs.m_value );
		}

		inline bool
		operator!=( const context_t &rhs ) const
		{
			return ( m_value != rhs.m_value );
		}

		inline T
		value() const
		{
			return m_value;
		}

		inline const nodeoze::any&
		data() const
		{
			return m_data;
		}

		inline const std::error_code&
		err() const
		{
			return m_err;
		}

		friend inline std::ostream&
		operator<<( std::ostream &os, const context_t &c )
		{
			os << c.value();
			return os;
		}

	private:

		T				m_value;
		nodeoze::any	m_data;
		std::error_code	m_err;
	};

	typedef context_t< State >													state_t;
	typedef std::function< bool ( const state_t &last, const state_t &next ) >	observe_f;
	typedef std::function< void () >											action_f;

	state_machine( const std::string &name, const state_t &initial )
	:
		m_name( name ),
		m_state( initial )
	{
	}

	state_machine&
	operator=( const state_machine &rhs )
	{
		m_state = rhs.m_state;
		return *this;
	}

	state_machine&
	operator=( state_machine &&rhs )
	{
		m_state = rhs.m_state;
		return *this;
	}

	inline bool
	can_change_state( const state_t &next )
	{
		return m_machine.find( std::make_pair( m_state.value(), next.value() ) ) != m_machine.end();
	}

	inline void
	post( Event e )
	{
		post( e, nodeoze::any::null(), std::error_code() );
	}

	inline void
	post( Event e, nodeoze::any data )
	{
		post( e, std::move( data ), std::error_code() );
	}

	inline void
	post( Event e, std::error_code err )
	{
		post( e, nodeoze::any::null(), err );
	}

	inline void
	on_transition( State state, Event event, State next, action_f action )
	{
		m_machine.emplace( std::piecewise_construct, std::forward_as_tuple( std::make_pair( state, event ) ), std::forward_as_tuple( std::make_pair( next, action ) ) );
	}

	inline void
	observe( const std::vector< State > &states, observe_f observer )
	{
		auto id = ++m_observer_id;

		mlog( marker::state_machine, log::level_t::info, "adding observer to %", m_name );

		m_observers.emplace( std::piecewise_construct, std::forward_as_tuple( id ), std::forward_as_tuple( std::make_pair( states, observer ) ) );
	}

	inline const std::string
	name() const
	{
		return m_name;
	}
	
	inline const state_t&
	state() const
	{
		return m_state;
	}

	inline void
	inflate( const nodeoze::any &root )
	{
		m_state = static_cast< State >( root[ "state" ].to_uint32() );
	}

	inline void
	deflate( nodeoze::any &root ) const
	{
		root[ "state" ] = static_cast< std::uint32_t >( m_state.value() );
	}

private:

	typedef context_t< Event >				event_t;
	typedef std::pair< State, Event >		key_t;
	typedef std::pair< State, action_f >	val_t;

	struct hash_t
	{
		inline std::size_t
		operator()( const key_t &key ) const
		{
			return static_cast< std::size_t >( key.first ) + static_cast<std::size_t>( key.second );
		}
	};

	inline void
	post( Event event, nodeoze::any data, std::error_code err )
	{
		if ( !m_in_transition )
		{
			auto transition = m_machine.find( std::make_pair( m_state.value(), event ) );
			
			if ( transition != m_machine.end() )
			{
				auto last = m_state;

				if ( m_state != transition->second.first )
				{
					mlog( marker::state_machine, log::level_t::info, "% %:% => %", m_name, m_state, event, transition->second.first );
				
					m_in_transition = true;
					m_state			=
					{
						transition->second.first,
						std::move( data ),
						err
					};

#if defined( DEBUG )
					auto saved = m_state;
#endif
					transition->second.second();
#if defined( DEBUG )
					assert( m_state == saved );
#endif
				}
				else
				{
					mlog( marker::state_machine, log::level_t::info, "% %:% ignoring transition but invoking observers", m_name, m_state, event );
				}

				dispatch_observers( last, m_state );

				if ( !m_event_queue.empty() )
				{
					runloop::shared().dispatch( [=]() mutable
					{
						auto event = m_event_queue.front();

						m_event_queue.pop_front();

						post( event.value(), std::move( event.data() ), event.err() );
					} );
				}
			}
			else
			{
				mlog( marker::state_machine, log::level_t::error, "% detected bad state transition %:%", m_name, m_state, event );
#if defined( DEBUG )
				abort();
#endif
			}
		}
		else
		{
#if defined( DEBUG )
			mlog( marker::state_machine, log::level_t::info, "pushing event % on event queue because we're already handling transition to %", event, m_state.value() );
#endif
			m_event_queue.push_back(
			{
				event,
				std::move( data ),
				err
			} );
		}

		m_in_transition = false;
	}

	inline void
	dispatch_observers( const state_t &last, const state_t &next )
	{
		auto observers = m_observers;

		for ( auto &it : observers )
		{
			if ( ( it.second.first.size() == 0 ) || ( std::find( it.second.first.begin(), it.second.first.end(), next.value() ) != it.second.first.end() ) )
			{
				if ( !it.second.second( last, next ) )
				{
					auto eit = m_observers.find( it.first );

					if ( eit != m_observers.end() )
					{
						mlog( marker::state_machine, log::level_t::info, "removing observer from %", m_name );
						m_observers.erase( eit );
					}
				}
			}
		}
	}

	std::unordered_map< key_t, val_t, hash_t >											m_machine;
	nodeoze::deque< event_t >															m_event_queue;
	bool																				m_in_transition		= false;
	std::uint64_t																		m_observer_id		= 0;
	std::unordered_map< std::uint64_t, std::pair< std::vector< State >, observe_f > >	m_observers;
	std::string																			m_name;
	state_t																				m_state;
};

}
