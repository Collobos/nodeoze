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
 
#include <nodeoze/nlog.h>
#include <nodeoze/nmacros.h>
#include <dispatch/dispatch.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <mutex>
#include <syslog.h>
#include <asl.h>

using namespace nodeoze;

static aslmsg		msg;
static aslclient	c;


void
log::init( const std::string &name )
{
	if ( !m_mutex )
	{
		m_mutex = new std::recursive_mutex;
		init_limiter();
	}
	
	c = asl_open( name.c_str(), "com.apple.console", 0);

	msg = asl_new(ASL_TYPE_MSG);
	asl_set(msg, ASL_KEY_FACILITY, "com.apple.console");
	asl_set(msg, ASL_KEY_LEVEL, ASL_STRING_NOTICE);
	asl_set(msg, ASL_KEY_READ_UID, "-1");
}


void
log::cleanup( const std::string &name )
{
	nunused( name );
}


void
log::put_system_log( level_t l, const std::string &message )
{
	nunused( l );
	
	asl_log( c, msg, ASL_LEVEL_NOTICE, "%s", message.c_str() );
}


void
log::put_console_log( const std::string &message )
{
	fprintf( stderr, "%s\n", message.c_str() );
}


