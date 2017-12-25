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
 
#ifndef	_nodeoze_macros_h
#define	_nodeoze_macros_h

#include <nodeoze/nlog.h>

#define	ncheck_warning( X, MESSAGE, ... )								\
do																		\
{																		\
	if ( !( X ) )														\
	{																	\
		nlog( nodeoze::log::level_t::warning, MESSAGE, ##__VA_ARGS__ );	\
	}																	\
}																		\
while ( 0 )

#define ncheck_error( X, LABEL, MESSAGE, ...)							\
{																		\
do																		\
{																		\
	if ( !( X ) )														\
	{																	\
		nlog( nodeoze::log::level_t::error, MESSAGE, ##__VA_ARGS__ );	\
		nrequire_failed_test();											\
		goto LABEL;														\
	}																	\
}																		\
while ( 0 );															\
}

#define	ncheck_error_quiet( X, LABEL )									\
{																		\
do																		\
{																		\
	if ( !( X ) )														\
	{																	\
		nrequire_failed_test();											\
		goto LABEL;														\
	}																	\
}																		\
while( 0 );																\
}

#define ncheck_error_action( X, ACTION, LABEL, MESSAGE, ...)			\
{																		\
do																		\
{																		\
	if ( !( X ) )														\
	{																	\
		nlog( nodeoze::log::level_t::error, MESSAGE, ##__VA_ARGS__ );	\
		nrequire_failed_test();											\
		{ ACTION; }														\
		goto LABEL;														\
	}																	\
}																		\
while ( 0 );															\
}

#define ncheck_error_action_quiet( X, ACTION, LABEL )					\
{																		\
do																		\
{																		\
	if ( !( X ) )														\
	{																	\
		nrequire_failed_test();											\
		{ ACTION; }														\
		goto LABEL;														\
	}																	\
}																		\
while ( 0 );															\
}

#define nunused(x) ((void)(x))

#define nglue(x, y) x y

#define nreturn_args_count(_1_, _2_, _3_, _4_, _5_, count, ...) count
#define nexpand_args(args) nreturn_args_count args
#define ncount_args_max5(...) nexpand_args((__VA_ARGS__, 5, 4, 3, 2, 1, 0))

#define noverload_macro2(name, count) name##count
#define noverload_macro1(name, count) noverload_macro2(name, count)
#define noverload_macro(name, count) noverload_macro1(name, count)

#define ncall_overload(name, ...) nglue(noverload_macro(name, ncount_args_max5(__VA_ARGS__)), (__VA_ARGS__))


#define ndefine_global_static2( NAME, TYPE )		\
static inline TYPE&											\
NAME()													\
{														\
	static auto global	= new TYPE( );			\
	return *global;										\
}


#define ndefine_global_static3( NAME, TYPE, PARAM1 )		\
static inline TYPE&											\
NAME()													\
{														\
	static auto global	= new TYPE( PARAM1 );			\
	return *global;										\
}


#define ndefine_global_static4( NAME, TYPE, PARAM1, PARAM2 )		\
static inline TYPE&											\
NAME()													\
{														\
	static auto global	= new TYPE( PARAM1, PARAM2 );			\
	return *global;										\
}


#define ndefine_global_static5( NAME, TYPE, PARAM1, PARAM2, PARAM3 )		\
static inline TYPE&											\
NAME()													\
{														\
	static auto global	= new TYPE( PARAM1, PARAM2, PARAM3 );			\
	return *global;										\
}

#define ndefine_global_static( ... ) ncall_overload( ndefine_global_static, __VA_ARGS__ )

#if defined( WIN32 )

#	define ntext( X ) TEXT( X )

#else

#	define ntext( X ) X

#endif

extern void
nrequire_failed_test();

#endif
