
#ifndef NODEOZE_BSTREAM_IBSTREAM_TRAITS_H
#define NODEOZE_BSTREAM_IBSTREAM_TRAITS_H

#include <type_traits>
#include <nodeoze/bstream/utils/traits.h>
#include <nodeoze/bstream/fwd_decls.h>

namespace nodeoze
{
namespace bstream
{

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

template< class T > 
struct is_ibstream_constructible< T, 
    typename std::enable_if_t< ibstream_ctor_detected< T >::value || has_streaming_base< T >::value > > 
    : public std::true_type {};

namespace detail
{
    template< class T >
    static auto test_ref_deserializer( int )
        -> utils::sfinae_true_if< decltype( ref_deserializer< T >::get( std::declval< ibstream& >(), std::declval< T& >() ) ) >;
    template< class >
    static auto test_ref_deserializer( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_ref_deserializer : decltype( detail::test_ref_deserializer< T >( 0 ) ) {};

namespace detail
{
    template< class T >
    static auto test_value_deserializer( int )
        -> utils::sfinae_true_if< decltype( value_deserializer< T >::get( std::declval< ibstream& >() ) ) >;
    template< class >
    static auto test_value_deserializer( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_value_deserializer : decltype( detail::test_value_deserializer< T >( 0 ) ) {};

namespace detail
{
    template< class T >
    static auto test_ptr_deserializer( int )
        -> utils::sfinae_true_if< decltype( ptr_deserializer< T >::get( std::declval< ibstream& >() ) ) >;
    template< class >
    static auto test_ptr_deserializer( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_ptr_deserializer : decltype( detail::test_ptr_deserializer< T >( 0 ) ) {};

namespace detail
{
    template< class T >
    static auto test_shared_ptr_deserializer( int )
        -> utils::sfinae_true_if< decltype( shared_ptr_deserializer< T >::get( std::declval< ibstream& >() ) ) >;
    template< class >
    static auto test_shared_ptr_deserializer( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_shared_ptr_deserializer : decltype( detail::test_shared_ptr_deserializer< T >( 0 ) ) {};

namespace detail
{
    template< class T >
    static auto test_deserialize_method( int )
        -> utils::sfinae_true_if< decltype( std::declval< T >().deserialize( std::declval< ibstream& >() ) ) >;
    template< class >
    static auto test_deserialize_method( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_deserialize_method : decltype( detail::test_deserialize_method< T >( 0 ) ) {};

namespace detail
{
    template< class T >
    static auto test_ibstream_extraction_operator( int )
        -> utils::sfinae_true_if< decltype( std::declval< ibstream& >() >> std::declval< T& >() ) >;
    template< class >
    static auto test_ibstream_extraction_operator( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_ibstream_extraction_operator : decltype( detail::test_ibstream_extraction_operator< T >( 0 ) ) {};

template< class T, class Enable = void >
struct is_ref_deserializable : public std::false_type {};

template< class T >
struct is_ref_deserializable< T, 
    std::enable_if_t<
        has_ref_deserializer< T >::value ||
        ( has_value_deserializer< T >::value && std::is_assignable< T&,T >::value ) ||
        ( is_ibstream_constructible< T >::value && std::is_assignable< T&,T >::value ) > >
: public std::true_type {};

template< class T, class Enable = void >
struct is_value_deserializable : public std::false_type {};

template< class T >
struct is_value_deserializable< T, 
    std::enable_if_t<
        is_ibstream_constructible< T >::value ||
        has_value_deserializer< T >::value ||
        ( has_ref_deserializer< T >::value && std::is_default_constructible< T >::value ) > >
: public std::true_type {};

template< class T, class Enable = void >
struct is_ibstream_readable : public std::false_type {};

template< class T >
struct is_ibstream_readable< T, 
    std::enable_if_t<
        is_value_deserializable< T >::value ||
        is_ref_deserializable< T >::value > >
: public std::true_type {};


template< class T, class Enable = void >
struct use_value_deserializer : public std::false_type {};

template< class T >
struct use_value_deserializer< T,
    std::enable_if_t<
        !is_ibstream_constructible< T >::value && 
        has_value_deserializer< T >::value > >
: public std::true_type {};

template< class T, class Enable = void >
struct use_ref_deserializer : public std::false_type {};

template< class T >
struct use_ref_deserializer< T,
    std::enable_if_t<
        !is_ibstream_constructible< T >::value &&
        !has_value_deserializer< T >::value &&
        std::is_default_constructible< T >::value &&
        has_ref_deserializer< T >::value > >
: public std::true_type {};

} // namespace bstream
} // namespace nodeoze 

#endif // NODEOZE_BSTREAM_IBSTREAM_TRAITS_H
