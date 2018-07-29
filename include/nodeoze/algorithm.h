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

#ifndef _nodeoze_nalgorithm_h
#define _nodeoze_nalgorithm_h

#include <unordered_set>

namespace nodeoze {

typedef std::function< void () > next_f;
typedef std::function< void ( bool equal ) > async_equal_f;

template<class InputIterator, class Visitor, class CompletionHandler>
void
for_each_async( InputIterator begin, InputIterator end, Visitor visitor, CompletionHandler completion )
{
	if ( begin != end )
	{
		visitor( *begin, [=]() mutable
		{
			for_each_async( ++begin, end, visitor, completion );
		} );
	}
	else
	{
		completion();
	}
}

template<class InputIterator, class AsyncUnaryPredicate, class CompletionHandler>
void
find_if_async( InputIterator begin, InputIterator end, AsyncUnaryPredicate predicate, CompletionHandler completion)
{
	if ( begin != end )
	{
		predicate( *begin, [=]( bool equal ) mutable
		{
			if ( equal )
			{
				completion( begin );
			}
			else
			{
				find_if_async( ++begin, end, predicate, completion );
			}
		} );
	}
	else
	{
		completion( end );
	}
}

template<class S, class F>
void
unordered_set_difference( const std::unordered_set<S> &first, const std::unordered_set<S> &second, F functor )
{
	std::unordered_set<S> diff;
	using iter = typename std::unordered_set<S>::const_iterator;
	for ( iter it = first.begin(); it != first.end(); ++it )
	{
		if ( second.count( *it ) < 1 )
		{
			diff.emplace( *it );
		}
	}
	functor( diff );
}


}

#endif
