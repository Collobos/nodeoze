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

/* Inspired by
 *
 * Boris Kolpackov - http://www.codesynthesis.com
 *
 */

#ifndef _nodeoze_buffer_h
#define _nodeoze_buffer_h

#include <functional>
#include <algorithm>
#include <stdexcept>
#include <cstddef>
#include <cstring>
#include <string>
#include <iomanip>
#include <iostream>
#include <vector>

namespace nodeoze {

#ifndef DOCTEST_CONFIG_DISABLE
namespace detail
{
	class buffer_test_probe;
}
#endif

class buffer
{
public:

#ifndef DOCTEST_CONFIG_DISABLE
	friend class detail::buffer_test_probe;
#endif
	using elem_type = std::uint8_t;
	using size_type =  std::size_t;
	using position_type = std::uint64_t;
	using offset_type = std::int64_t;
	using checksum_type = std::uint32_t;

	using dealloc_function =  std::function< void ( elem_type *data ) >;
	using realloc_function =  std::function< elem_type* ( elem_type *data, size_type current_size, size_type new_size ) >;
	static dealloc_function default_dealloc;
	static realloc_function default_realloc;

	enum class policy
	{
		copy_on_write,
		no_copy_on_write,
		exclusive
	};

private:

	class _buffer_shared
	{
	public:

#ifndef DOCTEST_CONFIG_DISABLE
		friend class detail::buffer_test_probe;
#endif
		friend class buffer;

		_buffer_shared()
		:
		m_data{ nullptr },
		m_size{ 0 },
		m_refs{ 1 },
		m_dealloc{ default_dealloc },
		m_realloc{ default_realloc },
		m_policy{ policy::copy_on_write }
		{}

		_buffer_shared( policy pol )
		:
		m_data{ nullptr },
		m_size{ 0 },
		m_refs{ 1 },
		m_dealloc{ default_dealloc },
		m_realloc{ default_realloc },
		m_policy{ pol }
		{}

	private:

		void initialize( const void* src )
		{
			if ( ! m_data )
			{
				if ( m_size > 0 )
				{
					m_data = m_realloc( nullptr, 0, m_size );
					if ( ! m_data )
					{
						// this is only called from constructors; fling feces.
						throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
					}
					if ( src )
					{
						::memcpy( m_data, src, m_size );
					}
				}
			}
		}

		_buffer_shared( const void *src, size_type nbytes, policy pol = policy::copy_on_write )
		:
		m_data{ nullptr },
		m_size{ nbytes },
		m_refs{ 1 },
		m_dealloc{ default_dealloc },
		m_realloc{ default_realloc },
		m_policy{ pol }
		{
			initialize( src );
		}

		_buffer_shared( void* data, size_type nbytes, policy pol, dealloc_function dealloc, realloc_function realloc )
		:
		m_data{ reinterpret_cast< elem_type* >( data ) },
		m_size{ nbytes },
		m_refs{ 1 },
		m_dealloc{ dealloc },
		m_realloc{ realloc },
		m_policy{ pol }
		{
			initialize( nullptr );
		}

		_buffer_shared( _buffer_shared const& rhs, policy pol = policy::copy_on_write )
		:
		m_data{ nullptr },
		m_size{ rhs.m_size },
		m_refs{ 1 },
		m_dealloc{ default_dealloc },
		m_realloc{ default_realloc },
		m_policy{ pol }
		{
			initialize( rhs.m_data );
		}
	
		_buffer_shared( size_type nbytes, policy pol = policy::copy_on_write )
		:
		m_data{ nullptr },
		m_size{ nbytes },
		m_refs{ 1 },
		m_dealloc{ default_dealloc },
		m_realloc{ default_realloc },
		m_policy{ pol }
		{
			initialize( nullptr );
		}

		_buffer_shared( std::vector< elem_type > const& vec, policy pol = policy::copy_on_write )
		:
		m_data{ nullptr },
		m_size{ vec.size() },
		m_refs{ 1 },
		m_dealloc{ default_dealloc },
		m_realloc{ default_realloc },
		m_policy{ pol }
		{
			initialize( vec.data() );
		}

		~_buffer_shared()
		{
			assert( m_refs == 0 );
			if ( m_data )
			{
				if ( m_dealloc )
				{
					m_dealloc( m_data );
				}
			}
		}

		inline elem_type*
		detach()
		{
			elem_type* result = nullptr;

			if ( m_refs == 1 )
			{
				result =  m_data;
				m_data = nullptr;
				m_size = 0;	
				m_refs = 0;
				m_dealloc = default_dealloc;
				m_realloc = default_realloc;
				m_policy = policy::copy_on_write;
			}

			return result;
		}

		inline elem_type*
		detach( size_type& size, dealloc_function& dealloc )
		{
			// if detach() doesn't work, don't side-effect the arguments--be nice.
			auto tmp_size = m_size;
			auto tmp_dealloc = m_dealloc;
			auto result = detach();
			if ( result )
			{
				size = tmp_size;
				dealloc = tmp_dealloc;
			}
			return result;
		}

		inline void
		reallocate( size_type new_size )
		{
			if ( ! m_realloc )
			{
				throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
			}

			m_data = m_realloc( m_data, m_size,  new_size ) ;
			if ( m_data )
			{
				m_size = new_size;
			}
			else
			{
				m_size = 0;
				throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
			}
		}
	
		inline void
		reallocate( size_type new_size, std::error_code& ec )
		{
			clear_error( ec );
			if ( ! m_realloc )
			{
				ec =  make_error_code( std::errc::no_buffer_space );
				goto exit;
			}

			m_data = m_realloc( m_data, m_size,  new_size );

			if ( ! m_data )
			{
				m_size = 0;
				ec = make_error_code( std::errc::no_buffer_space );
				goto exit;
			}
			else
			{
				m_size = new_size;
			}

		exit:
			return;
		}
	
		inline elem_type*
		data()
		{
			return m_data;
		}
	
		inline const elem_type*
		data() const
		{
			return m_data;
		}
	
		inline size_type
		size() const
		{
			return m_size;
		}

		inline void
		size( size_type val )
		{
			m_size = val;
		}
	
		inline std::size_t
		refs() const
		{
			return m_refs;
		}
	
		inline void
		refs( std::size_t val )
		{
			m_refs = val;
		}

		inline std::size_t
		increment_refs()
		{
			return ++m_refs;
		}

		inline std::size_t
		decrement_refs()
		{
			return --m_refs;
		}

		inline bool
		is_policy( policy value ) const
		{
			return m_policy == value;
		}

		inline void
		set_policy( policy value )
		{
			m_policy = value;
		}

		inline bool
		is_exclusive() const
		{
			return is_policy( policy::exclusive );
		}

		inline void
		set_exclusive()
		{
			set_policy( policy::exclusive );
		}

		inline bool
		is_copy_on_write() const
		{
			return is_policy( policy::copy_on_write );
		}

		inline void
		set_copy_on_write()
		{
			set_policy( policy::copy_on_write );
		}

		inline bool
		is_no_copy_on_write() const
		{
			return is_policy( policy::no_copy_on_write );
		}

		inline void
		set_no_copy_on_write()
		{
			set_policy( policy::no_copy_on_write );
		}


		elem_type* 			m_data;
		size_type			m_size;
		std::size_t 		m_refs;
		dealloc_function	m_dealloc;
		realloc_function	m_realloc;
		policy				m_policy;
	};

	struct do_not_allocate_shared {};

	inline
	buffer( do_not_allocate_shared )
	:
	m_shared{ nullptr },
	m_data{ nullptr },
	m_size{ 0  }
	{}

public:

	friend class string_alias;

	static const size_type npos = std::numeric_limits< size_type >::max();

	void debug_state() const
	{
		std::cout << "buffer: " << std::endl;
		print_state();
		dump( std::cout );
		std::cout.flush();
	}
	
	inline
	buffer()
	:
		m_shared{ new _buffer_shared{} },
		m_data{ m_shared->data() },
		m_size{ m_shared->size() }
	{}

	template< class T, class = typename std::enable_if_t< std::is_integral< T >::value, void > >
	inline
	buffer( T size, policy pol = policy::copy_on_write )
	:
		m_shared{ new _buffer_shared{ static_cast< size_type >( size ), pol } },
		m_data{ m_shared->data() },
		m_size{ m_shared->size() }
	{}

	inline
	buffer( const char *data, policy pol = policy::copy_on_write )
	:
		buffer{ data, data ? strlen( data ) : 0, pol }
	{}

	inline
	buffer( const std::string &data, policy pol = policy::copy_on_write )
	:
		buffer{ data.c_str(), data.size(), pol }
	{}

	inline
	buffer( const void *data, size_type size, policy pol = policy::copy_on_write )
	:
		m_shared{ new _buffer_shared{ data, size, pol } },
		m_data{ m_shared->data() },
		m_size{ m_shared->size() }
	{}

	inline 
	buffer( void *data, size_type size, policy pol, dealloc_function dealloc, realloc_function realloc )
	:
	m_shared{ new _buffer_shared{ data, size, pol, dealloc, realloc } },
	m_data{ m_shared->data() },
	m_size{ m_shared->size() }
	{}

	inline
	buffer( const buffer &rhs )
	:
	m_shared{ share( rhs ) },
	m_data{ m_shared->data() },
	m_size{ m_shared->size() }
	{}

	/*
	 *	The move ctor avoids ref-count arithmetic and 
	 *	potentially hastens freeing the memory because
	 *	one fewer reference is extant.
	 */
	inline
	buffer( buffer&& rhs )
	:
	m_shared{ nullptr },
	m_data{ nullptr },
	m_size{ 0 }
	{
		swap( rhs );
	}
/*
	inline
	buffer( std::vector< elem_type > const& vec, policy pol = policy::copy_on_write )
	:
	m_shared{ new _buffer_shared{ vec, pol } },
	m_data{ m_shared->data() },
	m_size{ m_shared->size() }
	{}
*/
	inline ~buffer()
	{
		unshare();
	}

	inline bool
	is_writable()
	{
		assert( m_shared != nullptr );
		return is_unique() || is_no_copy_on_write();
	}

	inline buffer&
	make_writable()
	{
		assert( m_shared != nullptr );
		if ( ! is_writable() )
		{
			clone();
		}
		return *this;
	}
			
 	inline buffer&
	clone()
	{
		assert( m_shared != nullptr );
		unshare( new _buffer_shared{ m_data, m_size } );
		return *this;
	}

	inline bool
	is_unique() const
	{
		assert( m_shared != nullptr );
		return  m_shared->refs() == 1;
	}

	inline buffer&
	make_unique()
	{
		assert( m_shared != nullptr );
		if ( ! is_unique() )
		{
			clone();
		}
		return *this;
	}

	inline bool
	is_exclusive() const
	{
		assert( m_shared != nullptr );
		return m_shared->is_exclusive();
	}

	inline buffer&
	make_exclusive()
	{
		assert( m_shared != nullptr );
		if ( ! is_exclusive() )
		{
			make_unique();
			m_shared->set_exclusive();
		}
		return *this;
	}

	inline bool
	is_copy_on_write() const
	{
		assert( m_shared != nullptr );
		return m_shared->is_copy_on_write();
	}
	
	inline buffer&
	make_copy_on_write()
	{
		assert( m_shared != nullptr );
		if ( ! is_copy_on_write() )
		{
			make_unique();
			m_shared->set_copy_on_write();
		}
		return *this;
	}

	inline bool
	is_no_copy_on_write() const
	{
		assert( m_shared != nullptr );
		return m_shared->is_no_copy_on_write();
	}
	
	inline buffer&
	make_no_copy_on_write()
	{
		assert( m_shared != nullptr );
		if ( ! is_no_copy_on_write() )
		{
			make_unique();
			m_shared->set_no_copy_on_write();
		}
		return *this;
	}

	inline buffer&
	reset_bounds()
	{
		assert( m_shared != nullptr );
		if ( is_writable() )
		{
			m_data = m_shared->data();
			m_size = m_shared->size();
		}
		return *this;
	}
				

	inline buffer
	slice( size_type offset, size_type len, bool force_copy = false ) const
	{

		assert( invariants() );

		buffer result( do_not_allocate_shared{} );

		size_type slice_size = 0;
		if ( offset < m_size )
		{
			slice_size = std::min( len, m_size - offset );
			if ( slice_size > 0 )
			{
				if ( m_shared->is_exclusive() || force_copy )
				{
					_buffer_shared *tmp = new _buffer_shared{ m_data + offset, slice_size };
					result.m_shared = tmp;
					result.m_data = tmp->data();
					result.m_size = tmp->size();
				}
				else
				{
					result.m_shared = share(*this);
					result.m_data = m_data + offset;
					result.m_size = slice_size;
				}
			}
		}

		assert( invariants() );
		if ( result.m_shared == nullptr )
		{
			result.m_shared = new _buffer_shared{};
		}
		assert( result.invariants() );
		return result;
	}


	buffer&
	operator=( const buffer &rhs )
	{
		if ( this != &rhs )
		{
			unshare( share( rhs ) );
		}
		assert( invariants() );
		return *this;
	}

	buffer&
	operator=( buffer&& rhs )
	{
		if ( this != &rhs )
		{
			unshare();
			swap( rhs );
		}
		assert( invariants() );
		return *this;
	}


	inline bool
	operator==( const buffer &rhs ) const
	{
		return 
		( 
			( this == &rhs ) || 
			( m_size == 0 && rhs.m_size == 0 ) || 
			( 
				( m_size == rhs.m_size ) && 
				( std::memcmp( m_data, rhs.m_data, m_size ) == 0  ) 
			) 
		) ? true : false;
	}
	
	inline bool
	operator!=( const buffer &rhs ) const
	{
		return !( *this == rhs );
	}

	inline void
	swap( buffer &rhs )
	{
		_buffer_shared *tmp_shared = m_shared;
		elem_type *tmp_data = m_data;
		size_type tmp_size = m_size;
		m_shared = rhs.m_shared;
		m_data = rhs.m_data;
		m_size = rhs.m_size;
		rhs.m_shared = tmp_shared;
		rhs.m_data = tmp_data;
		rhs.m_size = tmp_size;
	}

	inline buffer&
	assign( const char *data )
	{
		assign( data, data ? strlen( data ) : 0 );
		return *this;
	}

	inline buffer&
	assign( const char *data, std::error_code& ec )
	{
		assign( data, data ? strlen( data ) : 0, ec );
		return *this;
	}

	inline buffer&
	assign( const std::string &data )
	{
		assign( data.c_str(), data.size() );
		return *this;
	}

	inline buffer&
	assign( const std::string &data, std::error_code& ec )
	{
		assign( data.c_str(), data.size(), ec );
		return *this;
	}

	inline buffer&
	assign( const void *data, size_type length )
	{
		std::error_code ec;
		assign( data, length, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	inline buffer&
	assign( const void *data, size_type length, std::error_code& ec )
	{
		clear_error( ec );

		if ( data && length > 0 )
		{
			if ( is_writable() ) 
			{
				if ( length <= m_shared->size() )
				{
					std::memcpy( m_shared->data(), data, length );
					m_data = m_shared->data();
					m_size = length;
				}
				else
				{
					m_size = 0; // don't preserve any buffer contents
					m_shared->reallocate( length, ec );
					if ( ec ) goto exit;
					std::memcpy( m_data, data, m_size );
				}
			}
			else
			{
				unshare( new _buffer_shared{data, length} );
			}
		}
		else
		{
			m_size = 0;
		}

	exit:
		return *this;
	}

	inline buffer&
	assign( const void* data, size_type pos, size_type length )
	{
		if ( pos > m_size )
		{
			pos = m_size;
		}
		if ( pos < m_size )
		{
			m_size = pos;
		}

		append( data, length );
		assert( invariants() );
		return *this;
	}

	inline buffer&
	assign( buffer const& buf, size_type pos )
	{
		assign( buf.m_data, pos, buf.m_size );
		return *this;
	}

	inline buffer&
	clear()
	{
		unshare( new _buffer_shared{} );
		return *this;
	}

	inline buffer&
	append( const buffer& rhs )
	{
		append( rhs.m_data, rhs.m_size );
		return *this;
	}

	inline buffer&
	append( const buffer& rhs, std::error_code& ec )
	{
		append( rhs.m_data, rhs.m_size, ec );
		return *this;
	}

	inline buffer&
	append( const void* src, size_type nbytes )
	{
		std::error_code ec;
		append( src, nbytes, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	inline buffer&
	append( const void* src, size_type nbytes, std::error_code& ec )
	{
		clear_error( ec );

		assert(invariants());

		if ( src != nullptr && nbytes > 0 )
		{
			if ( is_writable() )
			{
				// if necessary, normalize the buffer
				if ( m_data > m_shared->data() )
				{
					std::memmove( m_shared->data(), m_data, m_size );
					m_data = m_shared->data();
				}
				auto required_size = m_size + nbytes;
				if ( required_size > m_shared->size() )
				{
					auto append_offset = m_size;
					m_shared->reallocate( required_size, ec );
					if ( ec ) goto exit;
					std::memmove( m_shared->data() + append_offset, src, nbytes );
				}
				else // enough space to append exists in allocated block
				{
					std::memmove( m_shared->data() + m_size, src, nbytes );
					m_size += nbytes;
				}
			}
			else
			{
				_buffer_shared *tmp = new _buffer_shared{ m_size + nbytes };
				std::memmove( tmp->data(), m_data, m_size );
				std::memmove( tmp->data() + m_size, src, nbytes );
				unshare( tmp );
			}
		}
		assert(invariants());

	exit:
		return *this;
	}

	inline buffer&
	fill( elem_type value = 0 )
	{
		std::error_code ec;
		fill( value, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	inline buffer&
	fill( elem_type value, std::error_code& ec )
	{
		clear_error( ec );

		assert(invariants());

		if ( m_size > 0 ) // implies pointers are non-null
		{
			if ( ! is_writable() )
			{
				// don't make_unique() -- it copies the buffer, which is just going to be overwritten by the fill
				unshare( new _buffer_shared{ m_size } );
			}
			if ( m_shared->data() == nullptr )
			{
				auto alloc_size = m_size;
				m_size = 0;
				m_shared->reallocate( alloc_size, ec );
				if ( ec ) goto exit;
			}
			std::memset( m_data, value, m_size );
		}

	exit:
		return *this;
	}

	inline size_type
	size() const
	{
		return m_size;
	}

	inline size_type
	capacity() const
	{
		return m_size;
	}

	inline buffer&
	size( size_type nbytes )
	{
		std::error_code ec;
		size( nbytes, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	inline buffer&
	size( size_type nbytes, std::error_code& ec )
	{
		clear_error( ec );

		assert(invariants());

		if ( nbytes <= m_size )
		{
			m_size = nbytes;
		}
		else
		{
			if ( is_writable() )
			{
				// normalize if necessary
				if ( m_data > m_shared->data() )
				{
					std::memmove( m_shared->data(), m_data, std::min( nbytes, m_size ) );
					m_data = m_shared->data();
				}
				if ( nbytes > m_shared->size() )
				{
					m_shared->reallocate( nbytes, ec );
					if ( ec ) goto exit;
					m_data = m_shared->data();
				}
				m_size = nbytes;
			}
			else
			{
				size_type move_size = std::min( m_size, nbytes );
				_buffer_shared *tmp = new _buffer_shared{ nbytes };
				std::memcpy( tmp->data(), m_data, move_size );
				unshare( tmp );
			}
		}
		assert( invariants() );

	exit:
		return *this;
	}

	inline buffer&
	capacity( size_type nbytes )
	{
		size( nbytes );
		return *this;
	}

	inline buffer&
	capacity( size_type nbytes, std::error_code& ec )
	{
		clear_error( ec );
		size( nbytes, ec );
		return *this;
	}

	inline bool
	empty() const
	{
		return ( m_size == 0 );
	}


	inline elem_type*
	mutable_data()
	{
		if ( ! is_writable() )
		{
			make_unique();
		}
		return m_data;
	}

	inline elem_type*
	rdata()
	{
		return m_data;
	}

	inline elem_type*
	exclusive_data()
	{
		make_exclusive();
		return m_data;
	}
	
	inline const elem_type*
	const_data() const
	{
		return m_data;
	}

	inline elem_type*
	detach()
	{
		elem_type* result = nullptr;
		result = m_shared->detach(); // null if not unique
		if ( result ) 
		{
			m_data = m_shared->data();
			m_size = m_shared->size();
		}
		return result;
	}

	inline elem_type*
	detach( size_type& size, dealloc_function& dealloc )
	{
		elem_type* result = nullptr;
		result = m_shared->detach( size, dealloc );
		if ( result )
		{
			m_data = m_shared->data();
			m_size = m_shared->size();
		}
		return result;
	}

	inline elem_type
	operator[]( size_type index ) const
	{
		return at( index );
	}

	inline elem_type
	at( size_type index ) const
	{
		if ( m_data == nullptr )
		{
			throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
		}

		if ( index >= m_size )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}

		return *(m_data + index);
	}
	
	inline elem_type
	at( size_type index, std::error_code& ec )
	{
		elem_type result = 0;

		clear_error( ec );
		if ( m_data == nullptr )
		{
			ec = make_error_code( std::errc::no_buffer_space );
			goto exit;
		}

		if ( index >= m_size )
		{
			ec = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		result = *(m_data + index);

	exit:
		return result;
	}


	inline buffer&
	rput( position_type pos, elem_type value )
	{
		*( m_data + pos  ) = value;
		return *this;
	}

	inline buffer&
	rputn( position_type pos, const void *data, size_type length )
	{
		std::memmove( m_data + pos, data, length );
		return *this;
	}

	inline buffer&
	put( size_type index, elem_type value )
	{
		if ( m_data == nullptr )
		{
			throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
		}

		if ( index >= m_size )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}

		if ( ! is_writable() )
		{
			make_unique();
		}

		*( m_data + index ) = value;
		return *this;
	}

	inline buffer&
	put( size_type index, elem_type value, std::error_code& ec )
	{
		clear_error( ec );

		if ( m_data == nullptr )
		{
			ec = make_error_code( std::errc::no_buffer_space );
			goto exit;
		}

		if ( index >= m_size )
		{
			ec = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		if ( ! is_writable() )
		{
			make_unique();
		}

		*( m_data + index ) = value;

	exit:
		return *this;
	}

	inline buffer&
	put( size_type index, const void* data, size_type length )
	{
		if ( m_data == nullptr )
		{
			throw std::system_error{ make_error_code( std::errc::no_buffer_space ) };
		}

		if ( index + length >= m_size )
		{
			throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
		}

		if ( ! is_writable() )
		{
			make_unique();
		}

		::memcpy( m_data + index, data, length );
		return *this;
	}

	inline buffer&
	put( size_type index, const void* data, size_type length, std::error_code& ec )
	{
		clear_error( ec );

		if ( m_data == nullptr )
		{
			ec = make_error_code( std::errc::no_buffer_space );
			goto exit;
		}

		if ( index + length >= m_size )
		{
			ec = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		if ( ! is_writable() )
		{
			make_unique();
		}

		::memcpy( m_data + index, data, length );
	
	exit:
		return *this;
	}

	// TODO: fix/make alternative signature with error_code& argument
	// TODO: confused by name; not sure this is doing whatever it is supposed to be doing
	inline std::error_code
	rotate( size_type to, size_type from, size_type end ) 
	{
		auto err = std::error_code();

		if ( m_size == 0 )
		{
			err = make_error_code( std::errc::no_buffer_space );
		}
		else
		{
			if ( ! ( ( to < m_size ) && ( from < m_size ) && ( end <= m_size ) ) )
			{
				err = make_error_code( std::errc::invalid_argument );
			}
			else
			{
				if ( ! is_writable() )
				{
					make_unique();
				}
				std::memmove( m_data + to, m_data + from, end - from );
			}
		}

		return err;
	}

	inline size_type
	find( elem_type c, size_type pos = 0 ) const
	{
		if ( m_size == 0 || pos >= m_size )
		{
			return npos;
		}

		const elem_type* p (static_cast<const elem_type*>( std::memchr( m_data + pos, c, m_size - pos ) ) );
		return p != 0 ? static_cast<size_type>( p - m_data ) : npos;
	}
	
	inline size_type
	find( const char *s, size_type len, size_type pos = 0 ) const
	{
		size_type ret = npos;
		
		if ( ( pos + len ) < m_size )
		{
			auto p		= std::search( m_data + pos, m_data + m_size - pos, s, s + len );
			auto index	= static_cast< size_type >( p - m_data );
			
			if ( index < m_size )
			{
				ret = index;
			}
		}
	
		return ret;
	}

	inline size_type
	rfind( elem_type c, size_type pos = npos ) const
	{
		if ( m_size != 0 )
		{
			size_type n = m_size;

			if ( --n > pos )
			{
				n = pos;
			}

			for ( ++n; n-- != 0; )
			{
				if ( m_data[ n ] == c )
				{
					return n;
  				}
			}
		}

		return npos;
	}
	
	checksum_type
	checksum() const
	{
		return checksum( 0, size() );
	}

	checksum_type
	checksum( size_type length ) const
	{
		return checksum( 0, length );
	}

	checksum_type
	checksum( size_type offset, size_type length ) const;

	inline std::string
	to_string() const
	{
		std::string result;
		if ( m_size > 0 )
		{
			result =  std::string( reinterpret_cast< const char* >( m_data ), m_size );
		}
		return result;
	}
	
	void
	dump( std::ostream& os ) const;

private:

	static inline void
	clear_error( std::error_code& ec )
	{
		static const std::error_code no_error;
		ec = no_error;
	}

	void
	print_state() const;

	inline bool
	invariants() const
	{
		bool result = true;
		if ( m_shared == nullptr )
		{
			result = false;
		}
		else // m_shared != nullptr
		{

			if ( m_shared->data() != nullptr )
			{
				if ( m_data == nullptr ||  m_data < m_shared->data() || ( ( m_data + m_size ) > ( m_shared->data() + m_shared->size() ) ) )
					result = false;
			}
			else
			{
				if ( m_data != nullptr  || m_size > 0 )
					result = false;
			}

			if ( m_shared->is_exclusive() && m_shared->refs() > 1 ) result = false;

		}

		if ( ! result )
		{
			print_state();
		}

		return result;
	}

	inline _buffer_shared*
	share( buffer const& rhs ) const
	{
		_buffer_shared* result = nullptr;

		if ( rhs.is_exclusive() )
		{
			result = new _buffer_shared{ *rhs.m_shared };
		}
		else
		{
			result = rhs.m_shared;
			result->increment_refs();
		}
		return result;
	}

	inline void
	unshare( _buffer_shared* replacement )
	{
		assert( replacement != nullptr );
		assert( m_shared != nullptr );
		assert( m_shared->refs() > 0 );

		if ( m_shared->decrement_refs() == 0 )
		{
			delete m_shared;
		}
		m_shared = replacement;
		m_data = m_shared->data();
		m_size = m_shared->size();
	}

	inline void
	unshare()
	{
		if ( m_shared )
		{
			assert( m_shared->refs() > 0 );

			if ( m_shared->decrement_refs() == 0 )
			{
				delete m_shared;
			}
			m_shared = nullptr;
			m_data = nullptr;
			m_size = 0;
		}
	}

	_buffer_shared*				m_shared;
	elem_type*					m_data;
	size_type					m_size;
};

class string_alias
{
public:

	string_alias()
	:
	m_buf{}
	{}

	string_alias( buffer const& buf )
	: 
	m_buf{ buf }
	{}
		
	string_alias( buffer const& buf, std::size_t offset, std::size_t size )
	:
	m_buf{ buf.slice( offset, size ) }
	{}

	string_alias( string_alias const& rhs )
	:
	m_buf{ rhs.m_buf }
	{}

	string_alias( string_alias&& rhs )
	:
	m_buf{ std::move( rhs.m_buf ) }
	{}

	string_alias&
	operator=( string_alias const& rhs )
	{
		if ( this != &rhs )
		{
			m_buf = rhs.m_buf;
		}
		return *this;
	}	

	string_alias&
	operator=( string_alias&& rhs )
	{
		if ( this != & rhs )
		{
			m_buf = std::move( rhs.m_buf );
		}
		return *this;
	}


	inline void
	clear()
	{
		m_buf.clear();
	}
	
	inline
	operator std::string_view () const noexcept
	{
		return std::string_view{ reinterpret_cast< const char* >( m_buf.m_data ), m_buf.m_size };
	}

	inline std::string_view
	view() const noexcept
	{
		return std::string_view{ reinterpret_cast< const char* >( m_buf.m_data ), m_buf.m_size };
	}

private:

	buffer	m_buf;
};

// Ghetto streambuf to provide support for msgpack::packer and unpacker

class bufwriter
{
public:

	inline
	bufwriter( std::size_t size )
	:
	m_buf{ size },
	m_pos{ 0 }
	{}

	inline void
	reset()
	{
		m_pos = 0;
	}

	inline std::size_t
	position() const
	{
		return m_pos;
	}

	inline void 
	putn( const void* src, std::size_t n )
	{
		accommodate( n );
		m_buf.rputn( m_pos, src, n );
		m_pos += n;
	}

	inline void 
	put( std::uint8_t b )
	{
		accommodate( 1 );
		m_buf.rput( m_pos, b );
		++m_pos;
	}

	inline std::uint8_t* 
	accommodate( std::size_t n )
	{
		auto remaining = m_buf.size() - m_pos;
		if ( n > remaining )
		{
			auto required = m_pos + n;
			auto cushioned_size = ( 3 * required ) / 2;
			m_buf.size( cushioned_size );
		}
		return m_buf.mutable_data() + m_pos;
	}

	inline void 
	advance( std::size_t n )
	{
		m_pos += n;
	}

	inline buffer
	get_buffer()
	{
		m_buf.size( m_pos );
		return m_buf;
	}

	inline void 
	write(const char* src, std::size_t len)
	{
		putn( src, len );
	}

private:

	buffer m_buf;
	std::size_t m_pos;
};

}




#endif
