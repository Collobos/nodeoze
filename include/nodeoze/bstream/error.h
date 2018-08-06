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
	explicit type_error( const std::string& what_arg ) 
	: 
	std::logic_error{ what_arg } 
	{}

	explicit type_error( const char* what_arg )
	: 
	std::logic_error{ what_arg } 
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
	invalid_operation,
	invalid_state,
	ibstreambuf_not_shareable,
};

class bstream_category_impl : public std::error_category
{
public:
	virtual const char* name() const noexcept override;
	virtual std::string message( int ev ) const noexcept override;
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

} // namespace bstream
} // namespace nodeoze

namespace std
{
  template <>
  struct is_error_condition_enum< nodeoze::bstream::errc > : public true_type {};
}

#endif /* NODEOZE_BSTREAM_ERROR_H */

