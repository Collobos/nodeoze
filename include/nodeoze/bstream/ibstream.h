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
#include <boost/endian/conversion.hpp>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/typecode.h>
#include <nodeoze/bstream/utils/io_buffers.h>
#include <nodeoze/bstream/utils/traits.h>

namespace nodeoze
{
namespace bstream
{
    class ibstream;
    
    template<class T> 
    struct is_ibstream_constructible : public std::is_constructible<T, ibstream&> {};
	
    template<class T, class Enable = void>
    struct value_deserializer;
    
    template<class T, class Enable = void>
    struct ref_deserializer;
	
	template<class T, class Enable = void>
    struct serializer;

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
	
	template<class T>
    struct value_deserializer<T, 
        std::enable_if_t<
            std::numeric_limits<T>::is_integer && 
            std::numeric_limits<T>::is_signed && 
            sizeof(T) == 1>>
    {
        inline static T get(utils::in_buffer& is)
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
                        throw type_error("invalid value for 8-bit signed integer");
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
        inline static T get(utils::in_buffer& is)
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
                        throw type_error("invalid value for 8-bit unsigned integer");
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
        inline static T get(utils::in_buffer& is)
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
                        return static_cast<T>(is.get_arithmetic_as<int8_t>());
                    case typecode::uint_8:
                        return static_cast<T>(is.get_arithmetic_as<uint8_t>());
                    case typecode::int_16:
                        return static_cast<T>(is.get_arithmetic_as<int16_t>());
                    case typecode::uint_16:
                    {
                        std::uint16_t n = is.get_arithmetic_as<uint16_t>();
                        if (n <= std::numeric_limits<T>::max())
                        {
                            return static_cast<T>(n);
                        }
                        else
                        {
                            throw type_error("invalid value for 16-bit signed integer");
                        }
                    }
                    default:
                        throw type_error("invalid value for 16-bit signed integer");
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
        inline static T get(utils::in_buffer& is)
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
                        return static_cast<T>(is.get_arithmetic_as<uint8_t>());
                    case typecode::uint_16:
                        return static_cast<T>(is.get_arithmetic_as<uint16_t>());
                    default:
                        throw type_error("invalid value for 16-bit unsigned integer");
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
        inline static T get(utils::in_buffer& is)
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
                        return static_cast<T>(is.get_arithmetic_as<int8_t>());
                    case typecode::uint_8:
                        return static_cast<T>(is.get_arithmetic_as<uint8_t>());
                    case typecode::int_16:
                        return static_cast<T>(is.get_arithmetic_as<int16_t>());
                    case typecode::uint_16:
                        return static_cast<T>(is.get_arithmetic_as<uint16_t>());
                    case typecode::int_32:
                        return static_cast<T>(is.get_arithmetic_as<int32_t>());
                    case typecode::uint_32:
                    {
                        std::uint32_t n = is.get_arithmetic_as<uint32_t>();
                        if (n <= std::numeric_limits<T>::max())
                        {
                            return static_cast<T>(n);
                        }
                        else
                        {
                            throw type_error("invalid value for 32-bit signed integer");
                        }
                    }
                    default:
                        throw type_error("invalid value for 32-bit signed integer");
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
        inline static T get(utils::in_buffer& is)
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
                        return static_cast<T>(is.get_arithmetic_as<uint8_t>());
                    case typecode::uint_16:
                        return static_cast<T>(is.get_arithmetic_as<uint16_t>());
                    case typecode::uint_32:
                        return static_cast<T>(is.get_arithmetic_as<uint32_t>());
                    default:
                        throw type_error("invalid value for 32-bit unsigned integer");
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
        inline static T get(utils::in_buffer& is)
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
                        return static_cast<T>(is.get_arithmetic_as<int8_t>());
                    case typecode::uint_8:
                        return static_cast<T>(is.get_arithmetic_as<uint8_t>());
                    case typecode::int_16:
                        return static_cast<T>(is.get_arithmetic_as<int16_t>());
                    case typecode::uint_16:
                        return static_cast<T>(is.get_arithmetic_as<uint16_t>());
                    case typecode::int_32:
                        return static_cast<T>(is.get_arithmetic_as<int32_t>());
                    case typecode::uint_32:
                        return static_cast<T>(is.get_arithmetic_as<uint32_t>());
                    case typecode::int_64:
                        return static_cast<T>(is.get_arithmetic_as<int64_t>());
                    case typecode::uint_64:
                    {
                        std::uint64_t n = is.get_arithmetic_as<uint64_t>();
                        if (n <= std::numeric_limits<T>::max())
                        {
                            return static_cast<T>(n);
                        }
                        else
                        {
                            throw type_error("invalid value for 64-bit signed integer");
                        }
                    }
                    default:
                        throw type_error("invalid value for 64-bit signed integer");
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
        inline static T get(utils::in_buffer& is)
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
                        return static_cast<T>(is.get_arithmetic_as<uint8_t>());
                    case typecode::uint_16:
                        return static_cast<T>(is.get_arithmetic_as<uint16_t>());
                    case typecode::uint_32:
                        return static_cast<T>(is.get_arithmetic_as<uint32_t>());
                    case typecode::uint_64:
                        return static_cast<T>(is.get_arithmetic_as<uint64_t>());
                    default:
                        throw type_error("invalid value for type 64-bit unsigned integer");
                }
            }
        }
    };
	
    template<>
    struct value_deserializer<std::string>
    {
        inline std::string operator()(utils::in_buffer& is) const
        {
            return get(is);
        }

        inline static std::string get(utils::in_buffer& is)
        {
            auto tcode = is.get();
            if (tcode >= typecode::fixstr_min && tcode <= typecode::fixstr_max)
            {
                std::uint8_t mask = 0x1f;
                std::size_t length = tcode & mask;
                const char* p = reinterpret_cast<const char*>(is.get_bytes(length));
                return std::string(p, length);
            }
            else
            {
                switch(tcode)
                {
                    case typecode::str_8:
                    {
                        std::size_t length = is.get_arithmetic_as<std::uint8_t>();
                        const char* p = reinterpret_cast<const char*>(is.get_bytes(length));
                        return std::string(p, length);
                    }
                    case typecode::str_16:
                    {
                        std::size_t length = is.get_arithmetic_as<std::uint16_t>();
                        const char* p = reinterpret_cast<const char*>(is.get_bytes(length));
                        return std::string(p, length);
                    }
                    case typecode::str_32:
                    {
                        std::size_t length = is.get_arithmetic_as<std::uint32_t>();
                        const char* p = reinterpret_cast<const char*>(is.get_bytes(length));
                        return std::string(p, length);
                    }
                    default:
						std::ostringstream msg;
						msg << "invalid typecode value for string: " << std::hex << (int)tcode << std::dec;
                        throw type_error(msg.str());
                }
            }
        }
    };	
	
	class obstream;
    
	class obstream_cntxt;

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
    class ibstream : public utils::in_buffer
    {
    public:
        using base = utils::in_buffer;

		template<class U, class E> friend struct value_deserializer;
		
		/*! \brief null constructor
		 */
		inline
		ibstream() { }

		/*	\brief constructor with externally-managed buffer
		 * 
		 *	\param data pointer to the beginning of the buffer
		 *	\param size number of bytes in the buffer
		 * 
		 *	The caller is responsible for guaranteeing that the buffer passed as
		 *	\c data will be valid throughout the lifetime of the ibstream instance
		 *	being constructed.
		 */
		inline
		ibstream(const void* data, std::size_t size)
		{
			utils::in_buffer::capture(std::make_unique<utils::const_anon_block>(data, size));
		}
		
		/*! \brief constructor from moved obstream buffer
		 *	\param ostr an obstream instance that provides the buffer for the constructed ibstream
		 * 
		 *	The constructed instance of ibstream will take possession of the internal buffer from
		 *	\c ostr by moving it. 
		 */
		ibstream(obstream&& ostr);
		
		/*! \brief constructor from copied obstream buffer
		 *	\param ostr obstream instance whose internal buffer is copied
		 * 
		 *	The constructed instance of ibstream will own a read-only copy of the buffer
		 *	from \c ostr. The copy will be destroyed when this instance of ibstream is destroyed.
		 */
		ibstream(obstream const& ostr);

		/*!	\brief constructor from moved const_memory_block
		 *	\param block const_memory_block whose contents are moved to this ibstream
		 *	\param limit offset from the beginning of block to the last meaningful byte in the buffer
		 *	
		 *	The constructed instance of ibstream takes possession of \c block (by moving it). \c limit 
		 *	is the number of meaningful bytes (that is, bytes corresponding to the originating obstream)
		 *  in block's buffer, which may be less than \c block.size().
		 */
		ibstream(utils::const_memory_block::ptr&& block, std::size_t limit) 
		: in_buffer{std::move(block), limit}
		{}
		
		/*!	\brief constructor from moved const_memory_block
		 *	\param block const_memory_block whose contents are moved to this ibstream
		 *	
		 *	The constructed instance of ibstream takes possession of \c block (by moving it). The
		 *	entire contents of the buffer are assumed to be meaningful (that is, corresponding to
		 *	the originating obstream).
		 */
		ibstream(utils::const_memory_block::ptr&& block)
		: in_buffer{std::move(block)}
		{}
		
        template<class T>
        inline typename std::enable_if_t<is_ibstream_constructible<T>::value, T>
        read_as()
        {
            return  T(*this);
        }
                
        template<class T>
        inline typename std::enable_if_t<
			!is_ibstream_constructible<T>::value && 
			has_value_deserializer<T>::value, 
			T> 
        read_as()
        {
            return value_deserializer<T>::get(*this);
        }
		
        template<class T>
        inline typename std::enable_if_t<
            !is_ibstream_constructible<T>::value &&
            !has_value_deserializer<T>::value &&
            std::is_default_constructible<T>::value &&
            has_ref_deserializer<T>::value,
            T>
        read_as()
        {
            T obj;
            ref_deserializer<T>::get(*this, obj);
            return obj;
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
                        length = base::get_arithmetic_as<std::uint16_t>();
                    }
                    break;
                    case typecode::array_32:
                    {
                        length = base::get_arithmetic_as<std::uint32_t>();
                    }
                    break;
                    default:
                    {
						std::ostringstream msg;
						msg << "invalid value for array header: " << std::hex << (int)tcode << std::dec;
                        throw type_error(msg.str());
                    }
                }
            }
            return length;
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
                        length = base::get_arithmetic_as<std::uint16_t>();
                    }
                    break;
                    case typecode::map_32:
                    {
                        length = base::get_arithmetic_as<std::uint32_t>();
                    }
                    break;
                    default:
                    {
						std::ostringstream msg;
						msg << "invalid value for map header: " << std::hex << (int)tcode << std::dec;
                        throw type_error(msg.str());
                    }
                }
            }
            return length;
		}
		
		inline ibstream&
		check_map_key(std::string const& key)
		{
			auto name = read_as<std::string>();
			if (name != key)
			{				
				std::ostringstream msg;
				msg << "invalid map key: expected " << key << ", found " << name;
                throw type_error(msg.str());
			}
			return *this;
		}
		
		inline ibstream&
		check_array_header(std::size_t expected)
		{
			auto actual = read_array_header();
			if (actual != expected)
			{
				std::ostringstream msg;
				msg << "invalid size in array header: expected " << expected << ", found " << actual;
                throw type_error(msg.str());
			}
			return *this;
		}

		inline ibstream&
		check_map_header(std::size_t expected)
		{
			auto actual = read_map_header();
			if (actual != expected)
			{
				std::ostringstream msg;
				msg << "invalid size in map header: expected " << expected << ", found " << actual;
                throw type_error(msg.str());
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
                    length = base::get_arithmetic_as<std::uint8_t>();
                }
                break;
                case typecode::bin_16:
                {
                    length = base::get_arithmetic_as<std::uint16_t>();
                }
                break;
                case typecode::bin_32:
                {
                    length = base::get_arithmetic_as<std::uint32_t>();
                }
                break;
                default:
                {
                    throw type_error("invalid value for blob header");
                }
            }
            return length;
        }
		
		void
		read_blob_body(void* dst, std::size_t nbytes)
		{
			const void* src = base::get_bytes(nbytes);
			memcpy(dst, src, nbytes);
		}
		
		std::vector<std::uint8_t> 
		read_blob_body(std::size_t nbytes)
		{
			std::vector<std::uint8_t> blob(nbytes);
			read_blob_body(blob.data(), nbytes);
			return blob;
		}
		
		std::vector<std::uint8_t> 
		read_blob()
		{
			auto nbytes = read_blob_header();
			return read_blob_body(nbytes);
		}

		void
		read_nil()
		{
			auto tcode = base::get();
			if (tcode != typecode::nil)
			{
				throw type_error("expected nil typecode");
			}
		}

	protected:
		
		template<class T>
		using shared_ptr_factory = std::function< std::shared_ptr<T> (ibstream&) >;

		template<class T>
		std::shared_ptr<T> 
		read_as_shared_ptr(shared_ptr_factory<T> factory)
		{
			std::shared_ptr<T> result{nullptr};
			
			auto n = read_array_header();
			if (n != 1)
			{
				throw type_error("ill-formed shared_ptr in stream, invalid array size");
			}
			
			auto code = peek();
			
			if (code == typecode::nil) // nullptr
			{
				code = get();
			}
			else if (typecode::is_positive_int(code)) // saved ptr
			{
				auto index = read_as<std::size_t>();
				result = std::static_pointer_cast<T>(get_saved_ptr(index));
			}
			else if (typecode::is_array(code) || typecode::is_map(code)) // streamed object
			{
					result = factory(*this);
					save_ptr(result);
			}
			else
			{
				throw type_error("invalid typecode for shared_ptr");
			}
			return result;
		}

		template<class T>
		using unique_ptr_factory = std::function< std::unique_ptr<T> (ibstream&) >;
		
		template<class T>
		std::unique_ptr<T>
		read_as_unique_ptr(unique_ptr_factory<T> factory)
		{
			auto n = read_array_header();
			if (n != 1)
			{
				throw type_error("ill-formed unique_ptr in stream, invalid array size");
			}
			
			auto code = peek();
			
			if (code == typecode::nil) // nullptr
			{
				code = get();
				return nullptr;
			}
			else if (typecode::is_array(code) || typecode::is_map(code)) // streamed object
			{
				return factory(*this);
			}
			else
			{
				throw type_error("invalid typecode for unique_ptr");
			}
		}
		
		virtual std::shared_ptr<void> 
		get_saved_ptr(std::size_t index);
		
		virtual void 
		save_ptr(std::shared_ptr<void> ptr);
		
    };
	
	class ibstream_cntxt : public ibstream
	{
	public:
		inline
		ibstream_cntxt() : ibstream{} { }

		inline
		ibstream_cntxt(const void* data, std::size_t size) : ibstream{data, size}
		{}
		
		ibstream_cntxt(obstream_cntxt&& ostr);
		
		ibstream_cntxt(obstream_cntxt const& ostr);

		ibstream_cntxt(utils::const_memory_block::ptr&& block, std::size_t limit) 
		: ibstream{std::move(block), limit}
		{}
		
		ibstream_cntxt(utils::const_memory_block::ptr&& block)
		: ibstream{std::move(block)}
		{}
		
		virtual void rewind() noexcept override
		{
			ibstream::rewind();
			m_shared_pointers.clear();
		}
	protected:		
		virtual void 
		save_ptr(std::shared_ptr<void> ptr) override;

		virtual std::shared_ptr<void> 
		get_saved_ptr(std::size_t index) override;

		std::vector<std::shared_ptr<void>> m_shared_pointers;
	};
        
    template<class T>
    struct value_deserializer<T, std::enable_if_t<std::is_enum<T>::value>>
    {
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
		static inline ibstream&
		get(ibstream& is, T& obj)
		{
			return obj.deserialize(is);
		}
	};
	
	template<class T>
	struct value_deserializer<T, std::enable_if_t<is_ibstream_constructible<T>::value>>
	{
		static inline T get(ibstream& is)
		{
			return T{is};
		}
	};
	
    template<>
    struct value_deserializer<bool>
    {
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
                    throw type_error("invalid value for bool");
            }
        }
    };

    template<>
    struct value_deserializer<float>
    {
        inline static float get(ibstream& is)
        {
            auto tcode = is.get();
            if (tcode == typecode::float_32)
            {
                std::uint32_t unpacked = is.get_arithmetic_as<std::uint32_t>();
                return reinterpret_cast<float&>(unpacked);
            }
            else
            {
                throw type_error("invalid value for float");
            }
        }
    };

    template<>
    struct value_deserializer<double>
    {
        inline static double get(ibstream& is)
        {
            auto tcode = is.get();
            if (tcode == typecode::float_32)
            {
                std::uint32_t unpacked = is.get_arithmetic_as<std::uint32_t>();
                return static_cast<double>(reinterpret_cast<float&>(unpacked));
            }
            else if (tcode == typecode::float_64)
            {
                std::uint64_t unpacked = is.get_arithmetic_as<std::uint64_t>();
                return reinterpret_cast<double&>(unpacked);
            }
            else
            {
                throw type_error("invalid value for double");
            }
        }
    };
	
	template<class Rep, class Ratio>
	struct value_deserializer<std::chrono::duration<Rep,Ratio>>
	{
		using duration_type = std::chrono::duration<Rep,Ratio>;
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
	struct value_deserializer<std::shared_ptr<T>,
		typename std::enable_if_t<is_ibstream_constructible<T>::value>>
	{
		inline static std::shared_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_shared_ptr<T>([] (ibstream& istrm)
			{
				return std::make_shared<T>(istrm);
			});
		}
	};
	
	/*
	 *	If no stream constructor is available, prefer a value_deserializer<T>
	 * 
	 *	The value_deserializer<shared_ptr<T>> based on a value_deserializer<T> 
	 *	requires a move contructor for T
	 */
	template<class T>
	struct value_deserializer<std::shared_ptr<T>,
		typename std::enable_if_t<
			!is_ibstream_constructible<T>::value &&
			has_value_deserializer<T>::value &&
			std::is_move_constructible<T>::value>>
	{
		inline static std::shared_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_shared_ptr<T>([] (ibstream& istrm)
			{
				return std::make_shared<T>(value_deserializer<T>::get(istrm));
			});
		}
	};
	
	/*
	 *	The value_deserializer<shared_ptr<T>> based on a ref_deserializer<T> 
	 *	requires eiher a move contructor or copy constructor for T;
	 *	prefer the move contructor
	 */
	template<class T>
	struct value_deserializer<std::shared_ptr<T>,
		typename std::enable_if_t<
            !is_ibstream_constructible<T>::value &&
            !has_value_deserializer<T>::value &&
            has_ref_deserializer<T>::value &&
            std::is_default_constructible<T>::value &&
            std::is_move_constructible<T>::value>>
	{
		inline static std::shared_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_shared_ptr<T>([] (ibstream& istrm)
			{
                T obj;
                ref_deserializer<T>::get(istrm, obj);
                return std::make_shared<T>(std::move(obj));
				
			});
		}
	};

	template<class T>
	struct value_deserializer<
		std::shared_ptr<T>,
		typename std::enable_if_t<
            !is_ibstream_constructible<T>::value &&
            !has_value_deserializer<T>::value &&
            has_ref_deserializer<T>::value &&
            std::is_default_constructible<T>::value &&
            !std::is_move_constructible<T>::value &&
            std::is_copy_constructible<T>::value>>
	{
		inline static std::shared_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_shared_ptr<T>([] (ibstream& istrm)
			{
                T obj;
                ref_deserializer<T>::get(istrm, obj);
                return std::make_shared<T>(obj);			
			});
		}
	};
	
	
	/*
	 *	Value deserializers for unique pointer types
	 */
	
	/*
	 *	Prefer stream-constructed when available
	 */
	template<class T>
	struct value_deserializer<std::unique_ptr<T>,
		typename std::enable_if_t<is_ibstream_constructible<T>::value>>
	{
		inline static std::unique_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_unique_ptr<T>([] (ibstream& istrm)
			{
				return std::make_unique<T>(istrm);
			});
		}
	};
	
	/*
	 *	If no stream constructor is available, prefer a value_deserializer<T>
	 * 
	 *	The value_deserializer<unique_ptr<T>> based on a value_deserializer<T> 
	 *	requires a move contructor for T
	 */
	template<class T>
	struct value_deserializer<std::unique_ptr<T>,
		typename std::enable_if_t<
			!is_ibstream_constructible<T>::value &&
			has_value_deserializer<T>::value &&
			std::is_move_constructible<T>::value>>
	{
		inline static std::unique_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_unique_ptr<T>([] (ibstream& istrm)
			{
				return std::make_unique<T>(value_deserializer<T>::get(istrm));
			});
		}
	};

	
	/*
	 *	The value_deserializer<unique_ptr<T>> based on a ref_deserializer<T> 
	 *	requires either a move contructor or copy constructor for T;
	 *	prefer the move contructor
	 */
	template<class T>
	struct value_deserializer<std::unique_ptr<T>,
		typename std::enable_if_t<
            !is_ibstream_constructible<T>::value &&
            !has_value_deserializer<T>::value &&
            has_ref_deserializer<T>::value &&
            std::is_default_constructible<T>::value &&
            std::is_move_constructible<T>::value>>
	{
		inline static std::unique_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_unique_ptr<T>([] (ibstream& istrm)
			{
                T obj;
                ref_deserializer<T>::get(istrm, obj);
                return std::make_unique<T>(std::move(obj));
				
			});
		}
	};

	template<class T>
	struct value_deserializer<std::unique_ptr<T>,
		typename std::enable_if_t<
            !is_ibstream_constructible<T>::value &&
            !has_value_deserializer<T>::value &&
            has_ref_deserializer<T>::value &&
            std::is_default_constructible<T>::value &&
            !std::is_move_constructible<T>::value &&
            std::is_copy_constructible<T>::value>>
	{
		inline static std::unique_ptr<T>
		get(ibstream& is)
		{
			return is.read_as_unique_ptr<T>([] (ibstream& istrm)
			{
                T obj;
                ref_deserializer<T>::get(istrm, obj);
                return std::make_unique<T>(obj);			
			});
		}
	};
	
    template<class T, class Enable = void> 
    struct ibstream_initializer;

//    template<class T>
//    struct ibstream_initializer<T, 
//        typename std::enable_if_t<is_ibstream_constructible<T>::value>>
//    {
//        using param_type = ibstream&;
//        using return_type = ibstream&;
//        inline static return_type get(param_type is)
//        {
//            return is;
//        }
//    };
//
//    template<class T>
//    struct ibstream_initializer <T, 
//        typename std::enable_if_t<
//            !is_ibstream_constructible<T>::value &&
//            has_value_deserializer<T>::value>>
//    {
//        using param_type = ibstream&;
//        using return_type = T;
//        inline static return_type get(param_type is)
//        {
//            return value_deserializer<T>::get(is);
//        }
//    };
//     
//    template<class T>
//    struct ibstream_initializer <T, 
//        typename std::enable_if_t<
//            !is_ibstream_constructible<T>::value &&
//            !has_value_deserializer<T>::value &&
//			std::is_default_constructible<T>::value &&
//            (std::is_copy_constructible<T>::value || std::is_move_constructible<T>::value) &&
//            has_ref_deserializer<T>::value>>
//    {
//        using param_type = ibstream&;
//        using return_type = T;
//        inline static return_type get(param_type is)
//        {
//            T obj;
//            ref_deserializer<T>::get(is, obj);
//            return obj;
//        }
//    };

    template<class T, class Alloc>
    struct value_deserializer<std::vector<T, Alloc>,
			typename std::enable_if_t<is_ibstream_readable<T>::value>>
    {
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
		static inline std::pair<T1, T2>
		get(ibstream& is)
		{
			is.check_array_header(2);
			T1 t1{ibstream_initializer<T1>::get(is)};
			T2 t2{ibstream_initializer<T2>::get(is)};
			return std::make_pair(t1, t2);
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

