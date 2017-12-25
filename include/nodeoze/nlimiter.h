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
 
#ifndef _nodeoze_limiter_h
#define _nodeoze_limiter_h

#include <functional>
#include <cstdint>
#include <chrono>

namespace nodeoze
{

class limiter
{
public:

	typedef std::function< void () > handler_f;

	limiter( double rate = 1, double window = 1 );
	
	~limiter();
	
	inline void
	on_turned_on( handler_f handler )
	{
		m_on_turned_on = handler;
	}
	
	inline void
	on_turned_off( handler_f handler )
	{
		m_on_turned_off = handler;
	}
	
	inline double
	rate() const
	{
		return m_rate;
	}
	
	inline void
	set_rate( double val )
	{
		m_rate = val;
	}
	
	inline double
	window() const
	{
		return m_window;
	}
	
	bool
	proceed();
	
private:

	typedef std::chrono::high_resolution_clock			clock_t;
	typedef std::chrono::duration< double, std::micro >	duration_t;
	
	handler_f											m_on_turned_on;
	handler_f											m_on_turned_off;
	clock_t::time_point									m_last_time;
	bool												m_last_result;
	double												m_rate;
	double												m_window;
	double												m_allowance;
};

}

#endif
