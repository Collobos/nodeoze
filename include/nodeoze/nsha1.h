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

#ifndef _nodeoze_sha1_h
#define _nodeoze_sha1_h

#include <cstdint>
#include <string>

namespace nodeoze {

namespace crypto {

namespace hash {

class sha1
{
public:

    sha1();
	
	void
	update( const std::string &s );
	
    void
	update( std::istream &is );
	
    std::string
	final();
	
private:

    static const unsigned int DIGEST_INTS	= 5;				/* number of 32bit integers per SHA1 digest */
    static const unsigned int BLOCK_INTS	= 16;				/* number of 32bit integers per SHA1 block */
    static const unsigned int BLOCK_BYTES	= BLOCK_INTS * 4;

	std::uint32_t	m_digest[DIGEST_INTS];
	std::string		m_buffer;
	std::uint64_t	m_transforms;

	void
	reset();
	
	void
	transform( std::uint32_t block[BLOCK_BYTES]);

    static void
	buffer_to_block(const std::string &buffer, std::uint32_t block[BLOCK_INTS]);
	
    static void
	read( std::istream &is, std::string &s, const std::size_t max);
};

}

}

}

#endif
