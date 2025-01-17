/* 
 * File:   msgpack.h
 * Author: David Curtis
 *
 * Created on February 5, 2018, 9:14 AM
 */

#ifndef NODEOZE_BSTREAM_MSGPACK_H
#define NODEOZE_BSTREAM_MSGPACK_H

#include <msgpack.hpp>
#include <type_traits>
#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>

#define BSTRM_HAS_MSGPACK_CVT_ADAPTOR( type_name )												\
	template<>																					\
	struct nodeoze::bstream::has_msgpack_cvt_adaptor< type_name > : public std::true_type {}	\
/**/

#define BSTRM_HAS_MSGPACK_AS_ADAPTOR( type_name )												\
	template<>																					\
	struct nodeoze::bstream::has_msgpack_as_adaptor< type_name > : public std::true_type {}		\
/**/
	
#define BSTRM_HAS_MSGPACK_PACK_ADAPTOR( type_name )												\
	template<>																					\
	struct nodeoze::bstream::has_msgpack_pack_adaptor< type_name > : public std::true_type {}	\
/**/
	
namespace nodeoze
{
namespace bstream
{
	template< class T >
	struct has_msgpack_cvt_adaptor : public std::false_type {};

	template< class T >
	struct has_msgpack_as_adaptor : public std::false_type {};

	template< class T >
	struct has_msgpack_pack_adaptor : public std::false_type {};

    template< class T > 
    struct is_msgpack_object_constructible : public std::is_constructible< T, msgpack::object const& > {};

	namespace detail
	{
		template< class T >
		static auto test_unpack_method( int )
			-> utils::sfinae_true_if< decltype( std::declval< T >().msgpack_unpack( std::declval< msgpack::object >() ) ) >;
		template< class >
		static auto test_unpack_method( long ) -> std::false_type;
	} // namespace detail

	template< class T >
	struct has_msgpack_unpack_method : decltype( detail::test_unpack_method< T >( 0 ) ) {};
	
/*	maybe someday when msgpack SFINAE isn't broken
	template< class T >
	struct has_msgpack_convert_adaptor
	{
	private:
		template< class U >
		static auto check( U* ) ->
			typename std::is_same
			<
				decltype( 
					std::declval< msgpack::adaptor::convert< U > >()( std::declval< msgpack::object >(), std::declval< U& >() )
				),
				msgpack::object const&
			>::type;
		template< typename > static std::false_type check( ... );
	public:
		using type = decltype( check< T >( nullptr ) );
		static constexpr bool value = type::value;
	};
*/

	namespace detail
	{
		template< class T >
		static auto test_pack_method( int )
			-> utils::sfinae_true_if< decltype( std::declval< T >().msgpack_pack( std::declval< msgpack::packer< obstream >& >() ) ) >;
		template< class >
		static auto test_pack_method( long ) -> std::false_type;
	} // namespace detail

	template< class T >
	struct has_msgpack_pack_method : decltype( detail::test_pack_method< T >( 0 ) ) {};

	template< class T >
	struct value_deserializer< T, 
			typename std::enable_if_t<
				( std::is_copy_constructible< T >::value || std::is_move_constructible< T >::value ) &&
				is_msgpack_object_constructible< T >::value > >
	{
        static T get( ibstream& is )
		{
			msgpack::object_handle handle;
			buffer buf = is.get_msgpack_obj_buf();
			msgpack::unpack( handle, reinterpret_cast< const char* >( buf.data() ), buf.size() );
			return T( handle.get() );
		}
	};

	template< class T >
	struct value_deserializer< T, 
			typename std::enable_if_t<
			( std::is_copy_constructible< T >::value || std::is_move_constructible< T >::value ) &&
			!is_msgpack_object_constructible< T >::value &&
			has_msgpack_as_adaptor< T >::value > >
	{
        static T get( ibstream& is )
		{
			msgpack::object_handle handle;
			buffer buf = is.get_msgpack_obj_buf();
			msgpack::unpack( handle, reinterpret_cast< const char* >( buf.data() ), buf.size() );
			return msgpack::adaptor::as< T >{}( handle.get() );
		}
	};

	template< class T >
	struct ref_deserializer< T, 
			typename std::enable_if_t<
				!has_msgpack_as_adaptor< T >::value &&
				has_msgpack_unpack_method< T >::value > >
	{
		static ibstream&
		get( ibstream& is, T& obj )
		{
			msgpack::object_handle handle;
			buffer buf = is.get_msgpack_obj_buf();
			msgpack::unpack( handle, reinterpret_cast< const char* >( buf.data() ), buf.size() );
			obj.msgpack_unpack( handle.get() );
			return is;
		}
	};

	template< class T >
	struct ref_deserializer< T, 
			typename std::enable_if_t<
				!has_msgpack_unpack_method< T >::value &&
				has_msgpack_cvt_adaptor< T >::value > >
	{
		static ibstream&
		get( ibstream& is, T& obj )
		{
			msgpack::object_handle handle;
			buffer buf = is.get_msgpack_obj_buf();
			msgpack::unpack( handle, reinterpret_cast< const char* >( buf.data() ), buf.size() );
			msgpack::adaptor::convert< T >{}( handle.get(), obj );
			return is;
		}
	};
	
	template< class T >
	struct serializer< T, typename std::enable_if_t< has_msgpack_pack_method< T >::value > >
	{
		static obstream& put( obstream& os, T const& obj )
		{
			msgpack::packer< obstream > pkr{ os };
			obj.msgpack_pack( pkr );
			return os;
		}
	};
	
	template< class T >
	struct serializer< T, 
			typename std::enable_if_t<
				!has_msgpack_pack_method< T >::value && 
				has_msgpack_pack_adaptor< T >::value > >
	{
		static obstream& put( obstream& os, T const& obj )
		{
			msgpack::packer< obstream > pkr{ os };
			msgpack::adaptor::pack< T >{}( pkr, obj );
			return os;
		}
	};

	inline void 
	dump_json( std::ostream& os, buffer const& buf )
	{
		msgpack::object_handle oh;
		msgpack::unpack( oh, reinterpret_cast< const char* >( buf.data() ), buf.size() );
		os << oh.get() << std::endl;		
	}
	
	inline std::string 
	strdump_json( buffer const& buf )
	{
		msgpack::object_handle oh;
		msgpack::unpack( oh, reinterpret_cast< const char* >( buf.data() ), buf.size() );
		std::ostringstream oss;
		oss << oh.get() << std::endl;
		return oss.str();
	}

/*
	template< class Buffer >
	void dump_json( std::ostream& os, Buffer const& buf )
	{
		msgpack::object_handle handle;
		buffer buf = is.get_msgpack_obj_buf();
		msgpack::unpack( handle, reinterpret_cast< const char* >( buf.data() ), buf.size() );
		os << handle.get() << std::endl;		
	}
	
	template< class Buffer >
	std::string strdump_json( Buffer const& buf )
	{
		msgpack::object_handle handle;
		buffer buf = is.get_msgpack_obj_buf();
		msgpack::unpack( handle, reinterpret_cast< const char* >( buf.data() ), buf.size() );
		std::ostringstream oss;
		oss << handle.get() << std::endl;
		return oss.str();
	}
*/
	
} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_MSGPACK_H

