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
#include <android/log.h>
#include <vector>
#include <mutex>
#include <stdio.h>
#include <string.h>
#include <time.h>

using namespace nodeoze;

void
log::init( const std::string &name )
{
	if ( !m_mutex )
	{
		m_mutex = new std::recursive_mutex;
		init_limiter();
	}

	m_name = name;
}


void
log::put_system_log( log::level_t l, const std::string &message )
{
	switch ( l )
	{
		case log::level_t::info:
		{
			__android_log_print( ANDROID_LOG_INFO, m_name.c_str(), "%s", message.c_str() );
		}
		break;
				
		case log::level_t::warning:
		{
			__android_log_print( ANDROID_LOG_WARN, m_name.c_str(), "%s", message.c_str() );
		}
		break;
				
		case log::level_t::error:
		{
			__android_log_print( ANDROID_LOG_ERROR, m_name.c_str(), "%s", message.c_str() );
		}
		break;
				
		case log::level_t::verbose:
		case log::level_t::voluminous:
		case log::level_t::debug:
		{
			__android_log_print( ANDROID_LOG_VERBOSE, m_name.c_str(), "%s", message.c_str() );
		}
		break;
	}
}


void
log::put_console_log( const std::string &message )
{
}
