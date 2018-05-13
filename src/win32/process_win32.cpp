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

#include <nodeoze/nprocess.h>
#include <nodeoze/nunicode.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nusername.h>
#include <nodeoze/win32/nuser.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/njson.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nthread.h>
#include <nodeoze/nlog.h>
#include <WinSock2.h>
#include <Windows.h>
#include <memory>

#pragma comment( lib, "wtsapi32.lib" )
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "crypt32.lib" )
#pragma comment( lib, "psapi.lib" )

using namespace nodeoze;

static std::string
get_username()
{
	TCHAR username[ 1024 ];
	DWORD size;

	ZeroMemory( username, sizeof( username ) );

	size = sizeof( username ) / sizeof( TCHAR );

	GetUserName( username, &size );

	return narrow( username );
}



process&
process::self()
{
	static process p( get_username(), GetCurrentProcessId() );
	
	return p;
}
