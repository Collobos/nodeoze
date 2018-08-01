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

#ifndef BSTREAM_IBSTREAM_H
#define BSTREAM_IBSTREAM_H

#include <cstdint>
#include <type_traits>
#include <sstream>
#include <deque>
#include <list>
#include <forward_list>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <tuple>
#include <typeindex>
#include <boost/endian/conversion.hpp>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/typecode.h>
#include <nodeoze/bstream/numstream.h>
#include <nodeoze/bstream/utils/traits.h>
#include <nodeoze/bstream/error_category_context.h>
#include <nodeoze/buffer.h>

namespace nodeoze
{
namespace bstream
{
class ibstream;

template< class T >
class streaming_base;

template< class T >
struct ibstream_ctor_detected : public std::is_constructible< T, ibstream& > {};

template< class T, class Enable = void >
struct has_streaming_base : public std::false_type {};

template< class T >
struct has_streaming_base< T,
    typename std::enable_if_t< std::is_base_of< bstream::streaming_base< T >, T >::value > >
: public std::true_type {};

template< class T, class Enable = void >
struct is_ibstream_constructible : public std::false_type {};

template<class T> 
struct is_ibstream_constructible< T, 
    typename std::enable_if_t< ibstream_ctor_detected< T >::value || has_streaming_base< T>::value > > 
    : public std::true_type {};

template<class T, class Enable = void>
struct value_deserializer;

template<class T, class Enable = void>
struct ref_deserializer;

template< class T, class Enable = void >
struct ptr_deserializer;

template< class T, class Enable = void >
struct shared_ptr_deserializer;

template< class T, class Enable = void >
struct unique_ptr_deserializer;

template<class T, class Enable = void>
struct serializer;

template< class Derived, class Base, class Enable = void >
struct base_serializer;

namespace detail
{
    template<class T>
    static auto test_ref_deserializer(int)
        -> utils::sfinae_true_if<decltype(ref_deserializer<T>::get( std::declval<ibstream&>(), std::declval<T&>() ))>;
    template<class>
    static auto test_ref_deserializer(long) -> std::false_type;
} // namespace detail

template<class T>
struct has_ref_deserializer : decltype(detail::test_ref_deserializer<T>(0)) {};

namespace detail
{
    template<class T>
    static auto test_value_deserializer(int)
        -> utils::sfinae_true_if<decltype(value_deserializer<T>::get( std::declval<ibstream&>() ))>;
    template<class>
    static auto test_value_deserializer(long) -> std::false_type;
} // namespace detail

template<class T>
struct has_value_deserializer : decltype(detail::test_value_deserializer<T>(0)) {};

namespace detail
{
    template<class T>
    static auto test_ptr_deserializer(int)
        -> utils::sfinae_true_if<decltype(ptr_deserializer<T>::get( std::declval<ibstream&>() ))>;
    template<class>
    static auto test_ptr_deserializer(long) -> std::false_type;
} // namespace detail

template<class T>
struct has_ptr_deserializer : decltype(detail::test_ptr_deserializer<T>(0)) {};

namespace detail
{
    template<class T>
    static auto test_shared_ptr_deserializer(int)
        -> utils::sfinae_true_if<decltype(shared_ptr_deserializer<T>::get( std::declval<ibstream&>() ))>;
    template<class>
    static auto test_shared_ptr_deserializer(long) -> std::false_type;
} // namespace detail

template<class T>
struct has_shared_ptr_deserializer : decltype(detail::test_shared_ptr_deserializer<T>(0)) {};

namespace detail
{
    template<class T>
    static auto test_deserialize_method(int)
        -> utils::sfinae_true_if<decltype( std::declval<T>().deserialize(std::declval<ibstream&>()) )>;
    template<class>
    static auto test_deserialize_method(long) -> std::false_type;
} // namespace detail

template<class T>
struct has_deserialize_method : decltype(detail::test_deserialize_method<T>(0)) {};

namespace detail
{
    template<class T>
    static auto test_ibstream_extraction_operator(int)
        -> utils::sfinae_true_if<decltype( std::declval<ibstream&>() >> std::declval<T&>() )>;
    template<class>
    static auto test_ibstream_extraction_operator(long) -> std::false_type;
} // namespace detail

template<class T>
struct has_ibstream_extraction_operator : decltype(detail::test_ibstream_extraction_operator<T>(0)) {};

template<class T, class Enable = void>
struct is_ref_deserializable : public std::false_type {};

template<class T>
struct is_ref_deserializable<T, 
    std::enable_if_t<
        has_ref_deserializer<T>::value ||
        (has_value_deserializer<T>::value && std::is_assignable<T&,T>::value) ||
        (is_ibstream_constructible<T>::value && std::is_assignable<T&,T>::value)>>
: public std::true_type {};

template<class T, class Enable = void>
struct is_value_deserializable : public std::false_type {};

template<class T>
struct is_value_deserializable<T, 
    std::enable_if_t<
        is_ibstream_constructible<T>::value ||
        has_value_deserializer<T>::value ||
        (has_ref_deserializer<T>::value && std::is_default_constructible<T>::value)>>
: public std::true_type {};

template<class T, class Enable = void>
struct is_ibstream_readable : public std::false_type {};

template<class T>
struct is_ibstream_readable<T, 
    std::enable_if_t<
        is_value_deserializable<T>::value ||
        is_ref_deserializable<T>::value>>
: public std::true_type {};


template<class T, class Enable = void>
struct use_value_deserializer : public std::false_type {};

template<class T>
struct use_value_deserializer<T,
    std::enable_if_t<
        !is_ibstream_constructible<T>::value && 
        has_value_deserializer<T>::value>>
: public std::true_type {};

template<class T, class Enable = void>
struct use_ref_deserializer : public std::false_type {};

template<class T>
struct use_ref_deserializer<T,
    std::enable_if_t<
        !is_ibstream_constructible<T>::value &&
        !has_value_deserializer<T>::value &&
        std::is_default_constructible<T>::value &&
        has_ref_deserializer<T>::value>>
: public std::true_type {};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        std::numeric_limits<T>::is_signed && 
        sizeof(T) == 1>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else if (tcode >= typecode::negative_fixint_min)
        {
            return static_cast<T>(static_cast<std::int8_t>(tcode));
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::int_8:
                    return static_cast<T>(static_cast<std::int8_t>(is.get()));
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }                
    }
};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        !std::numeric_limits<T>::is_signed && 
        sizeof(T) == 1>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::uint_8:
                    return static_cast<T>(is.get());
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }                
    }
};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        std::numeric_limits<T>::is_signed && 
        sizeof(T) == 2>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else if (tcode >= typecode::negative_fixint_min)
        {
            return static_cast<T>(static_cast<std::int8_t>(tcode));
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::int_8:
                    return static_cast<T>(is.get_num<int8_t>());
                case typecode::uint_8:
                    return static_cast<T>(is.get_num<uint8_t>());
                case typecode::int_16:
                    return static_cast<T>(is.get_num<int16_t>());
                case typecode::uint_16:
                {
                    std::uint16_t n = is.get_num<uint16_t>();
                    if (n <= std::numeric_limits<T>::max())
                    {
                        return static_cast<T>(n);
                    }
                    else
                    {
                        throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                    }
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        !std::numeric_limits<T>::is_signed && 
        sizeof(T) == 2>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::uint_8:
                    return static_cast<T>(is.get_num<uint8_t>());
                case typecode::uint_16:
                    return static_cast<T>(is.get_num<uint16_t>());
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        std::numeric_limits<T>::is_signed && 
        sizeof(T) == 4>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else if (tcode >= typecode::negative_fixint_min)
        {
            return static_cast<T>(static_cast<std::int8_t>(tcode));
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::int_8:
                    return static_cast<T>(is.get_num<int8_t>());
                case typecode::uint_8:
                    return static_cast<T>(is.get_num<uint8_t>());
                case typecode::int_16:
                    return static_cast<T>(is.get_num<int16_t>());
                case typecode::uint_16:
                    return static_cast<T>(is.get_num<uint16_t>());
                case typecode::int_32:
                    return static_cast<T>(is.get_num<int32_t>());
                case typecode::uint_32:
                {
                    std::uint32_t n = is.get_num<uint32_t>();
                    if (n <= std::numeric_limits<T>::max())
                    {
                        return static_cast<T>(n);
                    }
                    else
                    {
                        throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                    }
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        !std::numeric_limits<T>::is_signed && 
        sizeof(T) == 4>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::uint_8:
                    return static_cast<T>(is.get_num<uint8_t>());
                case typecode::uint_16:
                    return static_cast<T>(is.get_num<uint16_t>());
                case typecode::uint_32:
                    return static_cast<T>(is.get_num<uint32_t>());
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        std::numeric_limits<T>::is_signed && 
        sizeof(T) == 8>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else if (tcode >= typecode::negative_fixint_min)
        {
            return static_cast<T>(static_cast<std::int8_t>(tcode));
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::int_8:
                    return static_cast<T>(is.get_num<int8_t>());
                case typecode::uint_8:
                    return static_cast<T>(is.get_num<uint8_t>());
                case typecode::int_16:
                    return static_cast<T>(is.get_num<int16_t>());
                case typecode::uint_16:
                    return static_cast<T>(is.get_num<uint16_t>());
                case typecode::int_32:
                    return static_cast<T>(is.get_num<int32_t>());
                case typecode::uint_32:
                    return static_cast<T>(is.get_num<uint32_t>());
                case typecode::int_64:
                    return static_cast<T>(is.get_num<int64_t>());
                case typecode::uint_64:
                {
                    std::uint64_t n = is.get_num<uint64_t>();
                    if (n <= std::numeric_limits<T>::max())
                    {
                        return static_cast<T>(n);
                    }
                    else
                    {
                        throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                    }
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template<class T>
struct value_deserializer<T, 
    std::enable_if_t<
        std::numeric_limits<T>::is_integer && 
        !std::numeric_limits<T>::is_signed && 
        sizeof(T) == 8>>
{
    inline T 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static T get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode <= typecode::positive_fixint_max)
        {
            return static_cast<T>(tcode);
        }
        else
        {
            switch(tcode)
            {
                case typecode::bool_true:
                    return static_cast<T>(1);
                case typecode::bool_false:
                    return static_cast<T>(0);
                case typecode::uint_8:
                    return static_cast<T>(is.get_num<uint8_t>());
                case typecode::uint_16:
                    return static_cast<T>(is.get_num<uint16_t>());
                case typecode::uint_32:
                    return static_cast<T>(is.get_num<uint32_t>());
                case typecode::uint_64:
                    return static_cast<T>(is.get_num<uint64_t>());
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};

template<>
struct value_deserializer<std::string>
{
    inline std::string 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static std::string get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max)
        {
            std::uint8_t mask = 0x1f;
            std::size_t length = tcode & mask;
            return is.getn( length ).to_string(); // TODO: does this work? construct string?
        }
        else
        {
            switch(tcode)
            {
                case typecode::str_8:
                {
                    std::size_t length = is.get_num<std::uint8_t>();
                    return is.getn( length ).to_string();
                }
                case typecode::str_16:
                {
                    std::size_t length = is.get_num<std::uint16_t>();
                    return is.getn( length ).to_string();
                }
                case typecode::str_32:
                {
                    std::size_t length = is.get_num<std::uint32_t>();
                    return is.getn( length ).to_string();
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};	

template<>
struct value_deserializer<nodeoze::string_alias>
{
    inline nodeoze::string_alias 
    operator()(inumstream& is) const
    {
        return get(is);
    }

    inline static nodeoze::string_alias get(inumstream& is, std::size_t length)
    {
        return nodeoze::string_alias{ is.getn( length ) };
    }

    inline static nodeoze::string_alias get(inumstream& is)
    {
        auto tcode = is.get();
        if (tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max)
        {
            std::uint8_t mask = 0x1f;
            std::size_t length = tcode & mask;
            return get( is, length );
        }
        else
        {
            switch(tcode)
            {
                case typecode::str_8:
                {
                    std::size_t length = is.get_num<std::uint8_t>();
                    return get( is, length );
                }
                case typecode::str_16:
                {
                    std::size_t length = is.get_num<std::uint16_t>();
                    return get( is, length );
                }
                case typecode::str_32:
                {
                    std::size_t length = is.get_num<std::uint32_t>();
                    return get( is, length );
                }
                default:
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
    }
};	

class context_impl_base : public error_category_context
{
public:
    static constexpr bool is_valid_tag( poly_tag_type tag ) { return tag != invalid_tag; };

    context_impl_base( bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    error_category_context{},
    m_dedup_shared_ptrs{ dedup_shared_ptrs },
    m_byte_order{ byte_order }
    {}

    context_impl_base( error_category_context::category_init_list categories, bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    error_category_context{ categories },
    m_dedup_shared_ptrs{ dedup_shared_ptrs },
    m_byte_order{ byte_order }
    {}

    virtual ~context_impl_base() {}

    virtual poly_tag_type
    get_type_tag( std::type_index index ) const = 0;

    template< class T >
    inline poly_tag_type
    get_type_tag() const
    {
        return get_type_tag( typeid( T ) );
    }

    virtual bool
    can_downcast_ptr( poly_tag_type from, poly_tag_type to ) const = 0;

    virtual void*
    create_raw_from_tag( poly_tag_type tag, ibstream& is ) const = 0;

    virtual std::shared_ptr< void >
    create_shared_from_tag( poly_tag_type tag, ibstream& is ) const = 0;

    template< class T >
    inline T*
    create_raw( poly_tag_type tag, ibstream& is ) const
    {
        if ( can_downcast_ptr( tag, get_type_tag< T >() ) )
        {
            return static_cast< T* >( create_raw_from_tag( tag, is ) );
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::invalid_ptr_downcast ) };
        }
    }

    template< class T >
    inline std::shared_ptr< T >
    create_shared( poly_tag_type tag, ibstream& is ) const
    {
        if ( can_downcast_ptr( tag, get_type_tag< T >() ) )
        {
            return std::static_pointer_cast< T >( create_shared_from_tag( tag, is ) );
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::invalid_ptr_downcast ) };
        }
    }

    inline bool
    dedup_shared_ptrs() const
    {
        return m_dedup_shared_ptrs;
    }

    inline boost::endian::order
    byte_order() const
    {
        return m_byte_order;
    }

private:
    bool                        m_dedup_shared_ptrs;
    boost::endian::order        m_byte_order;
};

using poly_raw_factory_func = std::function< void* ( ibstream& ) >;

using poly_shared_factory_func = std::function< std::shared_ptr< void > ( ibstream& ) >;

template< class T1, class T2, class Enable = void >
struct can_downcast_ptr : public std::false_type {};

template< class T1, class T2 >
struct can_downcast_ptr< T1, T2, std::enable_if_t< std::is_base_of< T2, T1 >::value > > : public std::true_type {};

template< class T, class... Args >
struct can_downcast_ptr_row
{
    static const std::vector< bool > row_values;
};

template< class T, class... Args >
const std::vector< bool > can_downcast_ptr_row< T, Args... >::row_values = { can_downcast_ptr< T, Args >::value... };

template< class... Args >
struct can_downcast_ptr_table
{
    static const std::vector< std::vector< bool > > values;
};

template< class... Args >
const std::vector< std::vector< bool > > can_downcast_ptr_table< Args... >::values = 
{ 
    { can_downcast_ptr_row< Args, Args... >::row_values }... 
};

template< class... Args >
class context_impl : public context_impl_base
{
public:

    context_impl( bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    context_impl_base{ dedup_shared_ptrs, byte_order }
    {}

    context_impl( error_category_context::category_init_list categories, bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    context_impl_base{ categories, dedup_shared_ptrs, byte_order }
    {}

    virtual poly_tag_type 
    get_type_tag( std::type_index index ) const override
    {
        auto it = m_type_tag_map.find( index );
        if ( it != m_type_tag_map.end() )
        {
            return it->second;
        }
        else
        {
            return invalid_tag;
        }
    }

    virtual bool 
    can_downcast_ptr( poly_tag_type from, poly_tag_type to ) const override
    {
        return m_downcast_ptr_table.values[ from ][ to ];
    }

    virtual void*
    create_raw_from_tag( poly_tag_type tag, ibstream& is ) const override
    {
        return m_factories[ tag ]( is );
    }

    virtual std::shared_ptr< void >
    create_shared_from_tag( poly_tag_type tag, ibstream& is ) const override
    {
        return m_shared_factories[ tag ]( is );
    }

protected:
    static const std::unordered_map< std::type_index, poly_tag_type >   m_type_tag_map;
    static const can_downcast_ptr_table< Args... >                      m_downcast_ptr_table;
    static const std::vector< poly_raw_factory_func >                   m_factories;
    static const std::vector< poly_shared_factory_func >                m_shared_factories;
};

template< class... Args >
const std::unordered_map< std::type_index, poly_tag_type > context_impl< Args... >::m_type_tag_map = 
{ 
    { typeid(Args), utils::index< Args, Args... >::value }... 
};

template< class... Args >
const can_downcast_ptr_table< Args... > context_impl< Args... >::m_downcast_ptr_table;

template< class T, class Enable = void >
struct poly_factory;

template< class T >
struct poly_factory<T, std::enable_if_t< std::is_abstract< T >::value || ! has_ptr_deserializer< T >::value > >
{
    inline void* 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static inline void*
    get( ibstream& )
    {
        return nullptr;
    }
};

template< class T >
struct poly_factory< T, std::enable_if_t< ! std::is_abstract< T >::value && has_ptr_deserializer< T >::value > >
{
    inline void* 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static inline void*
    get( ibstream& is )
    {
        return ptr_deserializer< T >::get( is );
    }
};

template< class... Args >
const std::vector< poly_raw_factory_func > context_impl< Args... >::m_factories = 
{
    []( ibstream& is ) 
    {
        return poly_factory< Args >::get( is );
    }...
};

template< class T, class Enable = void >
struct poly_shared_factory;

template< class T >
struct poly_shared_factory<T, std::enable_if_t< std::is_abstract< T >::value || ! has_shared_ptr_deserializer< T >::value > >
{
    inline std::shared_ptr< void > 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static inline std::shared_ptr< void >
    get( ibstream& )
    {
        return nullptr;
    }
};

template< class T >
struct poly_shared_factory< T, std::enable_if_t< ! std::is_abstract< T >::value && has_shared_ptr_deserializer< T >::value > >
{
    inline std::shared_ptr< void > 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static inline std::shared_ptr< void >
    get( ibstream& is )
    {
        return shared_ptr_deserializer< T >::get( is );
    }
};

template< class... Args >
const std::vector< poly_shared_factory_func > context_impl< Args... >::m_shared_factories =
{
    [] ( ibstream& is )
    {
        return poly_shared_factory< Args >::get( is );
    }...
};

class context_base 
{
public:

    virtual ~context_base() {}

    virtual std::shared_ptr< const context_impl_base >
    get_context_impl() const = 0;

};

template< class... Args >
class context : public context_base
{
public:

    context( bool dedup_shared_ptrs = true, boost::endian::order byte_order = boost::endian::order::big )
    :
    m_context_impl{ std::make_shared< const context_impl< Args... > >( dedup_shared_ptrs, byte_order ) }
    {}

    context( error_category_context::category_init_list categories, bool dedup_shared_ptrs = true, boost::endian::order byte_order = boost::endian::order::big )
    :
    m_context_impl{ std::make_shared< const context_impl< Args... > >( categories, dedup_shared_ptrs, byte_order ) }
    {}
    
    virtual std::shared_ptr< const context_impl_base >
    get_context_impl() const override
    {
        return m_context_impl;
    }

private:
    std::shared_ptr< const context_impl_base >   m_context_impl;
};


inline context_base const& get_default_context()
{
    static const context<> default_context{ {} };
    return default_context;
}

/*! \class ibstream
    *	\brief binary input stream
    *	
    *	An instance of ibstream is associated with a read-only buffer. The caller 
    *	can read from the stream in a variety of ways, depending on the calling 
    *	context and the type being read. At present, ibstream doesn't explicity
    *	support run-time polymorphism. The reading context is assumed to know
    *	\a a \a priori the contents of buffer as streamed by the sender (that is,
    *	the types, number, and order of the items).
    */
class ibstream : public inumstream
{
public:
    using base = inumstream;

    template<class U, class E> friend struct value_deserializer;

    using saved_ptr_info = std::pair< std::type_index , std::shared_ptr< void > >;

    class ptr_deduper
    {
    public:

        template< class T >
        inline void
        save_ptr( std::shared_ptr< T > ptr )
        {
            m_saved_ptrs.push_back( saved_ptr_info( typeid( *ptr ), ptr ) );
        }

        inline saved_ptr_info const&
        get_saved_ptr( std::size_t index )
        {
            return m_saved_ptrs[ index ];
        }

        inline void
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

    inline
    ibstream( std::unique_ptr< bstream::ibstreambuf > strmbuf, context_base const& cntxt = get_default_context() )
    : 
    inumstream{ std::move( strmbuf ), cntxt.get_context_impl()->byte_order() },
    m_context{ cntxt.get_context_impl() },
    m_ptr_deduper{ m_context->dedup_shared_ptrs() ? std::make_unique< ptr_deduper >() : nullptr }
    {}

    template<class T>
    inline typename std::enable_if_t<is_ibstream_constructible<T>::value, T>
    read_as()
    {
        return  T(*this);
    }

    template<class T>
    inline typename std::enable_if_t<
        is_ibstream_constructible<T>::value 
        && std::is_default_constructible<T>::value, T>
    read_as( std::error_code& ec )
    {
        clear_error( ec );
        try 
        {
            return  T(*this);
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
            return T{};
        }
    }

    template<class T>
    inline typename std::enable_if_t<use_value_deserializer<T>::value, T>
    read_as()
    {
        return value_deserializer<T>::get(*this);
    }

    template<class T>
    inline typename std::enable_if_t<use_value_deserializer<T>::value
        && std::is_default_constructible<T>::value, T>
    read_as( std::error_code& ec )
    {
        clear_error( ec );
        try
        {
            return value_deserializer<T>::get(*this);
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
            return T{};
        }
    }

    template<class T>
    inline typename std::enable_if_t<use_ref_deserializer<T>::value, T>
    read_as()
    {
        T obj;
        ref_deserializer<T>::get(*this, obj);
        return obj;
    }

    template<class T>
    inline typename std::enable_if_t<use_ref_deserializer<T>::value, T>
    read_as( std::error_code& ec )
    {
        clear_error( ec );
        T obj;
        try
        {
            ref_deserializer<T>::get(*this, obj);
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return obj;
    }

    template< class T >
    inline typename std::enable_if_t< has_ref_deserializer< T >::value, ibstream& >
    read_as( T& obj )
    {
        return ref_deserializer< T >::get( *this, obj );
    }

    template< class T >
    inline typename std::enable_if_t< has_ref_deserializer< T >::value, ibstream& >
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

    template<class T>
    inline typename std::enable_if_t<
        !has_ref_deserializer<T>::value &&
        is_ibstream_constructible<T>::value &&
        std::is_assignable<T&,T>::value,
        ibstream&> 
    read_as( T& obj )
    {
        obj = T( *this );
        return *this;
    }

    template<class T>
    inline typename std::enable_if_t<
        !has_ref_deserializer<T>::value &&
        is_ibstream_constructible<T>::value &&
        std::is_assignable<T&,T>::value,
        ibstream&> 
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

    template<class T>
    inline typename std::enable_if_t<
        !has_ref_deserializer<T>::value &&
        !is_ibstream_constructible<T>::value &&
        has_value_deserializer<T>::value &&
        std::is_assignable<T&,T>::value,
        ibstream&>
    read_as( T& obj )
    {
        obj = value_deserializer<T>::get(*this);
        return *this;
    }

    template<class T>
    inline typename std::enable_if_t<
        !has_ref_deserializer<T>::value &&
        !is_ibstream_constructible<T>::value &&
        has_value_deserializer<T>::value &&
        std::is_assignable<T&,T>::value,
        ibstream&>
    read_as( T& obj, std::error_code& ec )
    {
        clear_error( ec );
        try
        {
            obj = value_deserializer<T>::get(*this);
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return *this;
    }

    inline std::size_t
    read_string_header()
    {
        std::size_t length = 0ul;
        auto tcode = base::get();
        if (tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max)
        {
            std::uint8_t mask = 0x1f;
            length = tcode & mask;
        }
        else
        {
            switch(tcode)
            {
                case typecode::str_8:
                {
                    length = base::get_num<std::uint8_t>();
                }
                break;
                case typecode::str_16:
                {
                    length = base::get_num<std::uint16_t>();
                }
                break;
                case typecode::str_32:
                {
                    length = base::get_num<std::uint32_t>();
                }
                break;
                default:
                {
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                }
            }
        }
        return length;
    }

    inline std::size_t
    read_string_header( std::error_code& ec )
    {
        clear_error( ec );
        std::size_t result = 0ul;
        try
        {
            result = read_string_header();
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return result;
    }

    inline std::size_t 
    read_array_header()
    {
        std::size_t length = 0;
        auto tcode = base::get();
        if (tcode >= typecode::fixarray_min && tcode <= typecode::fixarray_max)
        {
            std::uint8_t mask = 0x0f;
            length = tcode & mask;
        }
        else
        {
            switch (tcode)
            {
                case typecode::array_16:
                {
                    length = base::get_num<std::uint16_t>();
                }
                break;
                case typecode::array_32:
                {
                    length = base::get_num<std::uint32_t>();
                }
                break;
                default:
                {
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                }
            }
        }
        return length;
    }
    
    inline std::size_t 
    read_array_header( std::error_code& ec )
    {
        clear_error( ec );
        std::size_t result = 0;
        try
        {
            result = read_array_header();
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return result;
    }

    inline std::size_t
    read_map_header()
    {
        std::size_t length = 0;
        auto tcode = base::get();
        if (tcode >= typecode::fixmap_min && tcode <= typecode::fixarray_max)
        {
            std::uint8_t mask = 0x0f;
            length = tcode & mask;
        }
        else
        {
            switch (tcode)
            {
            case typecode::map_16:
                {
                    length = base::get_num<std::uint16_t>();
                }
                break;
                case typecode::map_32:
                {
                    length = base::get_num<std::uint32_t>();
                }
                break;
                default:
                {
                    throw std::system_error{ make_error_code( bstream::errc::type_error ) };
                }
            }
        }
        return length;
    }

    inline std::size_t
    read_map_header( std::error_code& ec )
    {
        clear_error( ec );
        std::size_t result = 0;
        try
        {
            result = read_map_header();
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return result;
    }

    inline ibstream&
    check_map_key(std::string const& key)
    {
        auto name = read_as<std::string>();
        if (name != key)
        {				
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
        return *this;
    }

    inline ibstream&
    check_map_key(std::string const& key, std::error_code& ec )
    {
        clear_error( ec );
        auto name = read_as<std::string>();
        if (name != key)
        {				
            ec = make_error_code( bstream::errc::type_error );
        }
        return *this;
    }

    inline ibstream&
    check_array_header(std::size_t expected)
    {
        auto actual = read_array_header();
        if (actual != expected)
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
        return *this;
    }

    inline ibstream&
    check_array_header(std::size_t expected, std::error_code& ec )
    {
        clear_error( ec );
        auto actual = read_array_header();
        if (actual != expected)
        {
            ec = make_error_code( bstream::errc::type_error );
        }
        return *this;
    }

    inline ibstream&
    check_map_header(std::size_t expected)
    {
        auto actual = read_map_header();
        if (actual != expected)
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
        return *this;
    }

    inline ibstream&
    check_map_header(std::size_t expected, std::error_code& ec )
    {
        clear_error( ec );
        auto actual = read_map_header();
        if (actual != expected)
        {
            ec = make_error_code( bstream::errc::type_error );
        }
        return *this;
    }

    inline std::size_t 
    read_blob_header()
    {
        std::size_t length = 0;
        auto tcode = base::get();
        switch (tcode)
        {
            case typecode::bin_8:
            {
                length = base::get_num<std::uint8_t>();
            }
            break;
            case typecode::bin_16:
            {
                length = base::get_num<std::uint16_t>();
            }
            break;
            case typecode::bin_32:
            {
                length = base::get_num<std::uint32_t>();
            }
            break;
            default:
            {
                throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
        return length;
    }

    inline std::size_t 
    read_blob_header( std::error_code& ec )
    {
        clear_error( ec );
        std::size_t result = 0;
        try
        {
            result = read_blob_header();
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return result;
    }

    nodeoze::buffer
    read_blob_body(std::size_t nbytes)
    {
        return getn( nbytes );
    }

    nodeoze::buffer
    read_blob_body(std::size_t nbytes, std::error_code& ec )
    {
        return getn( nbytes, ec );
    }

    nodeoze::buffer
    read_blob()
    {
        auto nbytes = read_blob_header();
        return read_blob_body(nbytes);
    }

    nodeoze::buffer
    read_blob( std::error_code& ec )
    {
        auto nbytes = read_blob_header( ec );
        if ( ec )
        {
            return buffer{};
        }
        else
        {
            return read_blob_body( nbytes, ec );
        }
    }

    inline std::size_t
    read_ext_header( std::uint8_t& ext_type )
    {
        std::size_t length = 0;
        auto tcode = base::get();
        switch (tcode)
        {
            case typecode::fixext_1:
            {
                length = 1;
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            case typecode::fixext_2:
            {
                length = 2;
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            case typecode::fixext_4:
            {
                length = 4;
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            case typecode::fixext_8:
            {
                length = 8;
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            case typecode::fixext_16:
            {
                length = 16;
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            case typecode::ext_8:
            {
                length = base::get_num< std::uint8_t >();
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            case typecode::ext_16:
            {
                length = base::get_num<std::uint16_t>();
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            case typecode::ext_32:
            {
                length = base::get_num<std::uint32_t>();
                ext_type = base::get_num< std::uint8_t >();
            }
            break;
            default:
            {
                throw std::system_error{ make_error_code( bstream::errc::type_error ) };
            }
        }
        return length;
    }

    inline std::size_t
    read_ext_header( std::uint8_t& ext_type, std::error_code& ec )
    {
        clear_error( ec );
        std::size_t result = 0;
        try
        {
            result = read_ext_header( ext_type );
        }
        catch ( std::system_error const& e )
        {
            ec = e.code();
        }
        return result;
    }

    nodeoze::buffer
    read_ext_body(std::size_t nbytes)
    {
        return getn( nbytes );
    }

    nodeoze::buffer
    read_ext_body(std::size_t nbytes, std::error_code& ec )
    {
        return getn( nbytes, ec );
    }

    void
    read_nil()
    {
        auto tcode = base::get();
        if (tcode != typecode::nil)
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
    }

    void
    read_nil( std::error_code& ec )
    {
        clear_error( ec );
        auto tcode = base::get();
        if (tcode != typecode::nil)
        {
            ec = make_error_code( bstream::errc::type_error );
        }
    }

    virtual void
    reset()
    {
        if ( m_ptr_deduper ) m_ptr_deduper->clear();
        position( 0 );
    }

    virtual void
    reset( std::error_code& err )
    {
        if ( m_ptr_deduper ) m_ptr_deduper->clear();
        position( 0, err );
    }

    virtual void 
    rewind()
    {
        position( 0 );
    }

    virtual void 
    rewind( std::error_code& err )
    {
        position( 0, err );
    }

    buffer
    get_msgpack_obj_buf();

    inline buffer
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
     *  Pointers are streamed as a 2 - element object (array or map).
     *  The first element is the type tag (int). If the tag is -1,
     *  then there is no run-time type information in the stream;
     *  the object should be constructed based on the assumption that
     *  it is an instance of T. If it is not -1, then it is interpreted
     *  as a type tag, and the ibstream instance is expected to have
     *  a poly_context that can create an instance from this tag.
     * 
     *  The second element is either a nil, a positive integer value, 
     *  or a serialized object (array or map). If it is nil, a nullptr
     *  value is returned. 
     *  If it is an integer,
     *  it is interpreted as an id associated with a previously-
     *  stream object, which is expected to be stored in the stream 
     *  graph context.
     *  If it is an object, an instance of the appropriate type
     *  (as indicated by the parameter T and/or the tag value as
     *  interpreted by the poly_contexts) is constructed from the 
     *  streamed object, cast to the return type (possibly mediated 
     *  by the poly_context), and returned.
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
    
    template<class T>
    std::unique_ptr<T>
    read_as_unique_ptr()
    {
        auto n = read_array_header();
        if ( n != 2 )
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
        
        auto tag = read_as< poly_tag_type >();

        auto code = peek();
        
        if (code == typecode::nil) // nullptr
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
    deserialize_as_unique_ptr( poly_tag_type tag )
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
    read_error_code()
    {
        auto n = read_array_header();
        if ( n != 2 )
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
        auto category_index = read_as< error_category_context::index_type >();
        auto value = read_as< error_category_context::index_type >();
        return std::error_code{ value, m_context->category_from_index( category_index ) };
    }

    
    void 
    ingest( bufwriter& os );

    std::unique_ptr< bufwriter >                    m_bufwriter = nullptr;
    std::shared_ptr< const context_impl_base >      m_context;
    std::unique_ptr< ptr_deduper >                  m_ptr_deduper;
};
	        
template<class T>
struct value_deserializer<T, std::enable_if_t<std::is_enum<T>::value>>
{
    inline T 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static T get(ibstream& is)
    {
        auto ut = is.read_as<typename std::underlying_type<T>::type>();
        return static_cast<T>(ut);
    }				
};

template<class T>
inline typename std::enable_if_t<has_ref_deserializer<T>::value, ibstream&> 
operator>>(ibstream& is, T& obj)
{
    return ref_deserializer<T>::get(is, obj);
}

template<class T>
inline typename std::enable_if_t<
    !has_ref_deserializer<T>::value &&
    is_ibstream_constructible<T>::value &&
    std::is_assignable<T&,T>::value,
    ibstream&> 
operator>>(ibstream& is, T& obj)
{
    obj = T(is);
    return is;
}

template<class T>
inline typename std::enable_if_t<
    !has_ref_deserializer<T>::value &&
    !is_ibstream_constructible<T>::value &&
    has_value_deserializer<T>::value &&
    std::is_assignable<T&,T>::value,
    ibstream&>
operator>>(ibstream& is, T& obj)
{
    obj = value_deserializer<T>::get(is);
    return is;
}

template<class T>
struct ref_deserializer<T, std::enable_if_t<has_deserialize_method<T>::value>>
{

    inline ibstream& 
    operator()(ibstream& is, T& obj) const
    {
        return get(is, obj);
    }

    static inline ibstream&
    get(ibstream& is, T& obj)
    {
        return obj.deserialize(is);
    }
};

template<class T>
struct value_deserializer<T, std::enable_if_t<is_ibstream_constructible<T>::value>>
{
    inline T 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline T get(ibstream& is)
    {
        return T{is};
    }
};

template<>
struct value_deserializer<bool>
{
    inline bool 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static bool get(ibstream& is)
    {
        auto tcode = is.get();
        switch(tcode)
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
struct value_deserializer<float>
{
    inline float 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static float get(ibstream& is)
    {
        auto tcode = is.get();
        if (tcode == typecode::float_32)
        {
            std::uint32_t unpacked = is.get_num<std::uint32_t>();
            return reinterpret_cast<float&>(unpacked);
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
    }
};

template<>
struct value_deserializer<double>
{
    inline double 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static double get(ibstream& is)
    {
        auto tcode = is.get();
        if (tcode == typecode::float_32)
        {
            std::uint32_t unpacked = is.get_num<std::uint32_t>();
            return static_cast<double>(reinterpret_cast<float&>(unpacked));
        }
        else if (tcode == typecode::float_64)
        {
            std::uint64_t unpacked = is.get_num<std::uint64_t>();
            return reinterpret_cast<double&>(unpacked);
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::type_error ) };
        }
    }
};

template<class Rep, class Ratio>
struct value_deserializer<std::chrono::duration<Rep,Ratio>>
{
    using duration_type = std::chrono::duration<Rep,Ratio>;

    inline duration_type 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static duration_type
    get(ibstream&is)
    {
        auto count = is.read_as<duration_type::rep>();
        return duration_type{count};
    }
};

template<class Clock, class Duration>
struct value_deserializer<std::chrono::time_point<Clock,Duration>>
{
    using time_point_type = std::chrono::time_point<Clock,Duration>;

    inline time_point_type 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static time_point_type
    get(ibstream& is)
    {
        auto ticks = is.read_as<typename time_point_type::rep>();
        return time_point_type(typename time_point_type::duration(ticks));
    }
};

/*
 *	Value deserializers for shared pointer types
 */

/*
 *	Prefer stream-constructed when available
 */

template<class T>
struct value_deserializer< std::shared_ptr< T > >
{
    inline std::shared_ptr< T > 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::shared_ptr<T>
    get(ibstream& is)
    {
        return is.read_as_shared_ptr< T >();
    }
};

template< class T >
struct ptr_deserializer< T, typename std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    inline T*
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    inline static T*
    get( ibstream& is )
    {
        return new T{ is };
    }
};

template< class T >
struct shared_ptr_deserializer< T, typename std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    inline std::shared_ptr< T >
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    inline static std::shared_ptr< T >
    get( ibstream& is )
    {
        return std::make_shared< T >( is );
    }
};

template< class T >
struct unique_ptr_deserializer< T, typename std::enable_if_t< is_ibstream_constructible< T >::value > >
{
    inline std::unique_ptr< T >
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    inline static std::unique_ptr< T >
    get( ibstream& is )
    {
        return std::make_unique< T >( is );
    }
};

/*
 *	If no stream constructor is available, prefer a value_deserializer<T>
 * 
 *	The value_deserializer<shared_ptr<T>> based on a value_deserializer<T> 
 *	requires a move contructor for T
 */

template< class T >
struct ptr_deserializer< T,
    typename std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        has_value_deserializer< T >::value &&
        std::is_move_constructible< T >::value > >
{
    inline T* 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static T*
    get(ibstream& is)
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
    inline std::shared_ptr< T >
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::shared_ptr< T >
    get(ibstream& is)
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
    inline std::unique_ptr< T >
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::unique_ptr< T >
    get(ibstream& is)
    {
        return std::make_unique< T >( value_deserializer< T >::get( is ) );
    }
};

/*
    *	The value_deserializer<shared_ptr<T>> based on a ref_deserializer<T> 
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
    inline T* 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static T*
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
    inline std::shared_ptr< T >
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::shared_ptr< T >
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
    inline std::unique_ptr< T > 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::unique_ptr< T >
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

template<class T>
struct value_deserializer< std::unique_ptr< T > >
{
    inline std::unique_ptr< T > 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::unique_ptr<T>
    get(ibstream& is)
    {
        return is.read_as_unique_ptr<T>();
    }
};



/*
 *	Prefer stream-constructed when available
 */
/*
 *	If no stream constructor is available, prefer a value_deserializer<T>
 * 
 *	The value_deserializer<unique_ptr<T>> based on a value_deserializer<T> 
 *	requires a move contructor for T
 */

/*
 *	The value_deserializer<unique_ptr<T>> based on a ref_deserializer<T> 
 *	requires either a move contructor or copy constructor for T;
 *	prefer the move contructor
 */

template<class T, class Enable = void> 
struct ibstream_initializer;

template<class T, class Alloc>
struct value_deserializer<std::vector<T, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::vector<T, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::vector<T, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::vector<T, Alloc> result;
        result.reserve(length);
        for (auto i = 0u; i < length; ++i)
        {
            result.emplace_back(ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class T, class Alloc>
struct value_deserializer<std::list<T, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::list<T, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::list<T, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::list<T, Alloc> result;
        for (auto i = 0u; i < length; ++i)
        {
            result.emplace_back(ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class T, class Alloc>
struct value_deserializer<std::forward_list<T, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::forward_list<T, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::forward_list<T, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::forward_list<T, Alloc> result;
        auto it = result.before_begin();
        for (auto i = 0u; i < length; ++i)
        {
            it = result.emplace_after(it, ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class T, class Alloc>
struct value_deserializer<std::deque<T, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::deque<T, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::deque<T, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::deque<T, Alloc> result;
        for (auto i = 0u; i < length; ++i)
        {
            result.emplace_back(ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class T, class Compare, class Alloc>
struct value_deserializer<std::set<T, Compare, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::set<T, Compare, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::set<T, Compare, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::set<T, Compare, Alloc> result;
        for (auto i = 0u; i < length; ++i)
        {
            result.emplace(ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class T, class Compare, class Alloc>
struct value_deserializer<std::multiset<T, Compare, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::multiset<T, Compare, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::multiset<T, Compare, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::multiset<T, Compare, Alloc> result;
        for (auto i = 0u; i < length; ++i)
        {
            result.emplace(ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class T, class Hash, class Equal, class Alloc>
struct value_deserializer<std::unordered_set<T, Hash, Equal, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::unordered_set<T, Hash, Equal, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::unordered_set<T, Hash, Equal, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::unordered_set<T, Hash, Equal, Alloc> result;
        for (auto i = 0u; i < length; ++i)
        {
            result.emplace(ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class T, class Hash, class Equal, class Alloc>
struct value_deserializer<std::unordered_multiset<T, Hash, Equal, Alloc>,
        typename std::enable_if_t<is_ibstream_readable<T>::value>>
{
    inline std::unordered_multiset<T, Hash, Equal, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    inline static std::unordered_multiset<T, Hash, Equal, Alloc> 
    get(ibstream& is)
    {
        auto length = is.read_array_header();
        std::unordered_multiset<T, Hash, Equal, Alloc> result;
        for (auto i = 0u; i < length; ++i)
        {
            result.emplace(ibstream_initializer<T>::get(is));
        }
        return result;
    }
};	

template<class K, class V, class Hash, class Equal, class Alloc>
struct value_deserializer<std::unordered_map<K, V, Hash, Equal, Alloc>,
        typename std::enable_if_t<
            is_ibstream_readable<K>::value &&
            is_ibstream_readable<V>::value>>
{
    inline std::unordered_map<K, V, Hash, Equal, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::unordered_map<K, V, Hash, Equal, Alloc>
    get(ibstream& is)
    {
        using pair_type = std::pair<K,V>;
        using map_type = std::unordered_map<K, V, Hash, Equal, Alloc>;
        
        auto length = is.read_array_header();
        map_type result;
        result.reserve(length);
        for (auto i = 0u; i < length; ++i)
        {
            result.insert(is.read_as<pair_type>());
        }
        return result;
    }
};

template<class K, class V, class Hash, class Equal, class Alloc>
struct value_deserializer<std::unordered_multimap<K, V, Hash, Equal, Alloc>,
        typename std::enable_if_t<
            is_ibstream_readable<K>::value &&
            is_ibstream_readable<V>::value>>
{
    inline std::unordered_multimap<K, V, Hash, Equal, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::unordered_multimap<K, V, Hash, Equal, Alloc>
    get(ibstream& is)
    {
        using pair_type = std::pair<K,V>;
        using map_type = std::unordered_multimap<K, V, Hash, Equal, Alloc>;
        
        auto length = is.read_array_header();
        map_type result;
        result.reserve(length);
        for (auto i = 0u; i < length; ++i)
        {
            result.insert(is.read_as<pair_type>());
        }
        return result;
    }
};

	
/*
 *	value_deserializers for map and multmap
 */

template<class K, class V, class Compare, class Alloc>
struct value_deserializer<std::map<K, V, Compare, Alloc>,
        typename std::enable_if_t<
            is_ibstream_readable<K>::value &&
            is_ibstream_readable<V>::value>>
{
    inline std::map<K, V, Compare, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::map<K, V, Compare, Alloc>
    get(ibstream& is)
    {
        using pair_type = std::pair<K, V>;
        using map_type = std::map<K, V, Compare, Alloc>;
        auto length = is.read_array_header();
        map_type result;
        for (auto i = 0u; i < length; ++i)
        {
            result.insert(is.read_as<pair_type>());
        }
        return result;
    }
};

template<class K, class V, class Compare, class Alloc>
struct value_deserializer<std::multimap<K, V, Compare, Alloc>,
        typename std::enable_if_t<
            is_ibstream_readable<K>::value &&
            is_ibstream_readable<V>::value>>
{
    inline std::multimap<K, V, Compare, Alloc> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::multimap<K, V, Compare, Alloc>
    get(ibstream& is)
    {
        using pair_type = std::pair<K, V>;
        using map_type = std::multimap<K, V, Compare, Alloc>;
        auto length = is.read_array_header();
        map_type result;
        for (auto i = 0u; i < length; ++i)
        {
            result.insert(is.read_as<pair_type>());
        }
        return result;
    }
};

template<class... Args>
struct value_deserializer<std::tuple<Args...>,
        std::enable_if_t<utils::conjunction<is_ibstream_readable<Args>::value...>::value>>
{
    using tuple_type = std::tuple<Args...>;
    
    inline tuple_type 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline tuple_type
    get(ibstream& is)
    {
        is.check_array_header(std::tuple_size<tuple_type>::value);
        tuple_type tup;
        get_members<0, Args...>(is, tup);
        return tup;
    }
    
    
    template<unsigned int N, class First, class... Rest>
    static inline 
    typename std::enable_if<(sizeof...(Rest) > 0)>::type
    get_members(ibstream& is, tuple_type& tup)
    {
        is >> std::get<N>(tup);
        get_members<N+1, Rest...>(is, tup);
    }
    
    template <unsigned int N, class T>
    static inline void
    get_members(ibstream& is, tuple_type& tup)
    {
        is >> std::get<N>(tup);
    }
};

template<class T1, class T2>
struct value_deserializer<std::pair<T1, T2>,
        typename std::enable_if_t<
            std::is_move_constructible<T1>::value &&
            std::is_move_constructible<T2>::value>>
{
    inline std::pair<T1, T2> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::pair<T1, T2>
    get(ibstream& is)
    {
        is.check_array_header(2);
        T1 t1{ibstream_initializer<T1>::get(is)};
        T2 t2{ibstream_initializer<T2>::get(is)};
        return std::make_pair(std::move(t1), std::move(t2));
    }
};

template<class T1, class T2>
struct value_deserializer<std::pair<T1, T2>,
        typename std::enable_if_t<
            (!std::is_move_constructible<T1>::value && std::is_copy_constructible<T1>::value) &&
            std::is_move_constructible<T2>::value>>
{
    inline std::pair<T1, T2> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::pair<T1, T2>
    get(ibstream& is)
    {
        is.check_array_header(2);
        T1 t1{ibstream_initializer<T1>::get(is)};
        T2 t2{ibstream_initializer<T2>::get(is)};
        return std::make_pair(t1, std::move(t2));
    }
};

template<class T1, class T2>
struct value_deserializer<std::pair<T1, T2>,
        typename std::enable_if_t<
            std::is_move_constructible<T1>::value &&
            (!std::is_move_constructible<T2>::value && std::is_copy_constructible<T2>::value)>>
{
    inline std::pair<T1, T2> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::pair<T1, T2>
    get(ibstream& is)
    {
        is.check_array_header(2);
        T1 t1{ibstream_initializer<T1>::get(is)};
        T2 t2{ibstream_initializer<T2>::get(is)};
        return std::make_pair(std::move(t1), t2);
    }
};

template<class T1, class T2>
struct value_deserializer<std::pair<T1, T2>,
        typename std::enable_if_t<
            (!std::is_move_constructible<T1>::value && std::is_copy_constructible<T1>::value) &&
            (!std::is_move_constructible<T2>::value && std::is_copy_constructible<T2>::value)>>
{
    inline std::pair<T1, T2> 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline std::pair<T1, T2>
    get(ibstream& is)
    {
        is.check_array_header(2);
        T1 t1{ibstream_initializer<T1>::get(is)};
        T2 t2{ibstream_initializer<T2>::get(is)};
        return std::make_pair(t1, t2);
    }
};

template<>
struct value_deserializer< nodeoze::buffer >
{
    inline nodeoze::buffer 
    operator()( ibstream& is )  const
    {
        return get(is);
    }

    static inline nodeoze::buffer
    get( ibstream& is )
    {
        return is.read_blob();
    }
};

template<>
struct value_deserializer< std::error_code >
{
    inline std::error_code
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static inline std::error_code
    get( ibstream& is )
    {
        return is.read_error_code();
    }
};

template<class T>
struct ibstream_initializer<T, 
    typename std::enable_if_t<is_ibstream_constructible<T>::value>>
{
    using param_type = ibstream&;
    using return_type = ibstream&;
    inline static return_type get(param_type is)
    {
        return is;
    }
};

template<class T>
struct ibstream_initializer <T, 
    typename std::enable_if_t<
        !is_ibstream_constructible<T>::value &&
        has_value_deserializer<T>::value>>
{
    using param_type = ibstream&;
    using return_type = T;
    inline static return_type get(param_type is)
    {
        return value_deserializer<T>::get(is);
    }
};
    
template<class T>
struct ibstream_initializer <T, 
    typename std::enable_if_t<
        !is_ibstream_constructible<T>::value &&
        !has_value_deserializer<T>::value &&
        std::is_default_constructible<T>::value &&
        (std::is_copy_constructible<T>::value || std::is_move_constructible<T>::value) &&
        has_ref_deserializer<T>::value>>
{
    using param_type = ibstream&;
    using return_type = T;
    inline static return_type get(param_type is)
    {
        T obj;
        ref_deserializer<T>::get(is, obj);
        return obj;
    }
};

} // bstream
} // nodeoze

#endif /* BSTREAM_IBSTREAM_H */

