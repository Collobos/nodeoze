/*
 * Copyright (c) 2013 - 2017, Collobos Software Inc.
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
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 *
 */

#include <nodeoze/nlog.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <mutex>

using namespace nodeoze;

static const int FACILITY = LOG_DAEMON;

void
log::init( const std::string &name )
{
    if ( !m_mutex )
	{
		m_mutex = new std::recursive_mutex;
		init_limiter();
	}

	openlog( name.c_str(), LOG_PID, FACILITY );
}


void
log::cleanup( const std::string &name )
{
}


void
log::put_system_log( level_t l, const std::string &message )
{
    switch ( l )
    {
        case level_t::info:
        {
            syslog( LOG_INFO, "%s", message.c_str() );
        }
        break;

        case level_t::warning:
        {
            syslog( LOG_WARNING, "%s", message.c_str() );
        }
        break;

        case level_t::error:
        {
            syslog( LOG_ERR, "%s", message.c_str() );
        }
        break;

        default:
        {
            syslog( LOG_DEBUG, "%s", message.c_str() );
        }
        break;
    }
}


void
log::put_console_log( const std::string &message )
{
	std::lock_guard< std::recursive_mutex > lock( *m_mutex );
	fprintf( stderr, "%s\n", message.c_str() );
}
