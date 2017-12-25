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
/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <nodeoze/nasync_repeater.h>
#include <nodeoze/nmarkers.h>
#if defined( WIN32 )
#	undef min
#endif

using namespace nodeoze;

void
timer::start()
{
	m_start = std::chrono::system_clock::now();
}

void
timer::check()
{
	m_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_start);
}

std::chrono::milliseconds
timer::elapsed() const
{
	return m_elapsed;
}
	
void
time_limiter::start()
{
	timer::start();
	m_expiration = m_start + m_time_limit;
}

void
time_limiter::check()
{
	timer::check();
	if (m_time_limit <= elapsed())
	{
		m_remaining = std::chrono::milliseconds(0);
	}
	else
	{
		m_remaining = m_time_limit - elapsed();
	}
}

std::chrono::milliseconds
time_limiter::remaining() const
{
	return m_remaining;
}

bool
time_limiter::has_next() const
{
	return (!base_limiter::is_aborted()) && (std::chrono::system_clock::now() < m_expiration);
}




std::chrono::milliseconds
exponential_backoff_scheduler::next_timeout() 
{
	auto rcount = count();
	
	if (rcount == 0)
	{
		return m_base_timeout;
	}
	else
	{
		auto shift = rcount - 1;
		shift = std::min(static_cast<std::uint32_t>(m_exp_bound - 1), shift);
		auto half = (1UL << static_cast< std::uint64_t >( shift) ) * m_base_timeout.count();
		return std::chrono::milliseconds(half + (m_rng() % half));
	}
}


std::chrono::milliseconds
time_limited_exponential_backoff_scheduler::next_timeout()
{
	check();
	auto next_interval = exponential_backoff_scheduler::next_timeout();
	if (remaining() < next_interval)
	{
		return remaining();
	}
	else
	{
		return next_interval;
	}
}


std::chrono::milliseconds
fixed_interval_scheduler::next_timeout()
{
	return m_interval;
}


std::chrono::milliseconds
time_limited_fixed_interval_scheduler::next_timeout()
{
	check();
	auto next_interval = fixed_interval_scheduler::next_timeout();
	if (remaining() < next_interval)
	{
		return remaining();
	}
	else
	{
		return next_interval;
	}
}


const std::string async_repeater::inactive_message = "inactive";
const std::string async_repeater::in_progress_message = "in_progress";
const std::string async_repeater::timed_out_message = "timed_out";
const std::string async_repeater::succeeded_message = "succeeded";
const std::string async_repeater::failed_message = "failed";
const std::string async_repeater::unknown_message = "unknown";

const char*
async_repeater::status_message(status_t status)
{
	switch (status)
	{
		case status_t::inactive:
		{
			return inactive_message.c_str();
		}
		break;
		case status_t::in_progress:
		{
			return in_progress_message.c_str();
		}
		break;
		case status_t::timed_out:
		{
			return timed_out_message.c_str();
		}
		break;
		case status_t::succeeded:
		{
			return succeeded_message.c_str();
		}
		break;
		case status_t::failed:
		{
			return failed_message.c_str();
		}
		break;
		default:
		{
			return unknown_message.c_str();
		}
	
	};
}


void
async_retry_with_timeout::attempt_failed()
{
	set_failed();
	
	cancel_timer();
	
	get_counter()->advance();
	
	if (get_limiter()->has_next())
	{
		attempt();
	}
	else
	{
		m_failure(this->shared_from_this());
	}
}

void
async_retry_with_timeout::attempt_succeeded()
{
	set_succeeded();
	cancel_timer();
}

void
async_retry_with_timeout::cancel_timer()
{
	if (m_timeout_timer != nullptr)
	{
		nodeoze::runloop::shared().cancel( m_timeout_timer );
		m_timeout_timer = nullptr;
	}
	else
	{
		nlog(nodeoze::log::level_t::error, "expected non-null timer");
	}
}

void
async_retry_with_timeout::attempt()
{
	if (is_in_progress())
	{
		nlog(nodeoze::log::level_t::warning, "entering attempt, status was %", status_message());
	}
	set_in_progress();
	schedule_next_timeout( [&]( nodeoze::runloop::event e ) mutable
	{
		nunused( e );
		
		if (is_aborted())
		{
			nlog(nodeoze::log::level_t::warning, "async retry attempt % timeout, is_aborted() true", get_counter()->count());
			return;
		}
		if (is_in_progress())
		{
			mlog( marker::async_iter, log::level_t::info, "async retry attempt % timeout, status in_progress, canceling timer", get_counter()->count());
			cancel_timer();
			set_timed_out();
			m_timed_out(this->shared_from_this());
			get_counter()->advance();
			
			if ( get_limiter()->has_next() )
			{
				mlog( marker::async_iter, log::level_t::info, "async retry timeout, starting next attempt %", get_counter()->count());
				attempt();
			}
			else
			{
				mlog( marker::async_iter, log::level_t::info, "async retry timeout, no more attempts, failure");
				m_failure(this->shared_from_this());
			}
		}
		else
		{
			nlog(nodeoze::log::level_t::error, "attempt timer expired with status %", status_message());
		}
	} );
	start_duration_timer();
	m_attempt(this->shared_from_this());
}

void
async_retry_with_timeout::schedule_next_timeout(nodeoze::runloop::event_f handler)
{
	if (m_timeout_timer != nullptr)
	{
		nlog(nodeoze::log::level_t::error, "expected null timer");
		cancel_timer();
		m_timeout_timer = nullptr;
	}
	
	auto timeout = get_scheduler()->next_timeout();
	m_timeout_timer = nodeoze::runloop::shared().create(timeout);
	nodeoze::runloop::shared().schedule(m_timeout_timer, handler);
}

