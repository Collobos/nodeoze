/*
 * The MIT License
 *
 * Copyright 2017 David Curtis.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* 
 * File:   ibstream.h
 * Author: David Curtis
 *
 * Created on June 29, 2017, 1:40 PM
 */

#ifndef NODEOZE_BSTREAM_IBSTREAM_H
#define NODEOZE_BSTREAM_IBSTREAM_H

#include <nodeoze/bstream/ibstream_traits.h>
#include <nodeoze/bstream/numeric_deserializers.h>
#include <nodeoze/bstream/context.h>
#include <nodeoze/bstream/numstream.h>
#include <deque>

namespace nodeoze
{
namespace bstream
{

/*! \class ibstream
 *	\brief binary input stream
 *	
 *	An instance of ibstream is associated with a read-only buffer. The caller 
 *	can read from the stream in a variety of ways, depending on the calling 
 *	context and the type being read. At present, ibstream doesn't explicity
 *	support run-time polymorphism. The reading context is assumed to know
 *	\a a \a priori the contents of buffer as streamed by the sender ( that is,
 *	the types, number, and order of the items ).
 */
class ibstream : public inumstream
{
public:
    using base = inumstream;

    template< class U, class E > friend struct value_deserializer;

    using saved_ptr_info = std::pair< std::type_index , std::shared_ptr< void > >;

    class ptr_deduper
    {
    public:

        template< class T >
        void
        save_ptr( std::shared_ptr< T > ptr )
        {
            m_saved_ptrs.push_back( saved_ptr_info( typeid( *ptr ), ptr ) );
        }

        saved_ptr_info const&
        get_saved_ptr( std::size_t index )
        {
            return m_saved_ptrs[ index ];
        }

        void
        clear()
        {
            m_saved_ptrs.clear();
        }

    private:

        std::deque< saved_ptr_info >        m_saved_ptrs;
    };


    ibstream() = delete;
    ibstream( ibstream const& ) = delete;
    ibstream( ibstream&& ) = delete;

    ibstream( std::unique_ptr< bstream::ibstreambuf > strmbuf, context_base const& cntxt = get_default_context() )
    : 
    inumstream{ std::move( strmbuf ), cntxt.get_context_impl()->byte_order() },
    m_context{ cntxt.get_context_impl() },
    m_ptr_deduper{ m_context->dedup_shared_ptrs() ? std::make_unique< ptr_deduper >() : nullptr }
    {}

    template< class T >
    typename std::enable_if_t< is_ibstream_constructible< T >::value, T >
    read_as()
    {
        return  T( *this );
    }

    template< class T >
    typename std::enable_if_t<
        is_ibstream_constructible< T >::value 
        && std::is_default_constructible< T >::value, T >
    read_as( std::error_code& ec )
    {
        clear_error( ec );
        try 
        {
            return  T( *this );
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
            return T{};
        }
    }

    template< class T >
    typename std::enable_if_t< use_value_deserializer< T >::value, T >
    read_as()
    {
        return value_deserializer< T >::get( *this );
    }

    template< class T >
    typename std::enable_if_t< use_value_deserializer< T >::value
        && std::is_default_constructible< T >::value, T >
    read_as( std::error_code& ec )
    {
        clear_error( ec );
        try
        {
            return value_deserializer< T >::get( *this );
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
            return T{};
        }
    }

    template< class T >
    typename std::enable_if_t< use_ref_deserializer< T >::value, T >
    read_as()
    {
        T obj;
        ref_deserializer< T >::get( *this, obj );
        return obj;
    }

    template< class T >
    typename std::enable_if_t< use_ref_deserializer< T >::value, T >
    read_as( std::error_code& ec )
    {
        clear_error( ec );
        T obj;
        try
        {
            ref_deserializer< T >::get( *this, obj );
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return obj;
    }

    template< class T >
    typename std::enable_if_t< has_ref_deserializer< T >::value, ibstream& >
    read_as( T& obj )
    {
        return ref_deserializer< T >::get( *this, obj );
    }

    template< class T >
    typename std::enable_if_t< has_ref_deserializer< T >::value, ibstream& >
    read_as( T& obj, std::error_code& ec )
    {
        clear_error( ec );
        try
        {
            ref_deserializer< T >::get( *this, obj );
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return *this;
    }

    template< class T >
    typename std::enable_if_t<
        !has_ref_deserializer< T >::value &&
        is_ibstream_constructible< T >::value &&
        std::is_assignable< T&,T >::value,
        ibstream& > 
    read_as( T& obj )
    {
        obj = T( *this );
        return *this;
    }

    template< class T >
    typename std::enable_if_t<
        !has_ref_deserializer< T >::value &&
        is_ibstream_constructible< T >::value &&
        std::is_assignable< T&,T >::value,
        ibstream& > 
    read_as( T& obj, std::error_code& ec )
    {
        clear_error( ec );
        try
        {
            obj = T( *this );
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return *this;
    }

    template< class T >
    typename std::enable_if_t<
        !has_ref_deserializer< T >::value &&
        !is_ibstream_constructible< T >::value &&
        has_value_deserializer< T >::value &&
        std::is_assignable< T&,T >::value,
        ibstream& >
    read_as( T& obj )
    {
        obj = value_deserializer< T >::get( *this );
        return *this;
    }

    template< class T >
    typename std::enable_if_t<
        !has_ref_deserializer< T >::value &&
        !is_ibstream_constructible< T >::value &&
        has_value_deserializer< T >::value &&
        std::is_assignable< T&,T >::value,
        ibstream& >
    read_as( T& obj, std::error_code& ec )
    {
        clear_error( ec );
        try
        {
            obj = value_deserializer< T >::get( *this );
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return *this;
    }

    std::size_t
    read_string_header();

    std::size_t
    read_string_header( std::error_code& ec );

    std::size_t 
    read_array_header();
    
    std::size_t 
    read_array_header( std::error_code& ec );

    std::size_t
    read_map_header();

    std::size_t
    read_map_header( std::error_code& ec );

    ibstream&
    check_map_key( std::string const& key );

    ibstream&
    check_map_key( std::string const& key, std::error_code& ec );

    ibstream&
    check_array_header( std::size_t expected );

    ibstream&
    check_array_header( std::size_t expected, std::error_code& ec );

    ibstream&
    check_map_header( std::size_t expected );

    ibstream&
    check_map_header( std::size_t expected, std::error_code& ec );

    std::size_t 
    read_blob_header();

    std::size_t 
    read_blob_header( std::error_code& ec );

    nodeoze::buffer
    read_blob_body( std::size_t nbytes )
    {
        return getn( nbytes );
    }

    nodeoze::buffer
    read_blob_body( std::size_t nbytes, std::error_code& ec )
    {
        return getn( nbytes, ec );
    }

    nodeoze::buffer
    read_blob()
    {
        auto nbytes = read_blob_header();
        return read_blob_body( nbytes );
    }

    nodeoze::buffer
    read_blob( std::error_code& ec );

    std::size_t
    read_ext_header( std::uint8_t& ext_type );

    std::size_t
    read_ext_header( std::uint8_t& ext_type, std::error_code& ec );

    nodeoze::buffer
    read_ext_body( std::size_t nbytes )
    {
        return getn( nbytes );
    }

    nodeoze::buffer
    read_ext_body( std::size_t nbytes, std::error_code& ec )
    {
        return getn( nbytes, ec );
    }

    void
    read_nil()
    {
        auto tcode = base::get();
        if ( tcode != typecode::nil )
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
    }

    void
    read_nil( std::error_code& ec )
    {
        clear_error( ec );
        auto tcode = base::get();
        if ( tcode != typecode::nil )
        {
            ec = make_error_code( bstream::errc::type_error );
        }
    }

    void
    reset()
    {
        if ( m_ptr_deduper ) m_ptr_deduper->clear();
        position( 0 );
    }

    void
    reset( std::error_code& err )
    {
        if ( m_ptr_deduper ) m_ptr_deduper->clear();
        position( 0, err );
    }

    // virtual void 
    // rewind()
    // {
    //     position( 0 );
    // }

    // virtual void 
    // rewind( std::error_code& err )
    // {
    //     position( 0, err );
    // }

    buffer
    get_msgpack_obj_buf();

    buffer
    get_msgpack_obj_buf( std::error_code& ec )
    {
        clear_error( ec );
        try
        {
            return get_msgpack_obj_buf();
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
            return buffer{};
        }
    }

protected:

    /*
     *  Pointers are streamed as a 2 - element object ( array or map ).
     *  The first element is the type tag ( int ). If the tag is -1,
     *  then there is no run-time type information in the stream;
     *  the object should be constructed based on the assumption that
     *  it is an instance of T. If it is not -1, then it is interpreted
     *  as a type tag, and the ibstream instance is expected to have
     *  a poly_context that can create an instance from this tag.
     * 
     *  The second element is either a nil, a positive integer value, 
     *  or a serialized object ( array or map ). If it is nil, a nullptr
     *  value is returned. 
     *  If it is an integer,
     *  it is interpreted as an id associated with a previously-
     *  stream object, which is expected to be stored in the stream 
     *  graph context.
     *  If it is an object, an instance of the appropriate type
     *  ( as indicated by the parameter T and/or the tag value as
     *  interpreted by the poly_contexts ) is constructed from the 
     *  streamed object, cast to the return type ( possibly mediated 
     *  by the poly_context ), and returned.
     * 
     */
    template< class T >
    std::shared_ptr< T >
    read_as_shared_ptr()
    {
        std::shared_ptr< T > result{ nullptr };
        auto n = read_array_header();
        if ( n != 2 )
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }

        auto type_tag = read_as< int >();
        auto code = peek();
        if ( code == typecode::nil ) // return result ( nullptr ) as is
        {
            code = get();
        }
        else if ( typecode::is_positive_int( code ) ) // saved ptr
        {
            result = get_saved_ptr< T >( type_tag );
        }
        else // not saved ptr
        {
            result = deserialize_as_shared_ptr< T >( type_tag );
        }
        return result;
    }
    
    template< class T >
    std::unique_ptr< T >
    read_as_unique_ptr()
    {
        auto n = read_array_header();
        if ( n != 2 )
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
        
        auto tag = read_as< poly_tag_type >();

        auto code = peek();
        
        if ( code == typecode::nil ) // nullptr
        {
            code = get();
            return nullptr;
        }
        else // streamed object
        {
            return deserialize_as_unique_ptr< T >( tag );
        }
    }

    template< class T >
    std::shared_ptr< T >
    deserialize_as_shared_ptr( std::enable_if_t< std::is_abstract< T >::value, poly_tag_type > tag )
    {
        std::shared_ptr< T > result{ nullptr };

        if ( tag == invalid_tag )
        {
            throw std::system_error{ make_error_code( bstream::errc::abstract_non_poly_class ) };
        }

        result =  m_context->create_shared< T >( tag, *this );

        if ( m_ptr_deduper )
        {
            m_ptr_deduper->save_ptr( result );
        }

        return result;
    }

    template< class T >
    std::shared_ptr< T >
    deserialize_as_shared_ptr( std::enable_if_t< ! std::is_abstract< T >::value, poly_tag_type > tag )
    {
        std::shared_ptr< T > result{ nullptr };

        if ( tag == invalid_tag ) // read as T
        {
            result = shared_ptr_deserializer< T >::get( *this );
        }
        else
        {
            result =  m_context->create_shared< T >( tag, *this );
        }

        if ( m_ptr_deduper )
        {
            m_ptr_deduper->save_ptr( result );
        }

        return result;
    }

    template< class T >
    std::unique_ptr< T >
    deserialize_as_unique_ptr( std::enable_if_t< std::is_abstract< T >::value, poly_tag_type > tag )
    {
        if ( tag == invalid_tag ) // read as T
        {
            throw std::system_error{ make_error_code( bstream::errc::abstract_non_poly_class ) };
        }
        else
        {
            return std::unique_ptr< T >( m_context->create_raw< T >( tag, *this ) );
        }
    }

    template< class T >
    std::unique_ptr< T >
    deserialize_as_unique_ptr( std::enable_if_t< ! std::is_abstract< T >::value, poly_tag_type > tag )
    {
        if ( tag == invalid_tag ) // read as T
        {
            return unique_ptr_deserializer< T >::get( *this );
        }
        else
        {
            return std::unique_ptr< T >( m_context->create_raw< T >( tag, *this ) );
        }
    }

    template< class T >
    std::shared_ptr< T >
    get_saved_ptr( int type_tag )
    {
        std::shared_ptr< T > result{ nullptr };

        if ( m_ptr_deduper )
        {
            auto index = read_as< std::size_t >();
            auto info = m_ptr_deduper->get_saved_ptr( index );
            if ( type_tag > -1 )
            {
                auto saved_tag = m_context->get_type_tag( info.first );
                if ( saved_tag != type_tag )
                {
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                }
                if ( m_context->can_downcast_ptr( type_tag, m_context->get_type_tag( typeid( T ) ) ) )
                {
                    result = std::static_pointer_cast< T >( info.second );
                }
                else
                {
                    throw std::system_error{ make_error_code( bstream::errc::invalid_ptr_downcast ) };
                }
            }
            else // no type info in stream
            {
                if ( info.first == typeid( T ) )
                {
                    result = std::static_pointer_cast< T >( info.second );
                }
                else
                {
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                }
            }
        }
        else 
        {
            throw std::system_error{ make_error_code( bstream::errc::context_mismatch ) };
        }
        return result;
    }


    std::error_code
    read_error_code();

    void 
    ingest( bufwriter& os );

    std::unique_ptr< bufwriter >                    m_bufwriter = nullptr;
    std::shared_ptr< const context_impl_base >      m_context;
    std::unique_ptr< ptr_deduper >                  m_ptr_deduper;
};
	        
template< class T >
inline typename std::enable_if_t< has_ref_deserializer< T >::value, ibstream& > 
operator>>( ibstream& is, T& obj )
{
    return ref_deserializer< T >::get( is, obj );
}

template< class T >
inline typename std::enable_if_t<
    !has_ref_deserializer< T >::value &&
    is_ibstream_constructible< T >::value &&
    std::is_assignable< T&,T >::value,
    ibstream& > 
operator>>( ibstream& is, T& obj )
{
    obj = T( is );
    return is;
}

template< class T >
inline typename std::enable_if_t<
    !has_ref_deserializer< T >::value &&
    !is_ibstream_constructible< T >::value &&
    has_value_deserializer< T >::value &&
    std::is_assignable< T&,T >::value,
    ibstream& >
operator>>( ibstream& is, T& obj )
{
    obj = value_deserializer< T >::get( is );
    return is;
}

template< class T >
struct value_deserializer< T, std::enable_if_t< std::is_enum< T >::value > >
{
    T 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static T get( ibstream& is )
    {
        auto ut = is.read_as< typename std::underlying_type< T >::type >();
        return static_cast< T >( ut );
    }				
};

template< class T >
struct ref_deserializer< T, std::enable_if_t< has_deserialize_method< T >::value > >
{

    ibstream& 
    operator()( ibstream& is, T& obj ) const
    {
        return get( is, obj );
    }

    static ibstream&
    get( ibstream& is, T& obj )
    {
        return obj.deserialize( is );
    }
};

template< class T >
struct value_deserializer< T, std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    T 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static T get( ibstream& is )
    {
        return T{ is };
    }
};

template<>
struct value_deserializer< bool >
{
    bool 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static bool get( ibstream& is )
    {
        auto tcode = is.get();
        switch( tcode )
        {
            case typecode::bool_true:
                return true;
            case typecode::bool_false:
                return false;
            default:
                throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
    }
};

template<>
struct value_deserializer< float >
{
    float 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static float get( ibstream& is )
    {
        auto tcode = is.get();
        if ( tcode == typecode::float_32 )
        {
            std::uint32_t unpacked = is.get_num< std::uint32_t >();
            return reinterpret_cast< float& >( unpacked );
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
    }
};

template<>
struct value_deserializer< double >
{
    double 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static double get( ibstream& is )
    {
        auto tcode = is.get();
        if ( tcode == typecode::float_32 )
        {
            std::uint32_t unpacked = is.get_num< std::uint32_t >();
            return static_cast< double >( reinterpret_cast< float& >( unpacked ) );
        }
        else if ( tcode == typecode::float_64 )
        {
            std::uint64_t unpacked = is.get_num< std::uint64_t >();
            return reinterpret_cast< double& >( unpacked );
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
    }
};


/*
 *	Value deserializers for shared pointer types
 */

/*
 *	Prefer stream-constructed when available
 */

template< class T >
struct value_deserializer< std::shared_ptr< T > >
{
    std::shared_ptr< T > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::shared_ptr< T >
    get( ibstream& is )
    {
        return is.read_as_shared_ptr< T >();
    }
};

template< class T >
struct ptr_deserializer< T, typename std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    T*
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static T*
    get( ibstream& is )
    {
        return new T{ is };
    }
};

template< class T >
struct shared_ptr_deserializer< T, typename std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    std::shared_ptr< T >
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static std::shared_ptr< T >
    get( ibstream& is )
    {
        return std::make_shared< T >( is );
    }
};

template< class T >
struct unique_ptr_deserializer< T, typename std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    std::unique_ptr< T >
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static std::unique_ptr< T >
    get( ibstream& is )
    {
        return std::make_unique< T >( is );
    }
};

/*
 *	If no stream constructor is available, prefer a value_deserializer< T >
 * 
 *	The value_deserializer< shared_ptr< T > > based on a value_deserializer< T > 
 *	requires a move contructor for T
 */

template< class T >
struct ptr_deserializer< T,
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        has_value_deserializer< T >::value &&
        std::is_move_constructible< T >::value > >
{
    T* 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static T*
    get( ibstream& is )
    {
        return new T{ value_deserializer< T >::get( is ) };
    }
};

template< class T >
struct shared_ptr_deserializer< T,
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        has_value_deserializer< T >::value &&
        std::is_move_constructible< T >::value > >
{
    std::shared_ptr< T >
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::shared_ptr< T >
    get( ibstream& is )
    {
        return std::make_shared< T >( value_deserializer< T >::get( is ) );
    }
};

template< class T >
struct unique_ptr_deserializer< T,
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        has_value_deserializer< T >::value &&
        std::is_move_constructible< T >::value > >
{
    std::unique_ptr< T >
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unique_ptr< T >
    get( ibstream& is )
    {
        return std::make_unique< T >( value_deserializer< T >::get( is ) );
    }
};

/*
 *	The value_deserializer< shared_ptr< T > > based on a ref_deserializer< T > 
 *	requires eiher a move contructor or copy constructor for T;
 *	prefer the move contructor
 */

template< class T >
struct ptr_deserializer< T,
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        !has_value_deserializer< T >::value &&
        has_ref_deserializer< T >::value &&
        std::is_default_constructible< T >::value > >
{
    T* 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static T*
    get( ibstream& is )
    {
        T *ptr = new T;
        ref_deserializer< T >::get( is, *ptr );
        return ptr;
    }
};

template< class T >
struct shared_ptr_deserializer< T,
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        !has_value_deserializer< T >::value &&
        has_ref_deserializer< T >::value &&
        std::is_default_constructible< T >::value > >
{
    std::shared_ptr< T >
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::shared_ptr< T >
    get( ibstream& is )
    {
        std::shared_ptr< T > ptr = std::make_shared< T >();
        ref_deserializer< T >::get( is, *ptr );
        return ptr;
    }
};

template< class T >
struct unique_ptr_deserializer< T,
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        !has_value_deserializer< T >::value &&
        has_ref_deserializer< T >::value &&
        std::is_default_constructible< T >::value > >
{
    std::unique_ptr< T > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unique_ptr< T >
    get( ibstream& is )
    {
        std::unique_ptr< T > ptr = std::make_unique< T >();
        ref_deserializer< T >::get( is, *ptr );
        return ptr;
    }
};

/*
 *	Value deserializers for unique pointer types
 */

template< class T >
struct value_deserializer< std::unique_ptr< T > >
{
    std::unique_ptr< T > 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static std::unique_ptr< T >
    get( ibstream& is )
    {
        return is.read_as_unique_ptr< T >();
    }
};

template<>
struct value_deserializer< nodeoze::buffer >
{
    nodeoze::buffer 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static nodeoze::buffer
    get( ibstream& is )
    {
        return is.read_blob();
    }
};

template<>
struct value_deserializer< std::error_code >
{
    std::error_code
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static std::error_code
    get( ibstream& is )
    {
        return is.read_error_code();
    }
};

template< class T >
struct ibstream_initializer< T, 
    typename std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    using param_type = ibstream&;
    using return_type = ibstream&;
    static return_type get( param_type is )
    {
        return is;
    }
};

template< class T >
struct ibstream_initializer < T, 
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        has_value_deserializer< T >::value > >
{
    using param_type = ibstream&;
    using return_type = T;
    static return_type get( param_type is )
    {
        return value_deserializer< T >::get( is );
    }
};
    
template< class T >
struct ibstream_initializer < T, 
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        !has_value_deserializer< T >::value &&
        std::is_default_constructible< T >::value &&
        ( std::is_copy_constructible< T >::value || std::is_move_constructible< T >::value ) &&
        has_ref_deserializer< T >::value > >
{
    using param_type = ibstream&;
    using return_type = T;
    static return_type get( param_type is )
    {
        T obj;
        ref_deserializer< T >::get( is, obj );
        return obj;
    }
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_IBSTREAM_H
