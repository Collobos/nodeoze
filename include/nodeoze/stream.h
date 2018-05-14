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

namespace nodeoze {

namespace stream {

class writable;

template< typename T >
struct is_writable_stream : public std::integral_constant< bool, false >
{
};

template<>
struct is_writable_stream< std::shared_ptr< writable > > : public std::integral_constant< bool, true >
{
};

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
	typename std::enable_if< is_writable_stream< T >::value, T >::type
	pipe( T dest )
	{
		on( "data", [dest]( buffer buf ) mutable
		{
			dest->write( buf );
		} );

		return dest;
	}

	template< class T >
	typename std::enable_if< is_writable_stream< T >::value >::type
	unpipe( typename T::ptr /* dest */ )
	{
	}

	void
	push( buffer b );

protected:

	virtual void
	really_read();
};


class writable : public base
{
public:

	using ptr = std::shared_ptr< writable >;

	writable();

	virtual ~writable();

	promise< void >
	write( buffer b );

protected:

	virtual promise< void >
	really_write( buffer b );
};


class duplex : public readable, public writable
{
public:

	duplex();

	virtual ~duplex();
};


class transform : public duplex
{
public:

	transform();

	virtual ~transform();
};

}

}
