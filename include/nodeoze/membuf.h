#ifndef NODEOZE_MEMBUF_H
#define NODEOZE_MEMBUF_H

#include <iosfwd>
#include <streambuf>
#include <nodeoze/nbuffer.h>
#include <functional>

#define SB_DEBUG_ON 0

#if (SB_DEBUG_ON)

#define SB_DEBUG( ... )	{ std::cout << __VA_ARGS__ << std::endl; print_state(); }

#else

#define SB_DEBUG( ... )	

#endif

#ifndef NODEOZE_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE
#define NODEOZE_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE 8
#endif


namespace nodeoze
{

class omembuf : public std::streambuf
{
public:

	friend class imembuf;

	omembuf( std::streamsize size, buffer::policy pol = buffer::policy::copy_on_write )
	:
	m_buf{ static_cast<buffer::size_type>( size ), pol },
	m_high_water_mark{ 0 }
	{
		set_ptrs();
	}

	omembuf( buffer const& buf )
	:
	m_buf{ buf },
	m_high_water_mark{ 0 }
	{
		set_ptrs();
	}

	omembuf( buffer&& buf )
	:
	m_buf{ std::move( buf ) },
	m_high_water_mark{ 0 }
	{
		set_ptrs();
	}

	omembuf& 
	clear() noexcept
	{
		set_ptrs();
		reset_high_water_mark();
		return *this;
	}

	buffer 
	get_buffer( bool force_copy = false ) const
	{
		high_water_mark( ppos() );
		SB_DEBUG( "in omembuf::get_buffer(), ppos() is " << ppos() << ", hwm is " << m_high_water_mark )
		return m_buf.slice( 0, m_high_water_mark, force_copy );
	}

	buffer
	release_buffer()
	{
		high_water_mark( ppos() );
		m_buf.size( m_high_water_mark );
		m_high_water_mark = 0;
		return std::move( m_buf );
	}

	void
	print_state() const
	{
		pos_type endpos = epptr() - pbase();
		pos_type pos = ppos();
		std::cout << "[ " << (void*) pbase() << ", " << pos << ", " << endpos << " ]" << " hwm: " << m_high_water_mark << std::endl;
		m_buf.dump( std::cout );
		std::cout.flush();
	}

protected:

	// force a hard lower bound to avoid non-resizing dilemma in accommodate_put, when cushioned == required == 1
	static constexpr std::streamsize min_alloc_size = 
		( NODEOZE_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE > 16 ) ? 
			NODEOZE_BUFFER_OUT_STREAMBUF_MIN_ALLOC_SIZE : 16;

	virtual std::streamsize
	xsputn( const char_type* bytes, std::streamsize n ) override
	{
		SB_DEBUG( "enter omembuf::xsputn( " << n << " ): " )

		accommodate_put( n );
		::memcpy( pptr(), bytes, n );
		pbump( n );

		SB_DEBUG( "leave omembuf::xsputn, pos " << ppos() << ": " )

		return n;
	}

	virtual int_type
	overflow( int_type byte = traits_type::eof() ) override
	{
		SB_DEBUG( "enter omembuf::overflow( " << byte << " ): " )

		int_type result = traits_type::eof();

		if ( byte != traits_type::eof() )
		{
			accommodate_put( 1 );
			*pptr() = byte;
			pbump(1);
			result = byte;
		}

		SB_DEBUG( "leave omembuf::overflow, result " << result << ": " )

		return result;
	}

	virtual pos_type 
	seekpos (pos_type pos, std::ios_base::openmode which = std::ios_base::out) override
	{
		SB_DEBUG( "enter omembuf::seekpos( " << pos << " ): " )

		pos_type result = -1;
		if ( which == std::ios_base::out )
		{
			pos_type current_pos = ppos();
			high_water_mark( current_pos );
			result = set_position( pos );
		}

		return result;
	}

	virtual pos_type 
	seekoff (off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::out) override
	{
		pos_type result = -1;

		if ( which == std::ios_base::out )
		{
			pos_type current_pos = ppos();
			auto hwm = high_water_mark( current_pos );

			pos_type new_pos = 0;
			switch ( way )
			{
				case std::ios_base::seekdir::beg:
				{
					new_pos = off;
				}
				break;
	
				case std::ios_base::seekdir::cur:
				{
					new_pos = current_pos + off;
				}
				break;

				case std::ios_base::seekdir::end:
				{
					new_pos = hwm + off;
				}
				break;
			}

			result = set_position( new_pos );
		}
		return result;
	}

	inline pos_type 
	set_position( pos_type pos )
	{
		SB_DEBUG( "enter omembuf::set_position( " << pos << " ): " )

		pos_type result = -1;

		if ( pos >=  0 )
		{
			pos_type current_pos = ppos();

			auto hwm = high_water_mark( current_pos );

			if ( pos > hwm )
			{
				off_type fill_size = pos - hwm;
				accommodate_put( fill_size );
				::memset( pptr(), 0, fill_size );
				pbump( fill_size );
				high_water_mark( ppos() );
			}
			else if ( pos >= current_pos )
			{
				pbump( pos - current_pos);
			}
			else
			{
				setp( pbase(), pbase() + m_buf.size() );
				pbump( pos );
			}

			result = pos;
		}

		SB_DEBUG( "after omembuf::set_position( " << pos << " ): " )

		return result;
	}

	inline void
	set_ptrs()
	{
		auto base = reinterpret_cast< char_type * >( m_buf.mutable_data() );
		setp( base, base + m_buf.size() );
	}

	inline void
	accommodate_put( std::streamsize n )
	{
		assert(std::less_equal<char *>()(pptr(), epptr()));
		std::streamsize remaining = epptr() - pptr();
		if ( remaining < n )
		{
			off_type pos = pptr() - pbase();
			std::streamsize required = pos + n;
			std::streamsize cushioned_size = ( required * 3 ) / 2;
			m_buf.size( std::max( min_alloc_size, cushioned_size ) ); 
			set_ptrs();
			pbump( pos );
		}
	}
	
	inline pos_type
	ppos() const
	{
		return static_cast< pos_type >( pptr() - pbase() ); 
	}

	inline pos_type
	high_water_mark( pos_type pos ) const
	{
		if ( pos > m_high_water_mark )
		{
			m_high_water_mark = pos;
		}

		assert( static_cast< buffer::size_type >( m_high_water_mark ) <= m_buf.size() );

		return m_high_water_mark;
	}

	inline pos_type
	high_water_mark() const
	{
		auto pos = ppos();

		if ( pos > m_high_water_mark )
		{
			m_high_water_mark = pos;
		}

		assert( static_cast< buffer::size_type >( m_high_water_mark ) <= m_buf.size() );

		return m_high_water_mark;
	}

	inline void
	reset_high_water_mark( pos_type pos = 0 )
	{
		m_high_water_mark = pos;
	}

	buffer				m_buf;
	mutable pos_type	m_high_water_mark;
};

class imembuf : public std::streambuf
{
public:

	inline 
	imembuf( buffer const& buf )
	:
	m_buf{ buf }
	{
		set_ptrs();
	}

	inline 
	imembuf( buffer&& buf )
	:
	m_buf{ std::move( buf ) }
	{
		set_ptrs();
	}

	inline imembuf& 
	rewind()
	{
		set_ptrs();
		return *this;
	}

	buffer 
	get_buffer( bool force_copy = false ) const
	{
		return m_buf.slice( 0, m_buf.size(), force_copy );
	}

	void
	print_state() const
	{
		pos_type endpos = egptr() - eback();
		pos_type pos = gpos();
		std::cout << "[ " << (void*) eback() << ", " << pos << ", " << endpos << " ]" << std::endl;
		m_buf.dump( std::cout );
		std::cout.flush();
	}


protected:

	inline void
	set_ptrs()
	{
		char_type *base = reinterpret_cast< char_type* >( const_cast< buffer::elem_type* >( m_buf.const_data() ) );
		setg( base, base, base + m_buf.size() );
	} 

	virtual std::streamsize 
	xsgetn (char_type* s, std::streamsize n) override
	{
		std::streamsize result = 0;
		std::streamsize copy_size = std::min( n, egptr() - gptr() );
		if ( copy_size > 0 )
		{
			::memcpy( s, gptr(), copy_size );
			gbump( copy_size );
			result = copy_size;
		}

		return result;
	}

	virtual pos_type 
	seekpos (pos_type pos, std::ios_base::openmode which = std::ios_base::in) override
	{
		SB_DEBUG( "enter imembuf::seekpos( " << pos << " ): " )

		pos_type result = -1;
		if ( which == std::ios_base::in )
		{
			result = set_position( pos );
		}

		return result;
	}


	inline pos_type
	gpos() const
	{
		return gptr() - eback();
	}

	virtual pos_type 
	seekoff (off_type off, std::ios_base::seekdir way, std::ios_base::openmode which = std::ios_base::in) override
	{
		pos_type result = -1;

		if ( which == std::ios_base::in )
		{
			pos_type new_pos = 0;

			switch ( way )
			{
				case std::ios_base::seekdir::beg:
				{
					new_pos = off;
				}
				break;
	
				case std::ios_base::seekdir::cur:
				{
					new_pos = gpos() + off;
				}
				break;

				case std::ios_base::seekdir::end:
				{
					new_pos = ( egptr() - eback() ) + off;
				}
				break;
			}

			result = set_position( new_pos );
		}

		return result;
	}

	inline pos_type 
	set_position( pos_type pos )
	{
		SB_DEBUG( "enter imembuf::set_position( " << pos << " ): " )

		pos_type result = -1;

		if ( pos >=  0 || pos <= ( egptr() - eback() ) )
		{
			set_ptrs();
			gbump( pos );
			result = pos;
		}

		SB_DEBUG( "after imembuf::set_position( " << pos << " ): " )

		return result;
	}

	buffer		m_buf;
};

} // namespace nodeoze

#endif // NODEOZE_MEMBUF_H