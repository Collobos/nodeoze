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

#include <nodeoze/mac.h>
#include <nodeoze/macros.h>
#include <cctype>

using namespace nodeoze;

mac::address::address()
{
	memset( m_bytes, 0, sizeof( m_bytes ) );
}


mac::address::address( const std::uint8_t *bytes, std::size_t size )
{
	memset( m_bytes, 0, sizeof( m_bytes ) );
	ncheck_error( size == 6, exit );
	memcpy( m_bytes, bytes, sizeof( m_bytes ) );

exit:

	return;
}

mac::address::address( const nodeoze::buffer &bytes )
{
	memset( m_bytes, 0, sizeof( m_bytes ) );
	ncheck_error( bytes.size() == 6, exit );
	memcpy( m_bytes, bytes.const_data(), sizeof( m_bytes ) );

exit:

	return;
}

mac::address::address( const std::string &s )
{
	memset( m_bytes, 0, sizeof( m_bytes ) );
	ncheck_error( s.size() == 17, exit );
	sscanf( s.c_str(), "%x-%x-%x-%x-%x-%x", &m_bytes[ 0 ], &m_bytes[ 1 ], &m_bytes[ 2 ], &m_bytes[ 3 ], &m_bytes[ 4 ], &m_bytes[ 5 ] );

exit:

	return;
}


mac::address::~address()
{
}


std::string
mac::address::to_string( const char delimiter ) const
{
	char buf[ 256 ];
	
	sprintf( buf, "%.2x%c%.2x%c%.2x%c%.2x%c%.2x%c%.2x", m_bytes[ 0 ], delimiter, m_bytes[ 1 ], delimiter, m_bytes[ 2 ], delimiter, m_bytes[ 3 ], delimiter, m_bytes[ 4 ], delimiter, m_bytes[ 5 ] );
	
	return buf;
}


any
mac::address::to_any() const
{
	return any( buffer( m_bytes, sizeof( m_bytes ) ) );
}

