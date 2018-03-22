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
 
#ifndef _nodeoze_connection_h
#define _nodeoze_connection_h

#include <nodeoze/nbuffer.h>
#include <nodeoze/nsocket.h>
#include <nodeoze/npromise.h>
#include <nodeoze/nproxy.h>
#include <nodeoze/nuri.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <string>
#include <ios>

namespace nodeoze {

class connection : public std::enable_shared_from_this< connection >
{
public:

	static const std::string																					close_event;
	
	typedef std::shared_ptr< connection >																		ptr;

	typedef std::function< void () >																			idle_timer_handler_f;
	
	typedef nodeoze::ip::tcp::socket::recv_reply_f																peek_reply_f;
	
	class factory
	{
	public:
	
		typedef std::function< bool ( const uri &resource ) >										secure_f;
		typedef std::function< connection::ptr ( const uri &resource ) >							builder_f;
		typedef std::function< connection::ptr ( const uri &resource, ip::tcp::socket socket ) >	builder_with_socket_f;
	
		factory( std::vector< std::string > schemes, secure_f secure, builder_f builder1, builder_with_socket_f builder2 );
		
		static bool
		wants_security( const uri &resoure );
		
		static connection::ptr
		build( const uri &resource );
		
		static connection::ptr
		build( const uri &resource, ip::tcp::socket socket );
		
	private:
	
		typedef std::unordered_map< std::string, std::tuple< secure_f, builder_f, builder_with_socket_f > > factories;
	
		static factories *m_factories;
	};
	
	template < typename T >
	static inline std::shared_ptr< T >
	create( const uri &resource )
	{
		return std::dynamic_pointer_cast< T >( factory::build( resource ) );
	}
	
	template < typename T >
	static inline std::shared_ptr< T >
	create( const uri &resource, ip::tcp::socket sock )
	{
		return std::dynamic_pointer_cast< T >( factory::build( resource, std::move( sock ) ) );
	}
	
	template< typename T, typename F >
	inline static bool
	find( const std::string &id, F functor )
	{
		auto found	= false;
		auto it		= instances().find( id );
		
		if ( it != instances().end() )
		{
			functor( *dynamic_cast< T* >( it->second ) );
			found = true;
		}
		
		return found;
	}
	
	connection( const uri &resource );
	
	connection( ip::tcp::socket sock );

	connection( ptr wrapped );
	
	connection( connection &rhs ) = delete;
	
	connection( connection &&rhs );

	virtual ~connection();
	
	connection&
	operator=( const connection& rhs ) = delete;
	
	connection&
	operator=( connection &&rhs );
	
	inline void
	push( stream::filter *filter )
	{
		if ( m_connected )
		{
			m_socket.push( filter );
		}
		else
		{
			m_filters.emplace_back( filter );
		}
	}
	
	inline stream::filter*
	find( const std::string &name ) const
	{
		return m_socket.find( name );
	}
	
	inline void
	remove( stream::filter *filter )
	{
		m_socket.remove( filter );
	}
	
	inline bool
	is_local() const
	{
		return m_socket.is_peer_local();
	}

	inline bool
	secure() const
	{
		return find( "tls" ) != nullptr ? true : false;
	}
	
	virtual promise< void >
	connect();
	
	virtual promise< void >
	send( buffer buf );

	virtual void
	peek( peek_reply_f reply );
	
	virtual void
	recv();
	
	virtual std::error_code
	process( const buffer &buf ) = 0;

	virtual bool
	is_colocated();
	
	virtual void
	close();
	
	inline bool
	is_open() const
	{
		return m_socket.is_open();
	}
	
	inline ip::endpoint
	peer() const
	{
		return m_socket.peer();
	}
	
	void
	unref();
	
	inline ip::endpoint
	name() const
	{
		return m_socket.name();
	}
	
	ip::address
	subnet() const;

	inline bool
	wrapped() const
	{
		return ( m_wrapped ) ? true : false;
	}
	
	inline const std::string&
	id() const
	{
		return m_id;
	}

	void
	set_idle_timer( std::chrono::milliseconds timeout, idle_timer_handler_f handler );
	
protected:

	typedef std::deque< std::unique_ptr< stream::filter > >	filters_t;

	static std::unordered_map< std::string, connection* >&
	instances()
	{
		static auto global = new std::unordered_map< std::string, connection* >();
		return *global;
	}

	connection();

	void
	maybe_reset_idle_timer();

	virtual nodeoze::uri
	destination() const;
	
	void
	handle_resolve(  const uri &resource, std::deque< ip::address > addrs, promise< void > promise );
	
	bool															m_connected		= false;
	bool															m_connecting	= false;
	ptr																m_wrapped;
	uri																m_resource;
	std::queue< std::pair< nodeoze::buffer, promise< void > > >		m_send_queue;
	filters_t														m_filters;
	ip::tcp::socket													m_socket;
	nodeoze::runloop::event											m_idle_timer	= nullptr;
	idle_timer_handler_f											m_idle_timer_handler;
	std::string														m_id;
};

}

#endif
