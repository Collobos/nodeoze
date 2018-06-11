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
 
#ifndef _nodeoze_expected_h
#define _nodeoze_expected_h

#include <nodeoze/log.h>
#include <stdexcept>
#include <typeinfo>
#include <assert.h>

namespace nodeoze {

template <typename T>
class expected
{
public:

	expected()
	:
		m_expected( false )
	{
	}

    expected( expected const &rhs )
	:
		m_expected( rhs.m_expected ),
		m_value( rhs.m_value )
	{
    }
	
	expected( expected &&rhs )
	:
		m_expected( rhs.m_expected ),
		m_value( std::move( rhs.m_value ) )
	{
    }

	inline expected&
	operator=( expected const &rhs )
	{
		m_expected	= rhs.m_expected;
		m_value		= rhs.m_value;

        return *this;
    }

	inline expected&
	operator=( expected &&rhs )
	{
		m_expected	= rhs.m_expected;
		m_value		= std::move( rhs.m_value );
		
		return *this;
	}

	expected( T const& value )
	:
		m_expected( true ),
		m_value( value )
	{
    }
	
    expected( T &&value )
	:
		m_expected( true ),
		m_value( std::move( value ) )
	{
    }

    expected( const std::exception &exception )
	:
		m_expected( false )
	{
		nunused( exception );
    }

	inline ~expected()
	{
	}
	
	inline bool
	is_valid() const
	{
		return m_expected;
    }

	inline T&
	get()
	{
		if ( !is_valid() )
		{
			nlog( log::level_t::error, "unexpected value" );
		}
		
		return as_value();
    }
	
	inline T const&
	get() const
	{
		if ( !is_valid() )
		{
			nlog( log::level_t::error, "unexpected value" );
		}
		
		return as_value();
	}

private:

	inline T&
	as_value()
	{
		return m_value;
    }
	
	inline T const&
	as_value() const
	{
		return m_value;
    }

    bool	m_expected;
	T		m_value;
};

}

#endif
