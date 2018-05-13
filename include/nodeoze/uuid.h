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

#ifndef _nodeoze_uuid_h
#define _nodeoze_uuid_h

#include <string>
#include <cstring>
#include <cstdint>
#include <memory>

namespace nodeoze {

class uuid
{
public:

	uuid();
	
	inline uuid( const uuid &rhs )
	{
		std::memcpy( &m_data, &rhs.m_data, sizeof( m_data ) );
	}

	inline ~uuid()
	{
	}
	
	inline uuid&
	operator=( const uuid &rhs )
	{
		std::memcpy( &m_data, &rhs.m_data, sizeof( m_data ) );
		return *this;
	}
	
	inline bool
	operator==( const uuid &rhs )
	{
		return std::memcmp( &m_data, &rhs.m_data, sizeof( m_data ) ) == 0 ? true : false;
	}

	inline bool
	operator!=( const uuid &rhs )
	{
		return std::memcmp( &m_data, &rhs.m_data, sizeof( m_data ) ) != 0 ? true : false;
	}

	std::string
	to_string( const char *delim = "-" ) const;
	
	std::string
	to_base64() const;
	
private:

	std::uint8_t m_data[ 16 ];
};

}

#endif
