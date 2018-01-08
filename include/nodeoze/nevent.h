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
 
#ifndef _nodeoze_event_h
#define _nodeoze_event_h

#include <functional>
#include <map>
#include <memory>
#include <list>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>


namespace nodeoze {

namespace event {

namespace detail {

struct listener_base
{
	typedef std::uint64_t id_type;
	listener_base()
	{
	}

	listener_base( id_type id )
	:
		m_id( id )
	{
	}

	virtual ~listener_base()
	{
	}

	id_type m_id;
};

}

template< class Key = std::string, class Table = std::unordered_map< Key, std::vector< std::shared_ptr< detail::listener_base > > > >
class emitter
{
public:

	typedef Key								id_type;
	typedef detail::listener_base::id_type	listener_id_type;
    
	emitter()
	{
	}

	emitter( const emitter& ) = delete;  

	emitter( emitter&& ) = delete;

	~emitter()
	{
	}

	emitter&
	operator=( const emitter& ) = delete;

	emitter&
	operator=( emitter&& ) = delete;

	inline listener_id_type
	add_listener( const id_type &id, std::function< void ()> handler )
	{
		auto listener_id = listener_id_type( 0 );

		if ( handler )
		{
			auto it	= m_listeners.find( id );

			listener_id = ++m_last_listener;

			if ( it == m_listeners.end() )
			{
				m_listeners[ id ] = std::vector< std::shared_ptr< detail::listener_base > >();
				it = m_listeners.find( id );
			}

			it->second.emplace_back( std::make_shared< listener<> >( listener_id, handler ) );
		}

		return listener_id;        
	}

    template <typename... Args>
	inline listener_id_type
	add_listener( const id_type &id, std::function< void ( Args... )> handler )
	{
		auto listener_id = listener_id_type( 0 );

		if ( handler )
		{
			auto it	= m_listeners.find( id );

			listener_id = ++m_last_listener;

			if ( it == m_listeners.end() )
			{
				m_listeners[ id ] = std::vector< std::shared_ptr< detail::listener_base > >();
				it = m_listeners.find( id );
			}

			it->second.emplace_back( std::make_shared< listener< Args... > >( listener_id, handler ) );
		}

		return listener_id;        
	}

    template<typename Listener>
    inline listener_id_type
	add_listener( const id_type &id, Listener listener )
	{
		return add_listener( id, make_function( listener ) );
    }

	inline listener_id_type
	on( const id_type &id, std::function< void () > listener )
	{
		return add_listener( id, listener );
	}

    template <typename... Args>
	inline listener_id_type
	on( const id_type &id, std::function< void ( Args... )> listener )
	{
		return add_listener( id, listener );
	}
    
    template<typename Listener>
	listener_id_type
	on( const id_type &id, Listener listener )
	{
		return on( id, make_function( listener ) );
    }

	void
	remove_listener( listener_id_type listener_id );

    template< typename... Args >
	inline void
	emit( const id_type &id, Args... args )
	{
		auto it = m_listeners.find( id );

		if ( it != m_listeners.end() )
		{
			std::vector< std::shared_ptr< listener< Args... > > > listeners;

			listeners.reserve( it->second.size() );

			for ( auto base : it->second )
			{
				auto l = std::dynamic_pointer_cast< listener< Args...> >( base );

				if ( l )
				{
					listeners.emplace_back( std::move( l ) );
				}
			}
					
			for ( auto &listener : listeners )
			{
				listener->m_handler( args... );
			}
		}
	}

private:

	template <typename... Args>
	struct listener : public detail::listener_base
    {
		listener()
		{
		}

		listener( id_type id, std::function< void ( Args...)> handler )
		:
			listener_base( id ),
			m_handler( std::move( handler ) )
		{
		}

        std::function< void ( Args... ) > m_handler;
    };

    template <typename T>
    struct function_traits
	:
		public function_traits< decltype( &T::operator() ) >
	{
	};

    template <typename ClassType, typename ReturnType, typename... Args>
	struct function_traits<ReturnType(ClassType::*)(Args...) const>
	{
		typedef std::function<ReturnType (Args...)> f_type;
	};

    template <typename L> 
    typename function_traits<L>::f_type make_function(L l)
	{
		return (typename function_traits<L>::f_type)(l);
	}

	Table				m_listeners;
    listener_id_type	m_last_listener = 0;
};

}

}

#endif
