#ifndef NODEOZE_BSTREAM_OBSTREAM_TRAITS_H
#define NODEOZE_BSTREAM_OBSTREAM_TRAITS_H

#include <type_traits>
#include <nodeoze/bstream/utils/traits.h>
#include <nodeoze/bstream/fwd_decls.h>

namespace nodeoze
{
namespace bstream
{

namespace detail
{
	template< class T >
	static auto test_serialize_impl_method( int )
		-> utils::sfinae_true_if< decltype( std::declval< T >().serialize_impl( std::declval< obstream& >() ) ) >;
	template< class >
	static auto test_serialize_impl_method( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_serialize_impl_method : decltype( detail::test_serialize_impl_method< T >( 0 ) ) {};

namespace detail
{
	template< class T >
	static auto test_serialize_method( int )
		-> utils::sfinae_true_if< decltype( std::declval< T >().serialize( std::declval< obstream& >() ) ) >;
	template< class >
	static auto test_serialize_method( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_serialize_method : decltype( detail::test_serialize_method< T >( 0 ) ) {};

namespace detail
{
	template< class T >
	static auto test_serializer( int )
		-> utils::sfinae_true_if< decltype( serializer< T >::put( std::declval< obstream& >(), std::declval< T >() ) ) >;
	template< class >
	static auto test_serializer( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_serializer : decltype( detail::test_serializer< T >( 0 ) ) {};

namespace detail
{
	template< class T >
	static auto test_obstream_insertion_operator( int )
		-> utils::sfinae_true_if< decltype( std::declval< obstream& >() << std::declval< T >() ) >;
	template< class >
	static auto test_obstream_insertion_operator( long ) -> std::false_type;
} // namespace detail

template< class T >
struct has_obstream_insertion_operator : decltype( detail::test_obstream_insertion_operator< T >( 0 ) ) {};

template< class T, class Enable = void >
struct is_serializable : public std::false_type {};

template< class T >
struct is_serializable< T, std::enable_if_t< has_serializer< T >::value > > : public std::true_type {};

} // namespace bstream
} // namespace nodeoze 

#endif // NODEOZE_BSTREAM_OBSTREAM_TRAITS_H
