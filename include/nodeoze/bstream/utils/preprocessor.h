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
 * File:   preprocessor.h
 * Author: David Curtis
 *
 * Created on July 5, 2017, 11:21 AM
 */

#ifndef NODEOZE_BSTREAM_UTILS_PREPROCESSOR_H
#define NODEOZE_BSTREAM_UTILS_PREPROCESSOR_H

#define UTILS_PP_ARG16_( _0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, ... ) _15
#define UTILS_PP_HAS_COMMA_( ... ) UTILS_PP_ARG16_( __VA_ARGS__, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 )
#define UTILS_PP_TRIGGER_PARENTHESIS_( ... ) ,

#define UTILS_PP_ISEMPTY( ... )																\
UTILS_PP_ISEMPTY_(																			\
          /* test if there is just one argument, eventually an empty						\
             one */																			\
          UTILS_PP_HAS_COMMA_( __VA_ARGS__ ),												\
          /* test if UTILS_PP_TRIGGER_PARENTHESIS_ together with the argument				\
             adds a comma */																\
          UTILS_PP_HAS_COMMA_( UTILS_PP_TRIGGER_PARENTHESIS_ __VA_ARGS__ ),					\
          /* test if the argument together with a parenthesis								\
             adds a comma */																\
          UTILS_PP_HAS_COMMA_( __VA_ARGS__ ( /*empty*/ ) ),									\
          /* test if placing it between UTILS_PP_TRIGGER_PARENTHESIS_ and the				\
             parenthesis adds a comma */													\
          UTILS_PP_HAS_COMMA_( UTILS_PP_TRIGGER_PARENTHESIS_ __VA_ARGS__ ( /*empty*/ ) )	\
          )
          
#define UTILS_PP_PASTE5_( _0, _1, _2, _3, _4 ) _0 ## _1 ## _2 ## _3 ## _4
#define UTILS_PP_ISEMPTY_( _0, _1, _2, _3 ) UTILS_PP_HAS_COMMA_( UTILS_PP_PASTE5_( UTILS_PP_IS_EMPTY_CASE_, _0, _1, _2, _3 ) )
#define UTILS_PP_IS_EMPTY_CASE_0001 ,

#define UTILS_PP_ECHO( ... ) __VA_ARGS__

#define UTILS_PP_WRAP_ECHO( ... ) ( __VA_ARGS__ )

#define UTILS_PP_DO_NOTHING( ... )

#endif /* NODEOZE_BSTREAM_UTILS_PREPROCESSOR_H */

