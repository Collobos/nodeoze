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

#include <nodeoze/nevent.h>
#include <nodeoze/nlog.h>
#include <nodeoze/ntest.h>
#include <system_error>

using namespace nodeoze;

class derived_emitter : public nodeoze::event::emitter
{
public:
	DECLARE_EVENTS( "test", "error");
};

DEFINE_EVENTS(derived_emitter);

TEST_CASE( "nodeoze: emitter" )
{
	static const std::size_t TEST = 0;
	static const std::size_t ERROR = 1;
	
	SUBCASE( "void base" )
	{
		event::emitter e;
		bool invoked = false;

		auto id = e.on( TEST, [&]()
		{
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( TEST );

		REQUIRE( invoked == true );
	}

	SUBCASE( "int base" )
	{
		event::emitter e;
		bool invoked = false;

		auto id = e.on( TEST, [&]( int i )
		{
			REQUIRE( i == 7 );
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( TEST, 7 );

		REQUIRE( invoked == true );
	}

	SUBCASE( "error base" )
	{
		event::emitter e;
		bool invoked = false;

		auto id = e.on( ERROR, [&]( std::error_code err )
		{
			REQUIRE( static_cast< std::errc >( err.value() ) == std::errc::invalid_argument );
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( ERROR, std::make_error_code( std::errc::invalid_argument ) );

		REQUIRE( invoked == true );
	}

	SUBCASE( "void derived" )
	{
		derived_emitter e;
		bool invoked = false;

		auto id = e.on( "test", [&]()
		{
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( "test" );

		REQUIRE( invoked == true );
	}

	SUBCASE( "int derived" )
	{
		derived_emitter e;
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

	SUBCASE( "error derived" )
	{
		derived_emitter e;
		bool invoked = false;

		auto id = e.on( "error", [&]( std::error_code err )
		{
			REQUIRE( static_cast< std::errc >( err.value() ) == std::errc::invalid_argument );
			invoked = true;
		} );

		REQUIRE( id != 0 );

		e.emit( "error", std::make_error_code( std::errc::invalid_argument ) );

		REQUIRE( invoked == true );
	}

}
