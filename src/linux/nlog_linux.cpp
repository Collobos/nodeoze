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

class linux_syslog : public log::sink
{
public:

	linux_syslog()
	{
		openlog( log::shared().name().c_str(), LOG_PID, FACILITY );
	}

	virtual ~linux_syslog()
	{
	}

	virtual void
	put( log::level_t level, std::chrono::system_clock::time_point when, std::uint32_t pid, std::uint32_t tid, const std::string &file, const std::string &func, std::uint32_t line, const std::string &message )
	{
		std::ostringstream os;

		os << pid << ":" << tid << " " << std::to_string( when ) << " " << prune_filename( file ) << ":" << line << " " << prune_function( func ) << " " << message << std::endl;

		switch ( level )
		{
			case log::level_t::info:
			{
				syslog( LOG_INFO, "%s", os.str().c_str() );
			}
			break;

			case log::level_t::warning:
			{
				syslog( LOG_WARNING, "%s", os.str().c_str() );
			}
			break;

			case log::level_t::error:
			{
				syslog( LOG_ERR, "%s", os.str().c_str() );
			}
			break;

			default:
			{
				syslog( LOG_DEBUG, "%s", os.str().c_str() );
			}
			break;
		}
	}
};


std::shared_ptr< log::sink >
log::sink::system()
{
	return std::make_shared< linux_syslog >();
}
