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

#include <nodeoze/buffer.h>
#include <nodeoze/macros.h>
#include <nodeoze/dump.h>
#include <nodeoze/test.h>
#include <iostream>

class nodeoze::detail::buffer_test_probe
{
public:
	buffer_test_probe( buffer const& target )
	: 
	m_target{ target }
	{}
	
	std::size_t refcount() const
	{
		return m_target.m_shared->refs();
	}

	buffer::_buffer_shared* shared()
	{
		return m_target.m_shared;
	}

	std::size_t shared_size() const
	{
		return m_target.m_shared->size();
	}

	std::uint8_t* shared_data()
	{
		return reinterpret_cast< std::uint8_t* >( m_target.m_shared->data() );
	}

	std::uint8_t* data()
	{
		return reinterpret_cast< std::uint8_t* >( m_target.m_data );
	}

	std::size_t size()
	{
		return m_target.m_size;
	}

	buffer::policy policy()
	{
		return m_target.m_shared->m_policy;
	}

    void
    print_state( std::ostream& os, bool dump = false ) const
    {
        os << "m_shared: " << reinterpret_cast< void* >( m_target.m_shared );
        if ( m_target.m_shared )
        {
            os << ", m_shared->data(): " << reinterpret_cast< void* >( m_target.m_shared->data() )
                    << ", m_shared->size(): " << m_target.m_shared->size()
                    << ", m_shared->refs(): " << m_target.m_shared->refs()
                    << std::boolalpha << ", m_shared->is_exclusive(): " << m_target.m_shared->is_exclusive()
                    << ", m_shared->is_copy_on_write(): " << m_target.m_shared->is_copy_on_write()
                    << ", m_shared->is_no_copy_on_write(): " << m_target.m_shared->is_no_copy_on_write();
        }
        os << ", m_data: " << reinterpret_cast< void* >( m_target.m_data )
                << ", m_size: " << m_target.m_size << std::endl;
        if ( dump )
        {
		    m_target.dump( os );
        }
        os.flush();
    }

private:
	buffer const& m_target;
};

using namespace nodeoze;

bool
buffer::invariants() const
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
		detail::buffer_test_probe probe{ *this };
		probe.print_state( std::cerr );
	}

	return result;
}

TEST_CASE( "nodeoze/smoke/buffer/constructors" )
{
	SUBCASE( "null constructor" )
	{
		buffer a;
		detail::buffer_test_probe probe{ a };
		CHECK( probe.shared() != nullptr );
		CHECK( probe.shared_data() == nullptr );
		CHECK( probe.shared_size() == 0 );
		CHECK( probe.data() == nullptr );
		CHECK( probe.size() == 0 );
		CHECK( probe.refcount() == 1 );
		CHECK( probe.policy() == buffer::policy::copy_on_write );
	}

	SUBCASE( "size constructor" )
	{
		buffer b{ 16, buffer::policy::exclusive };
		detail::buffer_test_probe probe{ b};
		CHECK( probe.shared() != nullptr );
		CHECK( probe.shared_data() != nullptr );
		CHECK( probe.shared_size() == 16 );
		CHECK( probe.data() != nullptr );
		CHECK( probe.data() == probe.shared_data() );
		CHECK( probe.size() == 16 );
		CHECK( probe.refcount() == 1 );
		CHECK( probe.policy() == buffer::policy::exclusive );
	}

	SUBCASE( "c-style string constructor" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s.c_str() };
		detail::buffer_test_probe probe{ b};
		CHECK( probe.shared() != nullptr );
		CHECK( probe.shared_data() != nullptr );
		CHECK( probe.shared_size() == s.size() );
		CHECK( probe.data() != nullptr );
		CHECK( probe.data() == probe.shared_data() );
		CHECK( probe.size() == s.size() );
		CHECK( probe.refcount() == 1 );
		CHECK( probe.policy() == buffer::policy::copy_on_write );
		CHECK( ( void* )probe.shared_data() != ( void* )s.data() );
		CHECK( ::memcmp( probe.shared_data(), s.data(), s.size() ) == 0 );
	}

	SUBCASE( "std::string constructor" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s };
		detail::buffer_test_probe probe{ b};
		CHECK( probe.shared() != nullptr );
		CHECK( probe.shared_data() != nullptr );
		CHECK( probe.shared_size() == s.size() );
		CHECK( probe.data() != nullptr );
		CHECK( probe.data() == probe.shared_data() );
		CHECK( probe.size() == s.size() );
		CHECK( probe.refcount() == 1 );
		CHECK( probe.policy() == buffer::policy::copy_on_write );
		CHECK( ( void* )probe.shared_data() != ( void* )s.data() );
		CHECK( ::memcmp( probe.shared_data(), s.data(), s.size() ) == 0 );
	}

	SUBCASE( "( void*, size ) constructor" )
	{
		std::string s{ "abcdefghijklmnop" };
		std::uint8_t* p = new std::uint8_t[ s.size() ];
		::memcpy( p, s.data(), s.size() );
		buffer b{ p, s.size() };
		detail::buffer_test_probe probe{ b};
		CHECK( probe.shared() != nullptr );
		CHECK( probe.shared_data() != nullptr );
		CHECK( probe.shared_size() == s.size() );
		CHECK( probe.data() != nullptr );
		CHECK( probe.data() == probe.shared_data() );
		CHECK( probe.size() == s.size() );
		CHECK( probe.refcount() == 1 );
		CHECK( probe.policy() == buffer::policy::copy_on_write );
		CHECK( ( void* )probe.shared_data() != ( void* )p );
		CHECK( ::memcmp( probe.shared_data(), p, s.size() ) == 0 );
		delete [] p;
	}

	SUBCASE( "( void*, size, policy, dealloc, realloc ) constructor" )
	{
		std::string s{ "abcdefghijklmnop" };
		std::uint8_t* p = new std::uint8_t[ s.size() ];
		::memcpy( p, s.data(), s.size() );
		{
			buffer b{ p, s.size(), buffer::policy::exclusive, 
			[] ( std::uint8_t* ptr )
			{
				delete [] ptr;
			}, nullptr  };

			detail::buffer_test_probe probe{ b};
			CHECK( probe.shared() != nullptr );
			CHECK( probe.shared_data() != nullptr );
			CHECK( probe.shared_size() == s.size() );
			CHECK( probe.data() != nullptr );
			CHECK( probe.data() == probe.shared_data() );
			CHECK( probe.size() == s.size() );
			CHECK( probe.refcount() == 1 );
			CHECK( probe.policy() == buffer::policy::exclusive );
			CHECK( ( void* )probe.shared_data() == ( void* )p );
		}
//		delete [] p; // this will cause memory sanitize to abort
	}

	SUBCASE( "copy constructor" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s };
		detail::buffer_test_probe probe_b{ b};
		CHECK( probe_b.shared() != nullptr );
		CHECK( probe_b.shared_data() != nullptr );
		CHECK( probe_b.shared_size() == s.size() );
		CHECK( probe_b.data() != nullptr );
		CHECK( probe_b.data() == probe_b.shared_data() );
		CHECK( probe_b.size() == s.size() );
		CHECK( probe_b.refcount() == 1 );
		CHECK( probe_b.policy() == buffer::policy::copy_on_write );
		CHECK( ( void* )probe_b.shared_data() != ( void* )s.data() );
		CHECK( ::memcmp( probe_b.shared_data(), s.data(), s.size() ) == 0 );

		buffer c{ b };
		detail::buffer_test_probe probe_c{ c};
		CHECK( probe_c.shared() != nullptr );
		CHECK( probe_b.shared() == probe_c.shared() );
		CHECK( probe_b.shared_data() == probe_c.shared_data() );
		CHECK( probe_b.shared_size() == probe_c.shared_size() );
		CHECK( probe_b.data() == probe_c.data() );
		CHECK( probe_b.size() == probe_c.size() );
		CHECK( probe_b.refcount() == 2 );
	}

	SUBCASE( "move constructor" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s };
		detail::buffer_test_probe probe_b{ b};
		CHECK( probe_b.shared() != nullptr );
		CHECK( probe_b.shared_data() != nullptr );
		CHECK( probe_b.shared_size() == s.size() );
		CHECK( probe_b.data() != nullptr );
		CHECK( probe_b.data() == probe_b.shared_data() );
		CHECK( probe_b.size() == s.size() );
		CHECK( probe_b.refcount() == 1 );
		CHECK( probe_b.policy() == buffer::policy::copy_on_write );
		CHECK( ( void* )probe_b.shared_data() != ( void* )s.data() );
		CHECK( ::memcmp( probe_b.shared_data(), s.data(), s.size() ) == 0 );

		auto b_shared = probe_b.shared();
		auto b_shared_data = probe_b.shared_data();
		auto b_shared_size = probe_b.shared_size();
		auto b_data = probe_b.data();
		auto b_size = probe_b.size();

		buffer c{ std::move( b ) };
		detail::buffer_test_probe probe_c{ c};
		CHECK( probe_c.shared() != nullptr );
		CHECK( probe_c.shared() == b_shared );
		CHECK( probe_c.shared_data() == b_shared_data );
		CHECK( probe_c.shared_size() == b_shared_size );
		CHECK( probe_c.data() == b_data );
		CHECK( probe_c.size() == b_size );
		CHECK( probe_c.refcount() == 1 );

		CHECK( probe_b.shared() == nullptr );
		CHECK( probe_b.data() == nullptr );
		CHECK( probe_b.size() == 0 );
	}
}


TEST_CASE( "nodeoze/smoke/buffer/policies" )
{
	SUBCASE( "copy on write" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s };
		detail::buffer_test_probe pb{ b};
		CHECK( pb.policy() == buffer::policy::copy_on_write );

		buffer c{ b };
		detail::buffer_test_probe pc{ c};
		CHECK( pc.refcount() == 2 );
		CHECK( pc.shared() == pb.shared() );
		
		c.put( 0, static_cast< std::uint8_t >( 'x' ) );

		CHECK( pb.refcount() == 1 );
		CHECK( pc.refcount() == 1 );
		CHECK( pb.shared() != pc.shared() );
		CHECK( pb.shared_data() == pb.data() );
		CHECK( pc.shared_data() == pc.data() );
		CHECK( pc.data() != pb.data() );
		std::string ss{ "xbcdefghijklmnop" };
		CHECK( ::memcmp( pc.data(), ss.data(), ss.size() ) == 0 );
		CHECK( ::memcmp( pb.data(), s.data(), s.size() ) == 0 );
	}

	SUBCASE( "exclusive - copy constructor" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s, buffer::policy::exclusive };
		detail::buffer_test_probe pb{ b};
		CHECK( pb.policy() == buffer::policy::exclusive );

		buffer c{ b };
		detail::buffer_test_probe pc{ c};

		CHECK( pc.policy() == buffer::policy::copy_on_write );

		CHECK( pb.refcount() == 1 );
		CHECK( pc.refcount() == 1 );
		CHECK( pb.shared() != pc.shared() );
		CHECK( pb.shared_data() == pb.data() );
		CHECK( pc.shared_data() == pc.data() );
		CHECK( pc.data() != pb.data() );
		CHECK( ::memcmp( pc.data(), s.data(), s.size() ) == 0 );
		CHECK( ::memcmp( pb.data(), s.data(), s.size() ) == 0 );
	}

	SUBCASE( "exclusive - slice" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s, buffer::policy::exclusive };
		detail::buffer_test_probe pb{ b};
		CHECK( pb.policy() == buffer::policy::exclusive );

		buffer c = b.slice( 0, b.size() ); 
		detail::buffer_test_probe pc{ c};

		CHECK( pc.policy() == buffer::policy::copy_on_write );

		CHECK( pb.refcount() == 1 );
		CHECK( pc.refcount() == 1 );
		CHECK( pb.shared() != pc.shared() );
		CHECK( pb.shared_data() == pb.data() );
		CHECK( pc.shared_data() == pc.data() );
		CHECK( pc.data() != pb.data() );
		CHECK( ::memcmp( pc.data(), s.data(), s.size() ) == 0 );
		CHECK( ::memcmp( pb.data(), s.data(), s.size() ) == 0 );
	}

	SUBCASE( "no_copy_on_write" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s, buffer::policy::no_copy_on_write };
		detail::buffer_test_probe pb{ b};
		CHECK( pb.policy() == buffer::policy::no_copy_on_write );

		buffer c{ b };
		detail::buffer_test_probe pc{ c};

		CHECK( pc.policy() == buffer::policy::no_copy_on_write );

		CHECK( pb.refcount() == 2 );
		CHECK( pc.refcount() == 2 );
		CHECK( pb.shared() == pc.shared() );
		CHECK( pb.shared_data() == pb.data() );
		CHECK( pc.shared_data() == pc.data() );
		CHECK( pc.data() == pb.data() );
		CHECK( ::memcmp( pc.data(), s.data(), s.size() ) == 0 );
		CHECK( ::memcmp( pb.data(), s.data(), s.size() ) == 0 );

		c.put( 0, static_cast< std::uint8_t >( 'x' ) );

		std::string ss{ "xbcdefghijklmnop" };
		
		CHECK( pb.refcount() == 2 );
		CHECK( pc.refcount() == 2 );
		CHECK( pb.shared() == pc.shared() );
		CHECK( pb.shared_data() == pb.data() );
		CHECK( pc.shared_data() == pc.data() );
		CHECK( pc.data() == pb.data() );
		CHECK( ::memcmp( pc.data(), ss.data(), ss.size() ) == 0 );
		CHECK( ::memcmp( pb.data(), ss.data(), ss.size() ) == 0 );
	}

	SUBCASE( "make exclusive from unique copy on write" )
	{
	}

	SUBCASE( "make exclusive from non-unique copy on write" )
	{
		std::string s{ "abcdefghijklmnop" };
		buffer b{ s };
		detail::buffer_test_probe pb{ b};
		CHECK( pb.policy() == buffer::policy::copy_on_write );

		buffer c{ b };
		detail::buffer_test_probe pc{ c};
		CHECK( pc.refcount() == 2 );
		CHECK( pc.shared() == pb.shared() );
		CHECK( pc.policy() == buffer::policy::copy_on_write );
		CHECK( pc.refcount() == 2 );

		c.make_exclusive();
		CHECK( pc.policy() == buffer::policy::exclusive );
		CHECK( pb.policy() == buffer::policy::copy_on_write );

		CHECK( c.is_exclusive() );
		CHECK( c.is_unique() );
		CHECK( b.is_copy_on_write() );

		CHECK( pb.refcount() == 1 );
		CHECK( pc.refcount() == 1 );
		CHECK( pb.shared() != pc.shared() );
		CHECK( pb.shared_data() == pb.data() );
		CHECK( pc.shared_data() == pc.data() );
		CHECK( pc.data() != pb.data() );
		CHECK( ::memcmp( pc.data(), s.data(), s.size() ) == 0 );
		CHECK( ::memcmp( pb.data(), s.data(), s.size() ) == 0 );
	}

	SUBCASE( "make exclusive from unique no copy on write" )
	{
	}

	SUBCASE( "make exclusive from non-unique no copy on write" )
	{
	}

	SUBCASE( "make no copy on write from exclusive" )
	{
	}

	SUBCASE( "make no copy on write from unique copy on write" )
	{
	}

	SUBCASE( "make no copy on write from non-unique copy on write" )
	{
	}

	SUBCASE( "make copy on write from exclusive" )
	{
	}

	SUBCASE( "make copy on write from unique no copy on write" )
	{
	}

	SUBCASE( "make copy on write from non-unique no copy on write" )
	{
	}
}

TEST_CASE( "nodeoze/smoke/buffer/resizing_realloc" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/slices" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/assignment" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/base_pointer_access" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/element_get_put" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/fill" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/find" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/rotate" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/checksum" )
{
}

TEST_CASE( "nodeoze/smoke/buffer/appending" )
{
}

