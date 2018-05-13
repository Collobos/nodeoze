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

#ifndef _nodeoze_async_repeater_h
#define _nodeoze_async_repeater_h

#include <chrono>
#include <nodeoze/runloop.h>
#include <nodeoze/notification.h>
#include <nodeoze/markers.h>
#include <nodeoze/log.h>
#include <random>

#if defined( WIN32 )
#	pragma warning( push )
#	pragma warning( disable : 4250 )
#endif

#if defined( check )
#undef check
#endif

using namespace nodeoze;

namespace nodeoze
{

class base_limiter
{
public:

	base_limiter()
	:
		m_aborted(false)
	{
	}

	virtual ~base_limiter()
	{
	}

	virtual bool
	has_next() const = 0;
	
	virtual void
	abort()
	{
		m_aborted = true;
	}
	
	virtual bool
	is_aborted() const
	{
		return m_aborted;
	}
	
protected:
	bool m_aborted;

};

class counter
{
public:

	virtual ~counter()
	{
	}

	virtual void
	reset()
	{
		m_count = 0;
	}
	
	virtual std::uint32_t
	count() const
	{
		return m_count;
	}
	
	virtual void
	advance()
	{
		m_count++;
	}
	
protected:

	std::uint32_t m_count = 0;
};


class count_limiter : public virtual counter, virtual public base_limiter
{
public:

	count_limiter(std::uint32_t _limit)
	:
		counter(),
		m_limit(_limit)
	{
	}

	virtual ~count_limiter()
	{
	}
	
	virtual bool
	has_next() const
	{
		return (!is_aborted()) && (count() < m_limit);
	}
	
protected:
	std::uint32_t m_limit;
};



template<typename T>
class stepper : public virtual counter, virtual public base_limiter
{
public:

	virtual ~stepper()
	{
	}

	virtual T
	current() = 0;

	virtual void
	abort()
	{
		base_limiter::abort();
	}
	
	virtual bool
	is_aborted() const
	{
		return base_limiter::is_aborted();
	}


};

class simple_stepper : public stepper<std::uint32_t>
{
public:

	virtual ~simple_stepper()
	{
	}

	virtual std::uint32_t
	current()
	{
		return count();
	}
};

template<typename C>
class container_stepper : public stepper<typename C::const_iterator>
{
public:

	container_stepper(C&& _container)
	:
		m_container(std::forward<C>(_container)),
		m_current(m_container.begin()),
		m_last(m_container.end())
	{
	}

	virtual ~container_stepper()
	{
	}
	
	virtual typename C::const_iterator
	current()
	{
		return m_current;
	}
	
	virtual void
	advance()
	{
		counter::advance();
		
		if (has_next())
		{
			m_current++;
		}
	}

	virtual void
	reset()
	{
		counter::reset();
		m_current = m_container.begin();
	}
	
	virtual bool
	has_next() const
	{
		return (! base_limiter::is_aborted()) && (m_current != m_last);
	}
	

protected:

	const C m_container;
	typename C::const_iterator m_current;
	typename C::const_iterator m_last;
};


template <typename Iter>
class iterator_stepper : public stepper<Iter>
{
public:
	iterator_stepper(Iter _current, Iter _last)
	:
	m_current(_current),
	m_last(_last)
	{ }

	virtual bool
	has_next() const
	{
		return (!base_limiter::is_aborted()) && (m_current != m_last);
	}

	virtual Iter
	current()
	{
		return m_current;
	}
	
	virtual void
	advance()
	{
		counter::advance();
		
		if (has_next())
		{
			m_current++;
		}
	}
	
protected:

	Iter m_current;
	Iter m_last;
	
private:

	virtual void
	reset()
	{
		counter::reset();
	}
};


class timer
{
public:

	virtual ~timer()
	{
	}

	virtual void
	start();
	
	virtual void
	check();
	
	virtual std::chrono::milliseconds
	elapsed() const;
	
protected:

	std::chrono::milliseconds m_elapsed;
	std::chrono::time_point<std::chrono::system_clock> m_start;
};

class time_limiter : public virtual timer, public virtual base_limiter
{
public:

	time_limiter(std::chrono::milliseconds _limit)
	:
		timer(),
		m_time_limit(_limit)
	{
	}

	virtual ~time_limiter()
	{
	}

	virtual void
	start();
	
	virtual void
	check();
	
	virtual std::chrono::milliseconds
	remaining() const;
	
	virtual bool
	has_next() const;
	
protected:
	const std::chrono::milliseconds m_time_limit;
	std::chrono::milliseconds m_remaining;
	std::chrono::time_point<std::chrono::system_clock> m_expiration;

};


class condition_limiter : public virtual counter, virtual public base_limiter
{
public:

	typedef std::function< bool (std::uint32_t _count) > condition_f;
	
	condition_limiter(condition_f _condition)
	:
		m_condition(_condition)
	{
	}

	virtual ~condition_limiter()
	{
	}
	
	virtual bool
	has_next() const
	{
		return (!base_limiter::is_aborted()) && m_condition(count());
	}
	
protected:
	condition_f m_condition;
};


class scheduler : public virtual counter, virtual public base_limiter
{
public:

	virtual ~scheduler()
	{
	}

	virtual std::chrono::milliseconds
	next_timeout() = 0;
};



class exponential_backoff_scheduler : public scheduler
{
public:

	exponential_backoff_scheduler(std::chrono::milliseconds _base, std::uint32_t _bound)
	:
		m_base_timeout(_base),
		m_exp_bound(_bound)
	{
	}

	virtual ~exponential_backoff_scheduler()
	{
	}

	virtual std::chrono::milliseconds
	next_timeout();

protected:

	std::chrono::milliseconds m_base_timeout;
	std::uint32_t m_exp_bound;
	std::minstd_rand m_rng;

};


class time_limited_exponential_backoff_scheduler : public exponential_backoff_scheduler, time_limiter
{
public:

	time_limited_exponential_backoff_scheduler(std::chrono::milliseconds _base, std::uint32_t _bound, std::chrono::milliseconds _limit)
	:
		exponential_backoff_scheduler(_base, _bound),
		time_limiter(_limit)
	{
	}

	virtual ~time_limited_exponential_backoff_scheduler()
	{
	}
	
	virtual std::chrono::milliseconds
	next_timeout();
};

class count_limited_exponential_backoff_scheduler : public exponential_backoff_scheduler, public count_limiter
{
public:

	count_limited_exponential_backoff_scheduler(std::chrono::milliseconds _base, std::uint32_t _bound, std::uint32_t _limit)
	:
		exponential_backoff_scheduler(_base, _bound),
		count_limiter(_limit)
	{
	}

	virtual ~count_limited_exponential_backoff_scheduler()
	{
	}
};



class fixed_interval_scheduler : public scheduler
{
public:

	fixed_interval_scheduler(std::chrono::milliseconds _interval)
	:
		m_interval(_interval)
	{
	}

	virtual ~fixed_interval_scheduler()
	{
	}

	virtual std::chrono::milliseconds
	next_timeout();

protected:

	std::chrono::milliseconds m_interval;
};


class time_limited_fixed_interval_scheduler : public fixed_interval_scheduler, time_limiter
{
public:

	time_limited_fixed_interval_scheduler(std::chrono::milliseconds _interval, std::chrono::milliseconds _limit)
	:
		fixed_interval_scheduler(_interval),
		time_limiter(_limit)
	{
	}

	virtual ~time_limited_fixed_interval_scheduler()
	{
	}
	
	virtual std::chrono::milliseconds
	next_timeout();
	
};

class count_limited_fixed_interval_scheduler : public fixed_interval_scheduler, count_limiter
{
public:

	count_limited_fixed_interval_scheduler(std::chrono::milliseconds _interval, std::uint32_t _limit)
	:
		fixed_interval_scheduler(_interval),
		count_limiter(_limit)
	{
	}

	virtual ~count_limited_fixed_interval_scheduler()
	{
	}
};

//class condition_limited_fixed_interval_scheduler : public fixed_interval_scheduler, condition_limiter
//{
//public:
//	condition_limited_fixed_interval_scheduler(std::chrono::milliseconds _interval, condition_limiter::condition_f _condition)
//	:
//	fixed_interval_scheduler(_interval),
//	condition_limiter(_condition)
//	{}
//};


class async_repeater
{
public:

	enum class status_t
	{
		inactive,
		in_progress,
		timed_out,
		succeeded,
		failed
	};
	
	static const std::string inactive_message;
	static const std::string in_progress_message;
	static const std::string timed_out_message;
	static const std::string succeeded_message;
	static const std::string failed_message;
	static const std::string unknown_message;

	typedef std::shared_ptr<async_repeater> repeater_ptr;

	typedef std::shared_ptr<counter> counter_ptr;
	typedef std::shared_ptr<base_limiter> base_limiter_ptr;
	
	async_repeater()
	:
		m_status( status_t::inactive)
	{
		mlog( marker::async_iter, log::level_t::info, "async_repeater constructor called");
	}

	virtual void
	abort()
	{
		mlog( marker::async_iter, log::level_t::info, "async_repeater::abort called");
		get_limiter()->abort();
	}
	
	virtual
	~async_repeater()
	{
		mlog( marker::async_iter, log::level_t::info, "async_repeater destructor called");
	}
	
	virtual bool
	is_aborted()
	{
		mlog( marker::async_iter, log::level_t::info, "async_repeater::is_aborted called");
		return get_limiter()->is_aborted();
	}
	
	virtual counter_ptr
	get_counter() const = 0;
	
	virtual base_limiter_ptr
	get_limiter() const = 0;
	
	virtual void
	attempt() = 0;
	
	virtual void
	attempt_failed() = 0;
	
	virtual void
	attempt_succeeded() = 0;
	
	virtual bool
	is_stale(std::uint32_t icount) const
	{
		return (get_counter()->count() != icount) || (!is_in_progress());
	}
	
	virtual void
	start()
	{
		mlog( marker::async_iter, log::level_t::info, "async_repeater::start called");
		
		if (get_limiter()->has_next())
		{
			attempt();
		}
	}


	static const char*
	status_message(status_t err);
	
	inline const char*
	status_message() const
	{
		return status_message(m_status);
	}
	
	inline bool
	is_failed() const
	{
		return m_status == status_t::failed;
	}
	
	inline bool
	is_succeeded() const
	{
		return m_status == status_t::succeeded;
	}
	
	inline bool
	is_timed_out() const
	{
		return m_status == status_t::timed_out;
	}
	
	inline bool
	is_in_progress() const
	{
		return m_status == status_t::in_progress;
	}
	
	inline void
	set_failed()
	{
		m_status = status_t::failed;
	}
	
	inline void
	set_succeeded()
	{
		m_status = status_t::succeeded;
	}
	
	inline void
	set_timed_out()
	{
		m_status = status_t::timed_out;
	}
	
	inline void
	set_in_progress()
	{
		m_status = status_t::in_progress;
	}
	
	inline void
	start_duration_timer()
	{
		m_duration_timer.start();
	}
	
	inline std::chrono::milliseconds
	get_duration()
	{
		m_duration_timer.check();
		return m_duration_timer.elapsed();
	}
	
protected:

	status_t			m_status = status_t::inactive;
	timer				m_duration_timer;
};


template <typename T>
class async_iterator : public async_repeater, public std::enable_shared_from_this<async_iterator<T>>
{
public:

	typedef std::shared_ptr<stepper<T>> stepper_ptr;
	typedef std::shared_ptr<async_iterator<T>> iterator_ptr;

	typedef std::function< void (iterator_ptr iptr) > attempt_f;
	typedef std::function< void (iterator_ptr iptr) > failure_f;

	async_iterator(stepper_ptr _stepper, attempt_f _attempt, failure_f _failure)
	:
		m_stepper(_stepper),
		m_attempt(_attempt),
		m_failure(_failure)
	{
	}

	virtual
	~async_iterator()
	{
		mlog( marker::async_iter, log::level_t::info, "async_iterator destructor called");
		if (!is_aborted())
		{
			abort();
		}
	}
	
	inline stepper_ptr
	get_stepper() const
	{
		return m_stepper;
	}

	virtual counter_ptr
	get_counter() const
	{
		return get_stepper();
	}
	
	virtual base_limiter_ptr
	get_limiter() const
	{
		return get_stepper();
	}
	
	virtual void
	attempt()
	{
		if (is_in_progress())
		{
			mlog( marker::async_iter, log::level_t::info, "entering attempt, status was in_progress");
		}
		else
		{
			mlog( marker::async_iter, log::level_t::info, "entering attempt, status was %", status_message());
		}
		set_in_progress();
		start_duration_timer();
		m_attempt(this->shared_from_this());
	}
	
	virtual void
	attempt_failed()
	{
		set_failed();
		
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
	
	virtual void
	attempt_succeeded()
	{
		set_succeeded();
	}

protected:
	stepper_ptr m_stepper;
	attempt_f m_attempt;
	failure_f m_failure;
//	timer m_timer;
};


class async_retry_with_timeout : public async_repeater, public std::enable_shared_from_this<async_retry_with_timeout>
{
public:

	
	typedef std::shared_ptr<scheduler> scheduler_ptr;
	typedef std::shared_ptr<async_retry_with_timeout> retry_ptr;
	
	typedef std::function< void (retry_ptr rptr) > attempt_f;
	typedef std::function< void (retry_ptr rptr) > failure_f;
	typedef std::function< void (retry_ptr rptr) > timed_out_f;


	async_retry_with_timeout(scheduler_ptr _scheduler,
		attempt_f _attempt,
		timed_out_f _timed_out,
		failure_f _failure)
	:
		m_scheduler(_scheduler),
		m_attempt(_attempt),
		m_timed_out(_timed_out),
		m_failure(_failure)
	{
	}
	
	virtual
	~async_retry_with_timeout()
	{
		mlog( marker::async_iter, log::level_t::info, "async_retry_with_timeout destructor called");
		if (!is_aborted())
		{
			abort();
		}
	}

	virtual void
	abort()
	{
		mlog( marker::async_iter, log::level_t::info, "async_retry_with_timeout::abort called");
		async_repeater::abort();
		mlog( marker::async_iter, log::level_t::info, "canceling timer");
		if (m_timeout_timer != nullptr)
		{
			cancel_timer();
		}
	}
	
	inline scheduler_ptr
	get_scheduler() const
	{
		return m_scheduler;
	}
	
	virtual counter_ptr
	get_counter() const
	{
		return get_scheduler();
	}
	
	virtual base_limiter_ptr
	get_limiter() const
	{
		return get_scheduler();
	}
	
	virtual void
	attempt_failed();
	
	virtual void
	attempt_succeeded();

	void
	cancel_timer();
	
	virtual void
	attempt();

	void
	schedule_next_timeout(nodeoze::runloop::event_f handler);

protected:

	scheduler_ptr m_scheduler;
	attempt_f m_attempt;
	timed_out_f m_timed_out;
	failure_f m_failure;
	nodeoze::runloop::event m_timeout_timer = nullptr;
};

}

#if defined( WIN32 )
#	pragma warning( pop )
#endif

#endif
