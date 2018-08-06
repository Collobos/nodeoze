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

#ifndef NODEOZE_BUFFER_H
#define NODEOZE_BUFFER_H

 #include <functional>
 #include <system_error>
#include <vector>

namespace nodeoze {

#ifndef DOCTEST_CONFIG_DISABLE
namespace detail
{
	class buffer_test_probe;
}
#endif

/**	A managed block of memory
 * 
 * 	The buffer class encapsulates a contiguous region of memory, providing configurable
 *  policies that control allocation and sharing of buffer space, as well as a number of
 * 	utility operations that are roughly analogous to C library functions such as memcpy, 
 * 	memset, etc. Buffers will automatically resize as necessary to accomodate certain operations,
 * 	or in respnse to explicit resizing requests (see size(size_type)).
 * 
 *  ### Sharing policies
 * 
 * 	An instance of the class buffer holds a pointer to an internal *memory object*. This
 * 	object is reference-counted, and may be shared by other instances of buffer. The 
 * 	circumstances under which a memory object may be shared, and the manner of that sharing,
 * 	are controlled by *sharing policies*.
 * 
 * 	There are three sharing policies: *copy_on_write*, *exclusive*, and *no_copy_on_write*.
 * 	By default, buffers are copy_on_write. Policy can be set at construction time, and
 * 	modified subsequently queried (is_copy_on_write(), is_exclusive(), is_no_copy_on_write()),
 * 	or modified (make_copy_on_write(), make_exclusive(), make_no_copy_on_write()). The
 * 	current state of a buffer with regards to sharing can be queried (is_unique()).
 * 	Although copy_on_write policy is enforced transparently when a buffeer is modified, 
 * 	it make be forced (force_unique()).
 * 
 *	#### copy_on_write 
 *	
 *	Buffers are internally reference-counted. The copy constructor and copy assignment operator
 *	copy a pointer to a shared buffer and increment its reference count. If an operation 
 *	that modifies the buffer is performed, and the reference count is greater than one, 
 *	a new buffer is allocated and the contents of the current buffer are copied to the
 *	new buffer before the modification is made.
 *	
 *  #### exclusive
 * 
 * 	If the buffer's policy is exclusive, the buffer is not shared--the reference count
 *  will never be greater than one. The copy constructor and copy assignment operator
 *  always allocate new buffer space and copy the currnent buffer contents into the
 * 	the new space.
 * 
 * 	#### no_copy_on_write
 * 
 * 	If the buffer's policy is no_copy_on_write, the buffer is shared and reference counted,
 *  but the buffer is not copied when modifications are made. A buffer whose policy is set
 * 	to no_copy_on_write must be handled with care. In particular, modifications that 
 * 	cause a buffer to undergo reallocation (e.g., resizing ) may invalidate addresses
 * 	obtained from certain accessors ( see data(), mutable_data(), and data() ).
 * 
 *  ### Custom allocators
 *  
 *  The buffer class performs its own allocation, reallocation and deallocation 
 * 	internally by default. The application may impose different memory management
 * 	policies by supplying custom (re)allocators and deallocators at construction
 *  time (see the constructor buffer( void*, size_type, policy, dealloc_function, realloc_function)
 * 	for details.)
 * 
 * 	### Error handling
 * 	
 * 	Almost all of the member functions that potentially need to report error condiions have
 * 	two forms--one that may throw exceptions, and one that passes std::error_code values 
 *  by side-effecting a reference parameter. In general, the exceptions thrown will be 
 * 	of type std::system_error, containing std::error_code values identical to those
 * 	returned by corresponding form of the same member function.
 */

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

	/** Custom deallocation functor
	 * 
	 * 	A custom deallocator is invoked when the reference count for a 
	 * 	buffer's internal allocation goes to zero. An implementation should
	 * 	deallocate the memory region indicated by the \c data parameter.
	 * 
	 * 	\param data the address of the memory region to be deallocated
	 * 
	 */
	using dealloc_function =  std::function< void ( elem_type *data ) >;

	/** Custom allocation/reallocation functor 
	 * 	
	 * 	A custom (re)allocator is invoked for both initial allocation
	 * 	and subsequent reallocation in the event of resizing. Am implementation
	 *	must provide the following behaviors:
	 *	
	 * 	If the value of the data parameter is \c nullptr, the implementation
	 * 	must return the address of a contiguous region of memory 
	 * 	at least new_size bytes in length; the current_size parameter
	 * 	should be ignored.
	 * 
	 * 	If the value of the data parameter is not \c nullptr, it points the
	 * 	a region of memory (previously allocated by this implementation )
	 * 	that was in use by the instance of buffer with which the allocator
	 * 	was associated at construction. The \c current_size parameter indicates
	 * 	the number of bytes in that region ( starting at the address in \c data)
	 * 	that have been used by the buffer instance. Their contents must be preserved
	 * 	in the buffer returned by the implementation. The implementation must
	 * 	return a region of memory with at least \c new_size bytes long, 
	 *  the allocator, such that the first \c current_size bytes have the same
	 * 	values as the corresponding bytes in the original buffer (at \c data).
	 * 	An implementation may return the address provided in the data parameter,
	 * 	provided that the block of memory at that address is at least 
	 * 	new_size bytes in length, and the contents of the first current_size bytes
	 * 	are unchanged. If a new region of memory is allocated, the previous memory
	 * 	should be deallocated.
	 * 
	 * 	\param data the address of the existing memory region; \c nullptr on initial allocation
	 *	\param current_size	the number of bytes in the existing buffer (at \c data) that must be preserved
	 *	\param new_size	the requested size of the new memory region
	 *	\return the address of a region of memory at least new_size bytes long, with contents from the previous memory (if any) preserved
	 */
	using realloc_function =  std::function< elem_type* ( elem_type *data, size_type current_size, size_type new_size ) >;
	static dealloc_function default_dealloc;
	static realloc_function default_realloc;

	/** Denote the sharing policy applied to an instance 
	 * 
	 * 
	 */
	enum class policy
	{
		copy_on_write, /*!< buffer is shared, copied on modification */
		no_copy_on_write, /*!< buffer is shared unconditionally */
		exclusive /*!< buffer is never shared */
	};

private:

#ifndef DOCTEST_CONFIG_DISABLE
	bool invariants() const;
#else
	constexpr bool invariants() { return true; }
#endif

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

		_buffer_shared( void* data, size_type size, policy pol, dealloc_function dealloc, realloc_function realloc )
		:
		m_data{ nullptr },
		m_size{ size },
		m_refs{ 1 },
		m_dealloc{ dealloc },
		m_realloc{ realloc },
		m_policy{ pol }
		{
			if ( m_size > 0 )
			{
				if ( data )
				{
					if ( realloc )
					{
						if ( ! dealloc )
						{
							throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
						}
						m_data = m_realloc( nullptr, 0, m_size );
						::memcpy( m_data, data, m_size );
					}
					else
					{
						m_data = reinterpret_cast< elem_type* >( data );
					}
				}
				else
				{
					if ( realloc )
					{
						if ( ! dealloc )
						{
							throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
						}
						m_data = m_realloc( nullptr, 0, m_size );
					}
				}
			}
			else
			{
				if ( ! realloc || ! dealloc )
				{
					throw std::system_error{ make_error_code( std::errc::invalid_argument ) };
				}
			}
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

		elem_type*
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

		elem_type*
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

		void
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
	
		void
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
	
		elem_type*
		data()
		{
			return m_data;
		}
	
		const elem_type*
		data() const
		{
			return m_data;
		}
	
		size_type
		size() const
		{
			return m_size;
		}

		void
		size( size_type val )
		{
			m_size = val;
		}
	
		std::size_t
		refs() const
		{
			return m_refs;
		}
	
		void
		refs( std::size_t val )
		{
			m_refs = val;
		}

		std::size_t
		increment_refs()
		{
			return ++m_refs;
		}

		std::size_t
		decrement_refs()
		{
			return --m_refs;
		}

		bool
		is_policy( policy value ) const
		{
			return m_policy == value;
		}

		void
		set_policy( policy value )
		{
			m_policy = value;
		}

		bool
		is_exclusive() const
		{
			return is_policy( policy::exclusive );
		}

		void
		set_exclusive()
		{
			set_policy( policy::exclusive );
		}

		bool
		is_copy_on_write() const
		{
			return is_policy( policy::copy_on_write );
		}

		void
		set_copy_on_write()
		{
			set_policy( policy::copy_on_write );
		}

		bool
		is_no_copy_on_write() const
		{
			return is_policy( policy::no_copy_on_write );
		}

		void
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

	buffer( do_not_allocate_shared )
	:
	m_shared{ nullptr },
	m_data{ nullptr },
	m_size{ 0  }
	{}

public:

	friend class string_alias;

	static const size_type npos = std::numeric_limits< size_type >::max();

	/** Default constructor
	 * 
	 * 	The constructed instance has no allocation, size is zero.
	 */
	buffer()
	:
		m_shared{ new _buffer_shared{} },
		m_data{ m_shared->data() },
		m_size{ m_shared->size() }
	{}

	/** Construct with allocation of specified size and policy
	 * 
	 *  The constructed instance has at least size bytes allocated. The
	 * 	allocated memory may be uninitialized.
	 * 
	 * 	\param size number of bytes to allocate
	 * 	\param pol sharing policy of the constructed instance, defaults to copy_on_write
	 */
	template< class T, class = typename std::enable_if_t< std::is_integral< T >::value, void > >
	buffer( T size, policy pol = policy::copy_on_write )
	:
		m_shared{ new _buffer_shared{ static_cast< size_type >( size ), pol } },
		m_data{ m_shared->data() },
		m_size{ m_shared->size() }
	{}

	/** Construct from C-style string
	 * 
	 * 	Construct a buffer instance, copying the contents from a C-style string. The
	 *  size of the resulting instance is strlen( data ); it does not include the 
	 * 	terminating null character.
	 * 
	 * 	\param data a null-terminiated character string
	 * 	\param pol sharing policy of the constructed instance, defaults to copy_on_write
	 */
	buffer( const char *data, policy pol = policy::copy_on_write )
	:
		buffer{ data, data ? strlen( data ) : 0, pol }
	{}

	/** Construct from std::string
	 * 
	 * 	Construct a buffer instance, copying the contents from the 
	 * 	string parameter.
	 * 
	 * 	\param data string whose contents are copied to the constucted instance
	 */
	buffer( const std::string &data, policy pol = policy::copy_on_write )
	:
		buffer{ data.c_str(), data.size(), pol }
	{}

	/** Construct from memory
	 * 
	 * 	Construct a buffer instance, copying the specified number of bytes
	 * 	from the specified location.
	 * 
	 * 	\param data the address of a region of memory to be copied into the constructed buffer
	 * 	\parap size the number of bytes to copy
	 * 	\param pol sharing policy of the constructed instance, defaults to copy_on_write
	 */
	buffer( const void *data, size_type size, policy pol = policy::copy_on_write )
	:
		m_shared{ new _buffer_shared{ data, size, pol } },
		m_data{ m_shared->data() },
		m_size{ m_shared->size() }
	{}

	/** Constructor of limitless power and great peril
	 * 
	 * 	"This is a highly sophistimacated doo-whacky. If you don't use it responsibly,
	 * 	KA-BLAMMO!" -- Homer Simpson
	 * 	
	 * 	This constructor allows the caller to customize memory allocation and deallocation,
	 * 	by providing functors that implement those operations (see \link dealloc_function \endlink and \link realloc_function \endlink).
	 * 	
	 * 	With the exception of the pol parameter, all of the parameters may take null or zero
	 *	value arguments at run time. Some combinations of parameter values are invalid, and will 
	 *	cause an exception to be thrown. The value of \c pol is orthogonal to the behaviors
	 *	associated with other parameter combinations.
	 *
	 * 	data	|	size	|	dealloc	|	realloc	|	result
	 * ---------|-----------|-----------|-----------|-----------
	 *  nullptr	|	0		|	nullptr	|	?		|	invalid
	 * 	P		|	0		|	?		|	?		|	invalid
	 * 	nullptr	|	0		|	D		|	R		|	1
	 * 	P		|	N		|	nullptr	|	nullptr	|	2
	 * 	P		|	N		|	nullptr	|	R		|	3
	 * 	P		|	N		|	D		|	R		|	4
	 *
	 * 	P = valid, non-null memory address
	 * 
	 * 	N = integer value > 0
	 * 
	 * 	D = dealloc functor
	 * 
	 * 	R = realloc functor
	 * 
	 * 	? = any value
	 * 
	 * 	1. No initial allocation is made, buffer size is zero. Subsequent operations
	 * 	that assign to the buffer or cause its size to be non-zero will cause the realloc
	 * 	functor to be invoked. The dealloc functor will be invoked when the buffer is destroyed.
	 * 
	 * 	2. The address passed to the data parameter is used directly by the buffer, the size being
	 * 	fixed by the \c size parameter value. Any operations that would reqiure re-allocation will
	 * 	result in an exception being thrown. No deallocator will be called when the buffer is destroyed.
	 * 	The calling context is responsible for ensuring the validity of the address passed in \c data 
	 * 	over the lifetime of the buffer being used. It should be noted that, if the constructed buffer's
	 * 	sharing policy is \c copy_on_write, it is possible for other instances of buffer to share this 
	 * 	memory. To ensure that there are no outstanding references after the constructed instance is destroyed,
	 * 	the policy should be set to \c exclusive.
	 * 
	 * 	3. Identical to case 2, except that the provided dealloc functor will be called when the 
	 * 	reference count associated with the memory object goes to zero.
	 * 
	 * 	4. On construction, the realloc functor will be invoked to allocate a memory region of
	 * 	\c size bytes in length. The contents of memory at the address \c data ( of length size )
	 * 	are copied to the allocated memory. The realloc and dealloc functors are used as necessary for
	 * 	subsequent operations that require reallocation or deallocation.
	 * 
	 */ 
	buffer( void *data, size_type size, policy pol, dealloc_function dealloc, realloc_function realloc )
	:
	m_shared{ new _buffer_shared{ data, size, pol, dealloc, realloc } },
	m_data{ m_shared->data() },
	m_size{ m_shared->size() }
	{}

	/** Copy constructor
	 * 
	 * 
	 */
	buffer( const buffer &rhs )
	:
	m_shared{ nullptr },
	m_data{ nullptr },
	m_size{ 0 }
	{
		adopt( rhs );
	}

	/** Move constructor
	 * 
	 *	The move ctor avoids ref-count arithmetic and 
	 *	potentially hastens freeing the memory because
	 *	one fewer reference is extant.
	 */
	buffer( buffer&& rhs )
	:
	m_shared{ nullptr },
	m_data{ nullptr },
	m_size{ 0 }
	{
		swap( rhs );
	}

	/** Copy constructor with policy
	 * 
	 * 
	 */
	buffer( const buffer &rhs, policy pol )
	:
	m_shared{ nullptr },
	m_data{ nullptr },
	m_size{ 0 }
	{
		adopt( rhs );
		set_policy( pol );
	}

	/** Move constructor with policy
	 * 
	 *	The move ctor avoids ref-count arithmetic and 
	 *	potentially hastens freeing the memory because
	 *	one fewer reference is extant.
	 */
	buffer( buffer&& rhs, policy pol )
	:
	m_shared{ nullptr },
	m_data{ nullptr },
	m_size{ 0 }
	{
		swap( rhs );
		set_policy( pol );
	}

	~buffer()
	{
		unshare();
	}

	/** Determine whether this instance can be modified without copying.
	 * 
	 * 
	 * 
	 * 
	 */
	bool
	is_writable()
	{
		assert( m_shared != nullptr );
		return is_unique() || is_no_copy_on_write();
	}

	/** Force this instance to be unique (non-shared).
	 * 
	 */
	buffer&
	make_writable()
	{
		assert( m_shared != nullptr );
		if ( ! is_writable() )
		{
			force_unique();
		}
		return *this;
	}
			
 	buffer&
	clone()
	{
		assert( m_shared != nullptr );
		auto cloned = new _buffer_shared{ m_data, m_size };
		unshare();
		m_shared = cloned;
		m_data = m_shared->data();
		m_size = m_shared->size();
//		unshare( new _buffer_shared{ m_data, m_size } );
		return *this;
	}

	/** Determine whether this instance is current shared.
	 * 
	 * 
	 * 
	 * 
	 */ 
	bool
	is_unique() const
	{
		assert( m_shared != nullptr );
		return  m_shared->refs() == 1;
	}

	/** Force this instance to be unique (non-shared).
	 * 
	 */
	buffer&
	force_unique()
	{
		assert( m_shared != nullptr );
		if ( ! is_unique() )
		{
			clone();
		}
		return *this;
	}
	/** Force this instance to be unique (non-shared);
	 *  Copy only the first nbytes bytes in the buffer when cloning.
	 * 
	 */
	buffer&
	force_unique( size_type nbytes )
	{
		assert( nbytes <= m_size );
		assert( m_shared != nullptr );
		if ( ! is_unique() )
		{
			assert( m_shared != nullptr );
			auto cloned = new _buffer_shared{ m_size };
			::memcpy( cloned->data(), m_shared->data(), nbytes );
			unshare();
			m_shared = cloned;
			m_data = m_shared->data();
			m_size = m_shared->size();
		}
		return *this;
	}

	buffer&
	set_policy( policy pol )
	{
		switch ( pol )
		{
			case policy::copy_on_write:
			{
				make_copy_on_write();
			}
			break;

			case policy::exclusive:
			{
				make_exclusive();
			}
			break;

			case policy::no_copy_on_write:
			{
				make_no_copy_on_write();
			}
			break;
		}
		return *this;
	}

	/** Determine whether this instance's current sharing policy is exclusive
	 * 
	 */
	bool
	is_exclusive() const
	{
		assert( m_shared != nullptr );
		return m_shared->is_exclusive();
	}

	/** Set this instance's sharing policy to exclusive
	 * 
	 */
	buffer&
	make_exclusive()
	{
		assert( m_shared != nullptr );
		if ( ! is_exclusive() )
		{
			force_unique();
			m_shared->set_exclusive();
		}
		return *this;
	}

	/** Determine whether this instance's current sharing policy is copy_on_write
	 * 
	 */
	bool
	is_copy_on_write() const
	{
		assert( m_shared != nullptr );
		return m_shared->is_copy_on_write();
	}
	
	/** Set this instance's sharing policy to copy_on_write
	 * 
	 */
	buffer&
	make_copy_on_write()
	{
		assert( m_shared != nullptr );
		if ( ! is_copy_on_write() )
		{
			force_unique();
			m_shared->set_copy_on_write();
		}
		return *this;
	}

	/** Determine whether this instance's current sharing policy is no_copy_on_write
	 * 
	 */
	bool
	is_no_copy_on_write() const
	{
		assert( m_shared != nullptr );
		return m_shared->is_no_copy_on_write();
	}
	
	/** Set this instance's sharing policy to no_copy_on_write
	 * 
	 */
	buffer&
	make_no_copy_on_write()
	{
		assert( m_shared != nullptr );
		if ( ! is_no_copy_on_write() )
		{
			force_unique();
			m_shared->set_no_copy_on_write();
		}
		return *this;
	}

	buffer&
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
				

	buffer
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
//					_buffer_shared *tmp = new _buffer_shared{ m_data + offset, slice_size };
					result.m_shared = new _buffer_shared{ m_data + offset, slice_size };
					result.m_data = result.m_shared->data();
					result.m_size = result.m_shared->size();
				}
				else
				{
					result.m_shared = m_shared;
					result.m_shared->increment_refs();
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
			unshare();
			adopt( rhs );
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


	bool
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
	
	bool
	operator!=( const buffer &rhs ) const
	{
		return !( *this == rhs );
	}

	void
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

	buffer&
	assign( const char *data )
	{
		assign( data, data ? strlen( data ) : 0 );
		return *this;
	}

	buffer&
	assign( const char *data, std::error_code& ec )
	{
		assign( data, data ? strlen( data ) : 0, ec );
		return *this;
	}

	buffer&
	assign( const std::string &data )
	{
		assign( data.c_str(), data.size() );
		return *this;
	}

	buffer&
	assign( const std::string &data, std::error_code& ec )
	{
		assign( data.c_str(), data.size(), ec );
		return *this;
	}

	buffer&
	assign( const void *data, size_type length )
	{
		std::error_code ec;
		assign( data, length, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	buffer&
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
				unshare();
				m_shared = new _buffer_shared{ data, length };
				m_data = m_shared->data();
				m_size = m_shared->size();
			}
		}
		else
		{
			m_size = 0;
		}

	exit:
		return *this;
	}

	buffer&
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

	buffer&
	assign( buffer const& buf, size_type pos )
	{
		assign( buf.m_data, pos, buf.m_size );
		return *this;
	}

	buffer&
	clear()
	{
		unshare();
		m_shared = new _buffer_shared{};
		return *this;
	}

	buffer&
	append( const buffer& rhs )
	{
		append( rhs.m_data, rhs.m_size );
		return *this;
	}

	buffer&
	append( const buffer& rhs, std::error_code& ec )
	{
		append( rhs.m_data, rhs.m_size, ec );
		return *this;
	}

	buffer&
	append( const void* src, size_type nbytes )
	{
		std::error_code ec;
		append( src, nbytes, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	buffer&
	append( const void* src, size_type nbytes, std::error_code& ec )
	{
		clear_error( ec );

		assert( invariants() );

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
				unshare();
				m_shared = tmp;
				m_data = m_shared->data();
				m_size = m_shared->size();
			}
		}
		assert( invariants() );

	exit:
		return *this;
	}

	buffer&
	fill( elem_type value = 0 )
	{
		std::error_code ec;
		fill( value, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	buffer&
	fill( elem_type value, std::error_code& ec )
	{
		clear_error( ec );

		assert( invariants() );

		if ( m_size > 0 ) // implies pointers are non-null
		{
			if ( ! is_writable() )
			{
				// don't force_unique() -- it copies the buffer, which is just going to be overwritten by the fill
				auto tmp = new _buffer_shared{ m_size };
				unshare();
				m_shared = tmp;
				m_data = m_shared->data();
				m_size = m_shared->size();
				std::memset( m_data, value, m_size );
				// unshare( new _buffer_shared{ m_size } );
			}
			else
			{
				assert( m_shared->data() != nullptr && m_data != nullptr );
				std::memset( m_data, value, m_size );
			}
		}
		return *this;
	}

	/**	size of the instance's memory object
	 * 
	 * 	\return the size of the instance's memory object, in bytes.
	 */
	size_type
	size() const
	{
		return m_size;
	}

	/** \deprecated (see size() )
	 * 
	 */
	size_type
	capacity() const
	{
		return m_size;
	}

	/**	Modify the size of the instance's memory object.
	 * 
	 * 	If the previous value of size() was zero, this member function
	 * 	will cause a memory object of length \c nbytes to be allocated.
	 * 	If the previous size was non-zero, this operation will cause the buffer to be resized, which
	 * 	may cause a new memory object to be allocated. The contents of the buffer prior to this operation
	 * 	are preserved. If the value of \c nbytes is less than the previous size, the buffer's memory object 
	 * 	is effectively truncated; previous contents at offsets greater than \c nbytes will not be preserved.
	 * 
	 * 	If memory object addresses were obtained by calling data(), mutable_data(), or data() prior 
	 * 	to this operation, those adresses may not be valid after this operation in invoked (the memory objects may have
	 * 	been deallocated).
	 * 
	 * 	If the memory object used by this instance is shared with other buffer instances and the policy
	 * 	associated with the memory object is copy_on_write, this operation is treated as a write operation--
	 * 	the refernce count for the shared memory object is decremented, a new memory object is allocated, 
	 * 	and the contents of the shared object are copied to the new memory object, which will be uniquely
	 * 	owned by this instance.
	 * 
	 *	\param nbytes the requested size of this instances memory object, in bytes.
	 *	\return a reference to this instance of buffer
	 */
	buffer&
	size( size_type nbytes )
	{
		std::error_code ec;
		size( nbytes, ec );
		if ( ec ) throw std::system_error{ ec };
		return *this;
	}

	buffer&
	size( size_type nbytes, std::error_code& ec )
	{
		clear_error( ec );

		assert( invariants() );

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
				unshare();
				m_shared = tmp;
				m_data = m_shared->data();
				m_size = m_shared->size();
			}
		}
		assert( invariants() );

	exit:
		return *this;
	}

	buffer&
	capacity( size_type nbytes )
	{
		size( nbytes );
		return *this;
	}

	buffer&
	capacity( size_type nbytes, std::error_code& ec )
	{
		clear_error( ec );
		size( nbytes, ec );
		return *this;
	}

	bool
	empty() const
	{
		return ( m_size == 0 );
	}


	elem_type*
	mutable_data()
	{
		if ( ! is_writable() )
		{
			force_unique();
		}
		return m_data;
	}

	elem_type*
	data()
	{
		return m_data;
	}

	elem_type*
	exclusive_data()
	{
		make_exclusive();
		return m_data;
	}
	
	const elem_type*
	data() const
	{
		return m_data;
	}

	elem_type*
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

	elem_type*
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

	elem_type
	operator[]( size_type index ) const
	{
		return at( index );
	}

	elem_type
	at( size_type index ) const
	{
		return *( m_data + index );
	}
	
	elem_type
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

		result = *( m_data + index );

	exit:
		return result;
	}


	buffer&
	rput( position_type pos, elem_type value )
	{
		*( m_data + pos  ) = value;
		return *this;
	}

	buffer&
	rputn( position_type pos, const void *data, size_type length )
	{
		std::memmove( m_data + pos, data, length );
		return *this;
	}

	/*
	 *	put to a buffer with no allocation (size == 0 )
	 *	or put outside of buffer bounds results in undefined behavior
	 * 
	 */
	buffer&
	put( size_type index, elem_type value )
	{
		if ( ! is_writable() )
		{
			force_unique();
		}

		*( m_data + index ) = value;
		return *this;
	}

	buffer&
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
			force_unique();
		}

		*( m_data + index ) = value;

	exit:
		return *this;
	}

	/*
	 *	put to a buffer with no allocation (size == 0 )
	 *	or put outside of buffer bounds results in undefined behavior
	 * 
	 */
	buffer&
	put( size_type index, const void* data, size_type length )
	{
		if ( ! is_writable() )
		{
			force_unique();
		}

		::memcpy( m_data + index, data, length );
		return *this;
	}

	buffer&
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
			force_unique();
		}

		::memcpy( m_data + index, data, length );
	
	exit:
		return *this;
	}

	// TODO: fix/make alternative signature with error_code& argument
	// TODO: confused by name; not sure this is doing whatever it is supposed to be doing
	std::error_code
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
					force_unique();
				}
				std::memmove( m_data + to, m_data + from, end - from );
			}
		}

		return err;
	}

	size_type
	find( elem_type c, size_type pos = 0 ) const
	{
		if ( m_size == 0 || pos >= m_size )
		{
			return npos;
		}

		const elem_type* p ( static_cast< const elem_type* >( std::memchr( m_data + pos, c, m_size - pos ) ) );
		return p != 0 ? static_cast< size_type >( p - m_data ) : npos;
	}
	
	size_type
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

	size_type
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

	std::string
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

	static void
	clear_error( std::error_code& ec )
	{
		static const std::error_code no_error;
		ec = no_error;
	}

	_buffer_shared*
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

	void
	adopt( buffer const& rhs )
	{
		assert( m_shared == nullptr );
		if ( rhs.is_exclusive() )
		{
			m_shared = new _buffer_shared{ rhs.m_data, rhs.m_size };
			m_data = m_shared->data();
			m_size = m_shared->size();
		}
		else
		{
			m_shared = rhs.m_shared;
			m_shared->increment_refs();
			m_data = rhs.m_data;
			m_size = rhs.m_size;
		}
	}

	void
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


	void
	clear()
	{
		m_buf.clear();
	}
	
	operator std::string_view () const noexcept
	{
		return std::string_view{ reinterpret_cast< const char* >( m_buf.m_data ), m_buf.m_size };
	}

	std::string_view
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

	bufwriter( std::size_t size )
	:
	m_buf{ size },
	m_pos{ 0 }
	{}

	void
	reset()
	{
		m_pos = 0;
	}

	std::size_t
	position() const
	{
		return m_pos;
	}

	void 
	putn( const void* src, std::size_t n )
	{
		accommodate( n );
		m_buf.rputn( m_pos, src, n );
		m_pos += n;
	}

	void 
	put( std::uint8_t b )
	{
		accommodate( 1 );
		m_buf.rput( m_pos, b );
		++m_pos;
	}

	std::uint8_t* 
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

	void 
	advance( std::size_t n )
	{
		m_pos += n;
	}

	buffer
	get_buffer()
	{
		m_buf.size( m_pos );
		return m_buf;
	}

	void 
	write( const char* src, std::size_t len )
	{
		putn( src, len );
	}

private:

	buffer m_buf;
	std::size_t m_pos;
};

} // namespace nodeoze

#endif // NODEOZE_BUFFER_H
