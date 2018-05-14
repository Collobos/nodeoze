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

#include <nodeoze/event.h>
#include <nodeoze/log.h>
#include <nodeoze/test.h>
#include <system_error>

using namespace nodeoze;

TEST_CASE( "nodeoze/smoke/emitter" )
{
	SUBCASE( "void base" )
	{
		event::emitter<> e;
		bool invoked = false;

		auto id = e.on( "test", [&]()
		{
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( "test" );

		REQUIRE( invoked == true );
	}

	SUBCASE( "int base" )
	{
		event::emitter<> e;
		bool invoked = false;

		auto id = e.on( "test", [&]( int i )
		{
			REQUIRE( i == 7 );
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( "test", 7 );

		REQUIRE( invoked == true );
	}

	SUBCASE( "error base" )
	{
		event::emitter<> e;
		bool invoked = false;

		auto id = e.on( "error", [&]( std::error_code err ) mutable
		{
			REQUIRE( static_cast< std::errc >( err.value() ) == std::errc::invalid_argument );
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( "error", make_error_code( std::errc::invalid_argument ) );

		REQUIRE( invoked == true );
	}

	SUBCASE( "std::string" )
	{
		event::emitter<> e;
		bool invoked = false;

		auto id = e.on( std::string( "error" ), [&]( std::error_code err ) mutable
		{
			REQUIRE( static_cast< std::errc >( err.value() ) == std::errc::invalid_argument );
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( std::string( "error" ), make_error_code( std::errc::invalid_argument ) );

		REQUIRE( invoked == true );
	}
}
