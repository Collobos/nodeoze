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

#include <nodeoze/buffer.h>
//#include <nodeoze/macros.h>
#include <nodeoze/dump.h>
#include <iostream>
#include <boost/crc.hpp>

nodeoze::buffer::dealloc_function nodeoze::buffer::default_dealloc = []( elem_type *data )
{
	::free( data );
};

nodeoze::buffer::realloc_function nodeoze::buffer::default_realloc = []( elem_type *data, size_type, size_type new_size )
{
	return ( data == nullptr ) ? reinterpret_cast< elem_type* >( ::malloc( new_size ) ) : reinterpret_cast< elem_type* >( ::realloc( data, new_size ) ); 
};

void
nodeoze::buffer::dump( std::ostream& os ) const
{
	dumpster{}.dump( os, m_data, m_size );
}

nodeoze::buffer::checksum_type
nodeoze::buffer::checksum( size_type offset, size_type length ) const
{
	
	boost::crc_32_type crc;
	if ( ( m_data != nullptr ) && ( offset < length ) && ( ( offset + length ) <= m_size ) )
	{
		crc.process_bytes( m_data + offset, length );
	}
	return crc.checksum();
}



