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
 * File:   error.h
 * Author: David Curtis
 *
 * Created on June 29, 2017, 1:35 PM
 */

#ifndef NODEOZE_BSTREAM_ERROR_H
#define NODEOZE_BSTREAM_ERROR_H

#include <system_error>

namespace nodeoze
{
namespace bstream
{

class type_error : public std::logic_error
{
public:
	explicit type_error(const std::string& what_arg) 
	: 
	logic_error{what_arg} 
	{}

	explicit type_error(const char* what_arg)
	: 
	logic_error{what_arg} 
	{}
};

enum class errc
{
	ok = 0,
	read_past_end_of_stream,
	type_error,
	member_count_error,
	context_mismatch,
	invalid_err_category,
	invalid_ptr_downcast,
	abstract_non_poly_class,
};

class bstream_category_impl : public std::error_category
{
public:
	virtual const char* name() const noexcept override
	{
		return "nodeoze::bstream";
	}

	virtual std::string message(int ev) const noexcept override
	{
		switch ( static_cast<errc> (ev) )
		{
		case bstream::errc::ok:
			return "success";
		case bstream::errc::read_past_end_of_stream:
			return "read past end of stream";
		case bstream::errc::type_error:
			return "type error";
		case bstream::errc::member_count_error:
			return "member count error";
		case bstream::errc::context_mismatch:
			return "context mismatch";
		case bstream::errc::invalid_err_category:
			return "invalid error category";
		case bstream::errc::invalid_ptr_downcast:
			return "invalid pointer downcast";
		case bstream::errc::abstract_non_poly_class:
			return "abstract class not found in polymorphic context";
		default:
			return "unknown bstream error";
		}
	}
};

inline std::error_category const& bstream_category() noexcept
{
    static bstream_category_impl instance;
    return instance;
}

inline std::error_condition 
make_error_condition( errc e )
{
    return std::error_condition( static_cast< int >( e ), bstream_category() );
}

inline std::error_code 
make_error_code( errc e )
{
    return std::error_code( static_cast< int >( e ), bstream_category() );
}

inline void
clear_error( std::error_code& ec )
{
	static const std::error_code good = make_error_code( bstream::errc::ok );
	ec = good;
}

} // namespace bstream
} // namespace nodeoze

namespace std
{
  template <>
  struct is_error_condition_enum< nodeoze::bstream::errc > : public true_type {};
}

#endif /* NODEOZE_BSTREAM_ERROR_H */

