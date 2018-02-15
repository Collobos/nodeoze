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
#include <nodeoze/npromise.h>
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

	typedef State	state_type;
	typedef Event	event_type;

	template< class T >
	struct context
	{
	public:

		context( T value )
		:
			m_value( value )
		{
		}

		context( T value, nodeoze::any &&data )
		:
			m_value( value ),
			m_data( std::move( data ) )
		{
		}

		context( T value, std::error_code err )
		:
			m_value( value ),
			m_err( err )
		{
		}

		context( T value, nodeoze::any &&data, std::error_code err )
		:
			m_value( value ),
			m_data( std::move( data ) ),
			m_err( err )
		{
		}

		context( const context &rhs )
		:
			m_value( rhs.m_value ),
			m_data( rhs.m_data ),
			m_err( rhs.m_err )
		{
		}

		context( context &&rhs )
		:
			m_value( rhs.m_value ),
			m_data( std::move( rhs.m_data ) ),
			m_err( rhs.m_err )
		{
		}

		inline context&
		operator=( const context &rhs )
		{
			m_value	= rhs.m_value;
			m_data	= rhs.m_data;
			m_err	= rhs.m_err;
		
			return *this;
		}

		inline context&
		operator=( context &&rhs )
		{
			m_value	= rhs.m_value;
			m_data	= std::move( rhs.m_data );
			m_err	= rhs.m_err;
		
			return *this;
		}

		inline bool
		operator==( const context &rhs ) const
		{
			return ( m_value == rhs.m_value );
		}

		inline bool
		operator!=( const context &rhs ) const
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
		operator<<( std::ostream &os, const context &c )
		{
			os << c.value();
			return os;
		}

	private:

		T				m_value;
		nodeoze::any	m_data;
		std::error_code	m_err;
	};

	typedef context< state_type >																		context_state_type;
	typedef std::function< bool ( const context_state_type &last, const context_state_type &next ) >	observe_f;
	typedef std::function< void () >																	action_f;

	state_machine( const std::string &name )
	:
		m_name( name ),
		m_state( state_type::start )
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
	can_change_state( const context_state_type &next )
	{
		return m_machine.find( std::make_pair( m_state.value(), next.value() ) ) != m_machine.end();
	}

	inline void
	post( event_type e )
	{
		really_post( e, nodeoze::any::null(), std::error_code() );
	}

	inline void
	post( event_type e, nodeoze::any data )
	{
		really_post( e, std::move( data ), std::error_code() );
	}

	inline void
	post( event_type e, std::error_code err )
	{
		really_post( e, nodeoze::any::null(), err );
	}

	inline promise< void >
	post( event_type e, std::vector< state_type > good, std::vector< state_type > bad )
	{
		auto ret = notify_on_transition( std::move( good ), std::move( bad ) );
		really_post( e, nodeoze::any::null(), std::error_code() );
		return ret;
	}

	inline promise< void >
	post( event_type e, nodeoze::any data, std::vector< state_type > good, std::vector< state_type > bad )
	{
		auto ret = notify_on_transition( std::move( good ), std::move( bad ) );
		really_post( e, std::move( data ), std::error_code() );
		return ret;
	}

	inline promise< void >
	post( event_type e, std::error_code err, std::vector< state_type > good, std::vector< state_type > bad )
	{
		auto ret = notify_on_transition( std::move( good ), std::move( bad ) );
		really_post( e, nodeoze::any::null(), err );
		return ret;
	}

	inline void
	on_transition( state_type state, event_type event, state_type next, action_f action )
	{
		m_machine.emplace( std::piecewise_construct, std::forward_as_tuple( std::make_pair( state, event ) ), std::forward_as_tuple( std::make_pair( next, action ) ) );
	}

	inline void
	observe( const std::vector< state_type > &states, observe_f observer )
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
	
	inline const context_state_type&
	state() const
	{
		return m_state;
	}

	inline void
	inflate( const nodeoze::any &root )
	{
		m_state = static_cast< state_type >( root[ "state" ].to_uint32() );
	}

	inline void
	deflate( nodeoze::any &root ) const
	{
		root[ "state" ] = static_cast< std::uint32_t >( m_state.value() );
	}

private:

	typedef context< event_type >				context_event_type;
	typedef std::pair< state_type, event_type >	key_type;
	typedef std::pair< state_type, action_f >	val_type;

	struct hash_type
	{
		inline std::size_t
		operator()( const key_type &key ) const
		{
			return static_cast< std::size_t >( key.first ) + static_cast<std::size_t>( key.second );
		}
	};

	inline void
	really_post( event_type event, nodeoze::any data, std::error_code err )
	{
		if ( !m_in_transition && m_event_queue.empty() )
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
					m_in_transition = false;
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
						if ( !m_event_queue.empty() )
						{
							auto event = m_event_queue.front();

							m_event_queue.pop_front();

							really_post( event.value(), std::move( event.data() ), event.err() );
						}
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
	}

	inline void
	dispatch_observers( const context_state_type &last, const context_state_type &next )
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

	inline promise< void >
	notify_on_transition( std::vector< state_type > good, std::vector< state_type > bad )
	{
		auto ret = promise< void >();

		observe( {}, [=, good{ std::move( good ) }, bad{ std::move( bad ) }]( auto &last, auto &next ) mutable
		{
			nunused( last );

			if ( std::find( good.begin(), good.end(), next.value() ) != good.end() )
			{
				ret.resolve();
			}
			else if ( std::find( bad.begin(), bad.end(), next.value() ) != bad.end() )
			{
				ret.reject( make_error_code( std::errc::state_not_recoverable ), reject_context );
			}

			return !ret.is_finished();
		} );

		return ret;
	}

	std::unordered_map< key_type, val_type, hash_type >									m_machine;
	nodeoze::deque< context_event_type >												m_event_queue;
	bool																				m_in_transition		= false;
	std::uint64_t																		m_observer_id		= 0;
	std::unordered_map< std::uint64_t, std::pair< std::vector< state_type >, observe_f > >	m_observers;
	std::string																			m_name;
	context_state_type																	m_state;
};

}
