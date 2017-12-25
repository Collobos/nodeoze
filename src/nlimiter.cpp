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

#include <nodeoze/nlimiter.h>
#include <nodeoze/nlog.h>
#include <chrono>


using namespace nodeoze;


limiter::limiter( double rate, double window )
:
	m_rate( rate ),
	m_window( window ),
	m_allowance( rate )
{
	m_last_time		= clock_t::now();
	m_last_result	= true;
}


limiter::~limiter()
{
}


bool
limiter::proceed()
{
	bool ok = false;
	
	if ( m_rate == 0 )
	{
		ok = true;
	}
	else
	{
		auto		current		= clock_t::now();
		duration_t	duration	= current - m_last_time;
		auto		delta		= duration.count() / 1000000.0;
		
		m_last_time		= current;
		m_allowance		+= ( static_cast< double >( delta ) ) * ( m_rate / m_window );
		
		if ( m_allowance > m_rate )
		{
			m_allowance = m_rate;
		}
		
		if ( m_allowance >= 1.0 )
		{
			m_allowance -= 1.0;
			ok			= true;
		}
		
		if ( m_last_result != ok )
		{
			m_last_result = ok;
			
			if ( ok && m_on_turned_off )
			{
				m_on_turned_off();
			}
			else if ( !ok && m_on_turned_on )
			{
				m_on_turned_on();
			}
		}
	}
	
	return ok;
}
