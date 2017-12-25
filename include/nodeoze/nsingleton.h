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

#ifndef _nodeoze_nsingleton_h
#define _nodeoze_nsingleton_h

#include <cstdlib>
#include <memory>
#include <mutex>

#define NODEOZE_DECLARE_SINGLETON( X )	\
public:									\
										\
	static X&							\
	shared();							\
										\
protected:								\
										\
	static X*							\
	create();							\
										\
	static X				*m_shared;	\
	static std::once_flag   m_once;


#define NODEOZE_DECLARE_COMMAND_LINE_SINGLETON( X )	\
public:												\
													\
	static X&										\
	shared();										\
													\
	static X&										\
	shared( int argc, std::tchar_t **argv );		\
													\
protected:											\
													\
	static X*										\
	create( int argc, std::tchar_t **argv );		\
													\
	static X				*m_shared;				\
	static std::once_flag   m_once;


#define NODEOZE_DECLARE_ABSTRACT_SINGLETON( X )	\
public:											\
												\
	static inline X& 							\
	shared()									\
	{											\
		return *m_shared;						\
	}											\
												\
private:										\
												\
	static X *m_shared;							\

#define NODEOZE_DECLARE_ASYNC_SINGLETON( X )	\
public:											\
												\
	template< class T >							\
	static inline void							\
	shared( T functor )							\
	{											\
		if ( m_shared != nullptr )				\
		{										\
			functor( *m_shared );				\
		}										\
		else									\
		{										\
			create_async( functor );			\
		}										\
	}											\
												\
private:										\
												\
	static X *m_shared;


#define NODEOZE_DEFINE_SINGLETON( X )	\
std::once_flag	X::m_once;				\
X*				X::m_shared = nullptr;	\
										\
X&										\
X::shared()								\
{										\
	std::call_once( m_once, []()		\
	{									\
		m_shared = create();			\
	} );								\
										\
	return *m_shared;					\
}

#define NODEOZE_DEFINE_COMMAND_LINE_SINGLETON( X )	\
std::once_flag	X::m_once;							\
X*				X::m_shared = nullptr;				\
													\
X&													\
X::shared()										\
{													\
	assert( m_shared );								\
	return *m_shared;								\
}													\
													\
X&													\
X::shared( int argc, std::tchar_t **argv )		\
{													\
	std::call_once( m_once, [argc,argv]()			\
	{												\
		m_shared = create( argc, argv );			\
	} );											\
													\
	return *m_shared;								\
}

#endif
