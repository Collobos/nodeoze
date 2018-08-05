/*
 * Copyright (c) 2013-2018, Collobos Software Inc.
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

#include <nodeoze/promise.h>
#include <nodeoze/buffer.h>
#include <nodeoze/event.h>
#include <nodeoze/deque.h>
#include <unordered_map>
#include <queue>

namespace nodeoze {

namespace stream {

class base : virtual public event::emitter<>
{
public:

	base();

	virtual ~base();

	std::uint32_t
	highwater_mark()
	{
		return m_highwater_mark;
	}

	void
	set_highwater_mark( std::uint32_t val )
	{
		m_highwater_mark = val;
	}

protected:

	std::uint32_t	m_highwater_mark;
};

class writable : public base
{
public:

	using ptr = std::shared_ptr< writable >;

	writable();

	virtual ~writable();

	bool
	write( buffer b, std::function< void () > cb = nullptr );

	void
	end();

protected:

	void
	start_write( buffer b );

	virtual promise< void >
	really_write( buffer b );

	bool					m_writing	= false;
	std::queue< buffer >	m_queue;
	bool					m_ended		= false;
};

/*
 * class readable
 * 
 * events:
 * 
 * "close"
 * "data"
 * "end"
 * "error"
 * "readable"
 *
 */

class readable : public base
{
public:

	using ptr = std::shared_ptr< readable >;

	readable();

	virtual ~readable();

	template< class T >
	typename std::enable_if< std::is_base_of< writable, T >::value, typename T::ptr >::type
	pipe( typename std::shared_ptr< T > dest )
	{
		auto it = m_pipes.find( dest );

		if ( it == m_pipes.end() )
		{
			m_pipes[ dest ] = listeners();
			it = m_pipes.find( dest );
		}

		it->second.emplace_back( std::make_pair( "data", on( "data", [this,dest]( buffer buf ) mutable
		{
			if ( !dest->write( buf ) )
			{
				really_pause();

				dest->once( "drain", [=]() mutable
				{
					really_read();
				} );
			}
		} ) ) );

		it->second.emplace_back( std::make_pair( "error", on( "error", [dest]( std::error_code err ) mutable
		{
			dest->emit( "error", err );
		} ) ) );

		it->second.emplace_back( std::make_pair( "end", on( "end", [dest]() mutable
		{
			dest->end();
		} ) ) );

		return dest;
	}

	template< class T >
	typename std::enable_if< std::is_base_of< writable, T >::value, void >::type
	unpipe( typename T::ptr dest )
	{
		auto it = m_pipes.find( dest );
		assert( it != m_pipes.end() );

		if ( it != m_pipes.end() )
		{
			for ( auto &id : it->second )
			{
				remove_listener( it->second->first, it->second->second );
			}

			m_pipes.erase( it );
		}
	}

	void
	push( buffer b );

protected:

	virtual void
	really_read();

	virtual void
	really_pause();

	using listeners = deque< std::pair< std::string, listener_id_type > >;

	std::unordered_map< writable::ptr, listeners > m_pipes;
};


class duplex : public readable, public writable
{
public:

	using ptr = std::shared_ptr< duplex >;

	duplex();

	virtual ~duplex();
};


class transform : public duplex
{
public:

	transform();

	virtual ~transform();
};


class dual
{
public:

	using ptr = std::shared_ptr< dual >;

	dual();

	virtual ~dual();

	virtual duplex::ptr
	top() = 0;

	virtual duplex::ptr
	bottom() = 0;
};

}

}
