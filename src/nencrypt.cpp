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

#include <nodeoze/nencrypt.h>
#include <nodeoze/nbase64.h>
#include <nodeoze/nbuffer.h>
#include <nodeoze/ntest.h>

using namespace nodeoze;

static buffer
transform( const std::string &input, const std::string &key )
{
	buffer ret( input.size() );
	
    short unsigned int klen =	key.length();
	short unsigned int ilen =	input.length();
    short unsigned int k	=	0;

	for( auto i = 0; i < ilen; i++ )
    {
		ret.push_back( input[ i ] ^ key[ k ] );
		k = ( ++k < klen ? k : 0 );
    }

    return ret;
}


std::string
nodeoze::crypto::encrypt( const std::string &input, const std::string &key )
{
	return codec::base64::encode( transform( input, key ) );
}


std::string
nodeoze::crypto::decrypt( const std::string &input, const std::string &key )
{
	return transform( codec::base64::decode( input ).to_string(), key ).to_string();
}


TEST_CASE( "nodeoze: xor encryption" )
{
	auto orig		= "this is a test of encryption";
	auto encrypted	= crypto::encrypt( orig, "this is a test key" );
	
	CHECK( orig != encrypted );
	
	auto good		= crypto::decrypt( encrypted, "this is a test key" );
	
	CHECK( orig == good );
	
	auto bad		= crypto::decrypt( encrypted, "this is not a key" );
	
	CHECK( orig != bad );
}

