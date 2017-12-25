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
 
#ifndef _nodeoze_concurrent_h
#define _nodeoze_concurrent_h

#include <mutex>
#include <list>

namespace nodeoze {

namespace concurrent {

template < class Data >
class queue
{
public:

	typedef std::function< bool ( Data& ) > func;

	inline queue()
	{
	}

	inline ~queue()
	{
	}

	inline void
	emplace( Data &&data )
	{
		std::lock_guard< std::recursive_mutex > guard( m_mutex );
		m_queue.emplace_back( std::move( data ) );
	}

	inline bool
	empty() const
	{
		std::lock_guard< std::recursive_mutex > guard( m_mutex );
		return m_queue.empty();
	}

	inline bool
	try_pop( Data &value )
	{
		bool ok = false;

		std::lock_guard< std::recursive_mutex > guard( m_mutex );

		if ( !m_queue.empty() )
		{
			value = m_queue.front();
			m_queue.pop_front();
			ok = true;
		}

		return ok;
	}

	inline void
	prune( func f )
	{
		std::lock_guard< std::recursive_mutex > guard( m_mutex );

		auto it = m_queue.begin();

		while ( it != m_queue.end() )
		{
			if ( f( *it ) )
			{
				it = m_queue.erase( it );
			}
			else
			{
				++it;
			}
		}
	}

	inline size_t
	size() const
	{
		std::lock_guard< std::recursive_mutex > guard( m_mutex );

		return m_queue.size();
	}

private:

	std::list< Data >		m_queue;
	std::recursive_mutex	m_mutex;
};

}

}

#endif
