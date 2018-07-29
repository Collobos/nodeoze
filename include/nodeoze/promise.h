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

#ifndef _nodeoze_promise_h
#define _nodeoze_promise_h

#include <nodeoze/runloop.h>
#include <nodeoze/macros.h>
#include <nodeoze/deque.h>
#include <functional>
#include <system_error>
#include <memory>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cassert>

namespace nodeoze {

template< typename T >
class promise;


template< typename T >
inline std::ostream&
operator<<( std::ostream &os, const deque< T > &v )
{
	auto first = true;
	
	os << "[ ";
	
	for ( auto &val : v )
	{
		if ( !first )
		{
			os << ", ";
		}
		else
		{
			first = false;
		}
		
		os << v;
	}
	
	os << "]";
	
	return os;
}

template< typename T >
inline std::ostream&
operator<<( std::ostream &os, const nodeoze::promise< T > &p )
{
	nunused( p );
	return os;
}

template< typename T >
struct is_promise : public std::integral_constant< bool, false >
{
};

template< typename T >
struct is_promise< promise< T > > : public std::integral_constant< bool, true >
{
};

template< typename T >
struct is_void_promise : public std::integral_constant< bool, false >
{
};

template<>
struct is_void_promise< promise< void > > : public std::integral_constant< bool, true >
{
};

template< class T >
struct __promise_shared
{
	typedef std::function< void ( T&& ) >				resolve_f;
	typedef std::function< void ( std::error_code ) >	reject_f;
	typedef std::function< void () >					finally_f;
	typedef deque< T >									maybe_array_type;
	
	bool				resolved;
	bool				rejected;
	resolve_f			resolve;
	reject_f			reject;
	finally_f			finally;
	runloop::event		timer;
	std::uint32_t		refs;
	T					val;
	std::error_code		err;
};

template<>
struct __promise_shared< void >
{
	typedef std::function< void () >					resolve_f;
	typedef std::function< void ( std::error_code ) >	reject_f;
	typedef std::function< void () >					finally_f;
	typedef void										maybe_array_type;
	
	bool				resolved;
	bool				rejected;
	resolve_f			resolve;
	reject_f			reject;
	finally_f			finally;
	runloop::event		timer;
	std::uint32_t		refs;
	std::error_code		err;
};

template< class T >
class promise
{
public:

	typedef deque< promise< T > >					promises_type;
	typedef deque< promise< T > >					array_type;
	typedef __promise_shared< T >					shared_type;
	typedef typename shared_type::maybe_array_type	maybe_array_type;

	template< class Q = T >
	static typename std::enable_if< std::is_void< Q >::value, promise< T > >::type
	build()
	{
		auto ret = promise< T >();

		ret.resolve();

		return ret;
	}

	template< class Q = T >
	static typename std::enable_if< !std::is_void< Q >::value, promise< T > >::type
	build( Q &&val )
	{
		auto ret = promise< T >();

		ret.resolve( std::move( val ) );

		return ret;
	}

	template< class Q = T >
	static typename std::enable_if< !std::is_void< Q >::value, promise< T > >::type
	build( const Q &val )
	{
		auto ret = promise< T >();

		ret.resolve( val );

		return ret;
	}

	static promise< T >
	build( std::error_code err )
	{
		auto ret = promise< T >();

		ret.reject( err );

		return ret;
	}

	static promise< maybe_array_type >
	all( promises_type promises );

	static promise< T >
	race( promises_type promises );
	
	static promise< T >
	any( promises_type promises );
	
	static bool
	are_all_finished( const promises_type &promises )
	{
		bool ok = true;

		for ( auto &promise : promises )
		{
			if ( !promise.is_finished() )
			{
				ok = false;
				break;
			}
		}

		return ok;
	}

	promise()
	:
		m_shared( new __promise_shared< T > )
	{
		m_shared->resolved	= false;
		m_shared->rejected	= false;
		m_shared->resolve	= nullptr;
		m_shared->reject	= nullptr;
		m_shared->timer		= nullptr;
		m_shared->refs		= 1;
	}

	promise( const promise &rhs )
	:
		m_shared( nullptr )
	{
		copy( rhs );
	}

	promise( promise &&rhs )
	:
		m_shared( nullptr )
	{
		move( std::move( rhs ) );
	}

	~promise()
	{
		unshare();
	}

	inline const promise&
	operator=( const promise &rhs )
	{
		copy( rhs );
		return *this;
	}

	inline const promise&
	operator=( promise &&rhs )
	{
		move( std::move( rhs ) );
		return *this;
	}
	
	template< class Q = T >
	typename std::enable_if< std::is_void< Q >::value >::type
	resolve()
	{
		assert( m_shared );

		if ( m_shared && !m_shared->resolved && !m_shared->rejected )
		{
			m_shared->resolved = true;

			if ( m_shared->resolve )
			{
				auto resolve = std::move( m_shared->resolve );
				resolve();
				m_shared->reject = nullptr;

				if ( m_shared->finally )
				{
					auto finally = std::move( m_shared->finally );
					finally();
				}
			}
		}
	}

	template< class Q = T >
    typename std::enable_if< !std::is_void< Q >::value, void >::type
	resolve( Q &&val )
	{
		assert( m_shared );

		if ( m_shared && !m_shared->resolved && !m_shared->rejected )
		{
			m_shared->resolved = true;
		
			if ( m_shared->resolve )
			{
				auto resolve = std::move( m_shared->resolve );
				resolve( std::move( val ) );
				m_shared->reject	= nullptr;

				if ( m_shared->finally )
				{
					auto finally = std::move( m_shared->finally );
					finally();
				}
			}
			else
			{
				m_shared->val = std::move( val );
			}
		}
	}

	template< class Q = T >
    typename std::enable_if< !std::is_void< Q >::value, void >::type
	resolve( const Q &val )
	{
		assert( m_shared );

		if ( m_shared && !m_shared->resolved && !m_shared->rejected )
		{
			m_shared->resolved = true;
			
			if ( m_shared->resolve )
			{
				auto resolve = std::move( m_shared->resolve );
				resolve( val );
				m_shared->reject = nullptr;

				if ( m_shared->finally )
				{
					auto finally = std::move( m_shared->finally );
					finally();
				}
			}
			else
			{
				m_shared->val = val;
			}
		}
	}
	
	void
	reject( std::error_code err )
	{
		assert( m_shared );

		if ( m_shared && !m_shared->resolved && !m_shared->rejected )
		{
			m_shared->err		= err;
			m_shared->rejected	= true;

			if ( m_shared->reject )
			{
				auto reject = std::move( m_shared->reject );
				reject( err );
				m_shared->resolve = nullptr;

				if ( m_shared->finally )
				{
					auto finally = std::move( m_shared->finally );
					finally();
				}
			}
			else
			{
				m_shared->err = err;
			}
		}
	}
	
	inline bool
	is_resolved() const
	{
		return ( m_shared ? m_shared->resolved : false );
	}

	inline bool
	is_rejected() const
	{
		return ( m_shared ? m_shared->rejected : false );
	}

	inline bool
	is_finished() const
	{
		return ( is_resolved() || is_rejected() );
	}

	promise< T >
	timeout( std::chrono::milliseconds timeout )
	{
		arm_timer( timeout );

		return *this;
	}

	template< typename Func >
	auto
	then( Func &&func )
	{
		return then( std::forward< Func >( func ), nullptr );
	}

	template< typename Result, typename Func >
	auto
	then( Func &&func )
	{
		return then< Result >( std::forward< Func >( func ), nullptr );
	}

	template< typename Resolve, typename Reject >
	auto
	then( Resolve &&resolve_func, Reject &&reject_func )
	{
		return really_then( std::forward< Resolve >( resolve_func ), std::forward< Reject >( reject_func ) );
	}
	
	template< typename Result, typename Resolve, typename Reject >
	auto
	then( Resolve &&resolve_func, Reject &&reject_func )
	{
		return really_then< Result >( std::forward< Resolve >( resolve_func ), std::forward< Reject >( reject_func ) );
	}
	
	template< class Q = T >
    typename std::enable_if< std::is_void< Q >::value, void >::type
	then( promise< Q > &&rhs )
	{
		if ( m_shared->resolved )
		{
			rhs.resolve();
		}
		else if ( m_shared->rejected )
		{
			rhs.reject( m_shared->err, m_shared->file, m_shared->func, m_shared->line );
		}
		else
		{
			assert( rhs.m_shared->resolve != nullptr );
			m_shared->resolve	= rhs.m_shared->resolve;
			m_shared->reject	= rhs.m_shared->reject;
		}
	}

	template< class Q = T >
    typename std::enable_if< !std::is_void< Q >::value, void >::type
	then( promise< Q > &&rhs )
	{
		if ( m_shared->resolved )
		{
			rhs.resolve( std::move( m_shared->val ) );
		}
		else if ( m_shared->rejected )
		{
			rhs.reject( m_shared->err );
		}
		else
		{
			assert( rhs.m_shared->resolve != nullptr );
			m_shared->resolve	= rhs.m_shared->resolve;
			m_shared->reject	= rhs.m_shared->reject;
		}
	}

	template< class F >
	promise< T >&
	catcher( F func )
	{
		if ( m_shared )
		{
			if ( is_finished() )
			{
				if ( m_shared->rejected )
				{
					func( m_shared->err );
				}
			}
			else
			{
				m_shared->reject = std::move( func );
			}
		}

		return *this;
	}

	template< class F >
	promise< T >&
	finally( F func )
	{
		if ( m_shared )
		{
			if ( is_finished() )
			{
				func();
			}
			else
			{
				m_shared->finally = std::move( func );
			}
		}

		return *this;
	}

	void
	reset()
	{
		unshare();

		m_shared = new __promise_shared< T >;

		m_shared->resolved	= false;
		m_shared->rejected	= false;
		m_shared->resolve	= nullptr;
		m_shared->reject	= nullptr;
		m_shared->finally	= nullptr;
		m_shared->refs		= 1;
	}

private:

	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve() >::type,
		typename std::enable_if< !is_promise< ResolveResult >::value && std::is_void< Q >::value && std::is_void< ResolveResult >::value >::type* = nullptr
	>
	promise< T >&
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		m_shared->resolve = std::move( resolve_func );
		m_shared->reject = std::move( reject_func );
    
    	maybe_direct_resolve_reject();

		return *this;
	}

	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve( Q&& )>::type,
		typename std::enable_if< !is_promise< ResolveResult >::value && !std::is_void< Q >::value && std::is_void< ResolveResult >::value >::type* = nullptr
	>
	promise< T >&
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		m_shared->resolve = std::move( resolve_func );
		m_shared->reject = std::move( reject_func );
    
    	maybe_direct_resolve_reject();

		return *this;
	}

	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve() >::type,
		typename std::enable_if< !is_promise< ResolveResult >::value && std::is_void< Q >::value && !std::is_void< ResolveResult >::value >::type* = nullptr
	>
	promise< ResolveResult >
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		nunused( reject_func );
		
		auto ret = promise< ResolveResult >();

		m_shared->resolve = [=]() mutable
		{
			ret.resolve( std::move( resolve_func() ) );
		};

		m_shared->reject = [=]( std::error_code err ) mutable
		{
			ret.reject( err );
		};
		
		maybe_direct_resolve_reject();
		
		return ret;
	}

	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve( Q&& ) >::type,
		typename std::enable_if< !is_promise< ResolveResult >::value && !std::is_void< Q >::value && !std::is_void< ResolveResult >::value >::type* = nullptr
	>
	promise< ResolveResult >
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		nunused( reject_func );
		
		auto ret = promise< ResolveResult >();
		
		m_shared->resolve = [=]( Q &&val ) mutable
		{
			ret.resolve( std::move( resolve_func( std::move( val ) ) ) );
		};

		m_shared->reject = [=]( std::error_code err ) mutable
		{
			ret.reject( err );
		};
		
		maybe_direct_resolve_reject();
		
		return ret;
	}
	
	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve() >::type,
		typename std::enable_if< is_void_promise< ResolveResult >::value && std::is_void < Q >::value >::type* = nullptr
	>
	ResolveResult
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		nunused( reject_func );
		
		ResolveResult ret;
		
		m_shared->resolve	= [=]() mutable
		{
			resolve_func().then( [=]() mutable
			{
				ret.resolve();
			},
			[=]( std::error_code err ) mutable
			{
				ret.reject( err );
			} );
		};

		m_shared->reject	= [=]( std::error_code err ) mutable
		{
			ret.reject( err );
		};
		
		maybe_direct_resolve_reject();

		return ret;
	}
	
	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve() >::type,
		typename std::enable_if< is_promise< ResolveResult >::value && !is_void_promise< ResolveResult >::value && std::is_void < Q >::value >::type* = nullptr
	>
	ResolveResult
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		nunused( reject_func );
		
		ResolveResult ret;
		
		m_shared->resolve	= [=]() mutable
		{
			resolve_func().then( [=]( auto answer ) mutable
			{
				ret.resolve( std::move( answer ) );
			},
			[=]( std::error_code err ) mutable
			{
				ret.reject( err );
			} );
		};

		m_shared->reject	= [=]( std::error_code err ) mutable
		{
			ret.reject( err );
		};
		
		maybe_direct_resolve_reject();

		return ret;
	}

	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve( Q&& ) >::type,
		typename std::enable_if< is_promise< ResolveResult >::value && is_void_promise< ResolveResult >::value && !std::is_void < Q >::value >::type* = nullptr
	>
	ResolveResult
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		nunused( reject_func );
		
		ResolveResult ret;
		
		m_shared->resolve	= [=]( Q &&val ) mutable
		{
			resolve_func( std::move( val ) ).then( [=]() mutable
			{
				ret.resolve();
			},
			[=]( std::error_code err ) mutable
			{
				ret.reject( err );
			} );
		};

		m_shared->reject	= [=]( std::error_code err ) mutable
		{
			ret.reject( err );
		};
		
		maybe_direct_resolve_reject();

		return ret;
	}

	template<
		typename Resolve,
		typename Reject,
		typename Q = T,
		typename ResolveResult = typename std::result_of< Resolve( Q&& ) >::type,
		typename std::enable_if< is_promise< ResolveResult >::value && !is_void_promise< ResolveResult >::value && !std::is_void < Q >::value >::type* = nullptr
	>
	ResolveResult
	really_then( Resolve &&resolve_func, Reject &&reject_func )
	{
		nunused( reject_func );
		
		ResolveResult ret;
		
		m_shared->resolve	= [=]( Q &&val ) mutable
		{
			resolve_func( val ).then( [=]( auto &&val ) mutable
			{
				ret.resolve( std::move( val ) );
			},
			[=]( std::error_code err ) mutable
			{
				ret.reject( err );
			} );
		};

		m_shared->reject	= [=]( std::error_code err ) mutable
		{
			ret.reject( err );
		};
		
		maybe_direct_resolve_reject();

		return ret;
	}
	
	
	template< class Q = T >
    typename std::enable_if< std::is_void< Q >::value, void >::type
	maybe_direct_resolve_reject()
	{
		if ( is_rejected() )
		{
			if ( m_shared->reject )
			{
				m_shared->reject( m_shared->err );
			}

			if ( m_shared->finally )
			{
				auto finally = std::move( m_shared->finally );
				finally();
			}
		}
		else if ( is_resolved() )
		{
			if ( m_shared->resolve )
			{
				m_shared->resolve();
			}

			if ( m_shared->finally )
			{
				auto finally = std::move( m_shared->finally );
				finally();
			}
		}
	}
	
	template< class Q = T >
    typename std::enable_if< !std::is_void< Q >::value, void >::type
	maybe_direct_resolve_reject()
	{
		if ( is_rejected() )
		{
			if ( m_shared->reject )
			{
				m_shared->reject( m_shared->err );
			}
		}
		else if ( is_resolved() )
		{
			if ( m_shared->resolve )
			{
				m_shared->resolve( std::move( m_shared->val ) );
			}
		}
	}

	inline void
	share( const promise &rhs )
	{
		m_shared = rhs.m_shared;

		if ( m_shared )
		{
			m_shared->refs++;
		}
	}

	inline void
	unshare()
	{
		if ( m_shared && ( --m_shared->refs == 0 ) )
		{
			cancel_timer();
			delete m_shared;
			m_shared = nullptr;
		}
	}

	inline void
	copy( const promise &rhs )
	{
		unshare();
		share( rhs );
	}

	inline void
	move( promise &&rhs )
	{
		unshare();

		m_shared		= rhs.m_shared;
		rhs.m_shared	= nullptr;
	}

	inline void
	arm_timer( std::chrono::milliseconds timeout )
	{
		cancel_timer();

		if ( m_shared )
		{
			m_shared->timer = runloop::shared().create( timeout );

			auto copy = *this;

			runloop::shared().schedule( m_shared->timer, [copy]( auto event ) mutable
			{
				nunused( event );

				copy.cancel_timer();

				if ( !copy.is_finished() )
				{
					copy.reject( make_error_code( std::errc::timed_out ) );
				}
			} );
		}
	}

	inline void
	cancel_timer()
	{
		if ( m_shared && m_shared->timer )
		{
			runloop::shared().cancel( m_shared->timer );
			m_shared->timer = nullptr;
		}
	}
	
	__promise_shared< T > *m_shared = nullptr;
};

}

template< class T >
inline nodeoze::promise< typename nodeoze::promise< T >::maybe_array_type >
nodeoze::promise< T >::all( promises_type promises )
{
	auto count			= std::make_shared< std::uint32_t >( 0 );
	auto total			= promises.size();
	auto is_finished	= [count, total]()
	{
		return *count == total;
	};
	auto ret			= promise< deque< T > >();
	
	if ( promises.size() > 0 )
	{
		auto result	= std::make_shared< deque< T > >( total );
		auto index	= 0;
		
		for ( auto &promise : promises )
		{
			promise.then( [=]( T val ) mutable
			{
				( *count )++;
				
				if ( !ret.is_finished() )
				{
					result->at( index ) = std::move( val );

					if ( is_finished() )
					{
						ret.resolve( std::move( *result ) );
					}
				}
			},
			[=]( auto err ) mutable
			{
				( *count )++;
				
				if ( !ret.is_finished() )
				{
					ret.reject( err );
				}
			} );

			index++;
		}
	}
	else
	{
		ret.resolve( deque< T >() );
	}

	return ret;
}

template<>
inline nodeoze::promise< typename nodeoze::promise< void >::maybe_array_type >
nodeoze::promise< void >::all( promises_type promises )
{
	auto count			= std::make_shared< std::uint32_t >( 0 );
	auto total			= promises.size();
	auto is_finished	= [count, total]()
	{
		return *count == total;
	};
	auto ret			= promise< void >();
	
	if ( promises.size() > 0 )
	{
		for ( auto &promise : promises )
		{
			promise.then( [=]() mutable
			{
				( *count )++;

				if ( !ret.is_finished() && is_finished() )
				{
					ret.resolve();
				}
			},
			[=]( auto err ) mutable
			{
				( *count )++;

				if ( !ret.is_finished() )
				{
					ret.reject( err );
				}
			} );
		}
	}
	else
	{
		ret.resolve();
	}

	return ret;
}

template< class T >
inline nodeoze::promise< T >
nodeoze::promise< T >::race( promises_type promises )
{
	promise< T > ret;
	
	if ( promises.size() > 0 )
	{
		for ( auto &promise : promises )
		{
			promise.then( [=]( T val ) mutable
			{
				if ( !ret.is_finished() )
				{
					ret.resolve( std::move( val ) );
				}
			},
			[=]( std::error_code err ) mutable
			{
				if ( !ret.is_finished() )
				{
					ret.reject( err );
				}
			} );
		}
	}
	else
	{
		ret.reject( make_error_code( std::errc::invalid_argument ) );
	}

	return ret;
}

template<>
inline nodeoze::promise< void >
nodeoze::promise< void >::race( promises_type promises )
{
	promise< void > ret;
	
	if ( promises.size() > 0 )
	{
		for ( auto &promise : promises )
		{
			promise.then( [=]() mutable
			{
				if ( !ret.is_finished() )
				{
					ret.resolve();
				}
			},
			[=]( std::error_code err ) mutable
			{
				if ( !ret.is_finished() )
				{
					ret.reject( err );
				}
			} );
		}
	}
	else
	{
		ret.reject( make_error_code( std::errc::invalid_argument ) );
	}

	return ret;
}

template< class T >
inline nodeoze::promise< T >
nodeoze::promise< T >::any( promises_type promises )
{
	auto count			= std::make_shared< std::uint32_t >( 0 );
	auto total			= promises.size();
	auto is_finished	= [count, total]()
	{
		return *count == total;
	};
	auto ret			= promise< T >();
	
	if ( promises.size() > 0 )
	{
		for ( auto &promise : promises )
		{
			promise.then( [=]( T val ) mutable
			{
				( *count )++;
				
				if ( !ret.is_finished() )
				{
					ret.resolve( std::move( val ) );
				}
			},
			[=]( std::error_code err ) mutable
			{
				( *count )++;
				
				if ( !ret.is_finished() && is_finished() )
				{
					ret.reject( err );
				}
			} );
		}
	}
	else
	{
		ret.reject( make_error_code( std::errc::invalid_argument ) );
	}

	return ret;
}

template<>
inline nodeoze::promise< void >
nodeoze::promise< void >::any( promises_type promises )
{
	auto count			= std::make_shared< std::uint32_t >( 0 );
	auto total			= promises.size();
	auto is_finished	= [count, total]()
	{
		return *count == total;
	};
	auto ret			= promise< void >();
	
	if ( promises.size() > 0 )
	{
		for ( auto &promise : promises )
		{
			promise.then( [=]() mutable
			{
				( *count )++;
				
				if ( !ret.is_finished() )
				{
					ret.resolve();
				}
			},
			[=]( auto err ) mutable
			{
				( *count )++;
				
				if ( !ret.is_finished() && is_finished() )
				{
					ret.reject( err );
				}
			} );
		}
	}
	else
	{
		ret.reject( make_error_code( std::errc::invalid_argument ) );
	}

	return ret;
}

#endif
