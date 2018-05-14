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
 * File:   macros.h
 * Author: David Curtis
 *
 * Created on June 30, 2017, 11:50 AM
 */

#ifndef BSTREAM_MACROS_H
#define BSTREAM_MACROS_H

#define BOOST_PP_VARIADICS 1

#include <boost/preprocessor/punctuation/remove_parens.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>

#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/facilities/empty.hpp>

#include <boost/preprocessor/tuple/eat.hpp>

#include <boost/preprocessor/variadic/to_seq.hpp>
#include <boost/preprocessor/variadic/to_list.hpp>
#include <boost/preprocessor/variadic/to_array.hpp>

#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>

#include <boost/preprocessor/array/size.hpp>
#include <boost/preprocessor/array/to_seq.hpp>

#include <boost/preprocessor/list/for_each_i.hpp>

#include <boost/preprocessor/stringize.hpp>

#include <nodeoze/bstream/base_classes.h>
#include <nodeoze/bstream/utils/preprocessor.h>

#define BSTRM_BUILD_OPTIONAL_ARRAY_(...)									\
	BOOST_PP_IIF(UTILS_PP_ISEMPTY(__VA_ARGS__),									\
		BSTRM_ZERO_LENGTH_ARRAY_,											\
		BSTRM_BUILD_ARRAY_)(__VA_ARGS__)									\
/**/

#define BSTRM_BUILD_ARRAY_(...)											\
	BOOST_PP_VARIADIC_TO_ARRAY(BOOST_PP_REMOVE_PARENS(__VA_ARGS__))				\
/**/

#define BSTRM_CLASS(name, bases, members)						\
    BSTRM_CLASS_(name,											\
		BSTRM_BUILD_OPTIONAL_ARRAY_(bases),								\
		BSTRM_BUILD_OPTIONAL_ARRAY_(members))								\
/**/

#define BSTRM_MAP_CLASS(name, bases, members)					\
    BSTRM_MAP_CLASS_(name,										\
		BSTRM_BUILD_OPTIONAL_ARRAY_(bases),								\
		BSTRM_BUILD_OPTIONAL_ARRAY_(members))								\
/*!!!*/

#define BSTRM_MAP_BASE(class_name)							\
    private BSTRM_MAP_BASE_TYPE_(class_name)					\
/**/

#define BSTRM_BASE(class_name)								\
    private BSTRM_BASE_ARRAY_TYPE_(class_name)					\
/**/

#define BSTRM_FRIEND_BASE(class_name)								\
    friend class BSTRM_BASE_ARRAY_TYPE_(class_name);			\
	using base_type = BSTRM_BASE_ARRAY_TYPE_(class_name);		\
/**/

#define BSTRM_FRIEND_MAP_BASE(class_name)							\
    friend class BSTRM_MAP_BASE_TYPE_(class_name);				\
	using base_type = BSTRM_MAP_BASE_TYPE_(class_name);			\
/**/

#define BSTRM_BASE_ALIAS_(name)											\
	base_type																	\
/**/

#define BSTRM_CTOR(name, bases, members)									\
    BSTRM_CTOR_(name,														\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/

#define BSTRM_MAP_CTOR(name, bases, members)								\
    BSTRM_MAP_CTOR_(name,													\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/

#define BSTRM_CTOR_DECL(name)												\
    BSTRM_CTOR_DECL_SIG_(name) ;											\
/**/
    
#define BSTRM_CTOR_DEF(scope, name, bases, members)						\
    BSTRM_CTOR_DEF_(scope, name,											\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/
 
#define BSTRM_MAP_CTOR_DEF(scope, name, bases, members)					\
    BSTRM_MAP_CTOR_DEF_(scope, name,										\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/
 
#define BSTRM_ITEM_COUNT(bases, members)									\
    BSTRM_ITEM_COUNT_(													\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/

#define BSTRM_INIT_BASE(name)												\
    name{nodeoze::bstream::ibstream_initializer<name>::get(is)}				\
/**/

#define BSTRM_INIT_MEMBER(name)											\
    name{nodeoze::bstream::ibstream_initializer								\
		<decltype(name)>::get(is)}												\
/**/

#define BSTRM_INIT_MAP_BASE(name)											\
    name{nodeoze::bstream::ibstream_initializer<name>::						\
		get(is.check_map_key(BOOST_PP_STRINGIZE(name)))}						\
/**/

#define BSTRM_INIT_MAP_MEMBER(name)										\
    name{nodeoze::bstream::ibstream_initializer								\
		<decltype(name)>::get(is.check_map_key(BOOST_PP_STRINGIZE(name)))}		\
/**/

#define BSTRM_SERIALIZE(name, bases, members)								\
    BSTRM_SERIALIZE_(name,												\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/

#define BSTRM_SERIALIZE_MAP(name, bases, members)							\
    BSTRM_SERIALIZE_MAP_(name,											\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/

#define BSTRM_SERIALIZE_DECL()											\
     BSTRM_SERIALIZE_DECL_SIG_() ;										\
/**/

#define BSTRM_SERIALIZE_DEF(scope, name, bases, members)					\
    BSTRM_SERIALIZE_DEF_(scope, name,										\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/

#define BSTRM_SERIALIZE_MAP_DEF(scope, name, bases, members)				\
    BSTRM_SERIALIZE_MAP_DEF_(scope, name,									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(bases),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_BASES_)(bases),									\
		BOOST_PP_IIF(UTILS_PP_ISEMPTY(members),									\
			BSTRM_ZERO_LENGTH_ARRAY_,										\
			BSTRM_BUILD_MEMBERS_)(members))								\
/**/


#define BSTRM_ZERO_LENGTH_ARRAY_(...) (0,())

#define BSTRM_BUILD_BASES_(bases)											\
	BOOST_PP_VARIADIC_TO_ARRAY(BOOST_PP_REMOVE_PARENS(bases))					\
/**/
	
#define BSTRM_BUILD_MEMBERS_(members)										\
	BOOST_PP_VARIADIC_TO_ARRAY(BOOST_PP_REMOVE_PARENS(members))					\
/**/
	

#define BSTRM_CLASS_(name, base_array, member_array)			\
    BSTRM_FRIEND_BASE(name)											\
    BSTRM_CTOR_(name, base_array, member_array)							\
    BSTRM_ITEM_COUNT_(base_array, member_array)							\
    BSTRM_SERIALIZE_(name, base_array, member_array)						\
/**/

#define BSTRM_MAP_CLASS_(name, base_array, member_array)		\
    BSTRM_FRIEND_MAP_BASE(name)										\
    BSTRM_MAP_CTOR_(name, base_array, member_array)						\
    BSTRM_ITEM_COUNT_(base_array, member_array)							\
    BSTRM_SERIALIZE_MAP_(name, base_array, member_array)					\
/*!!!*/

#define BSTRM_BASE_ARRAY_TYPE_(class_name)						\
    nodeoze::bstream::array_base<class_name>						\
/**/

#define BSTRM_MAP_BASE_TYPE_(class_name)						\
    nodeoze::bstream::map_base<class_name>							\
/**/

#define BSTRM_CTOR_(name, base_array, member_array)						\
    BSTRM_CTOR_DECL_SIG_(name) :											\
    BSTRM_INIT_BASE_(name)										\
    BSTRM_INIT_BASES_(base_array)											\
    BSTRM_INIT_MEMBERS_(member_array)	{}									\
/**/

#define BSTRM_MAP_CTOR_(name, base_array, member_array)					\
    BSTRM_CTOR_DECL_SIG_(name) :											\
    BSTRM_INIT_BASE_(name)										\
    BSTRM_INIT_MAP_BASES_(base_array)										\
    BSTRM_INIT_MAP_MEMBERS_(member_array)	{}								\
/*!!!*/

#define BSTRM_INIT_BASE_(name)									\
    BSTRM_BASE_ALIAS_(name) {is}											\
/**/

/*
#define BSTRM_INIT_BASES_(base_array)										\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(base_array),								\
    BSTRM_INIT_BASES_SEQ_(BOOST_PP_ARRAY_TO_SEQ(base_array)),				\
									BOOST_PP_EMPTY())							\
*/

#define BSTRM_INIT_BASES_(base_array)										\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(base_array),								\
				BSTRM_DO_INIT_BASES_,										\
				BSTRM_DO_NOT_INIT_BASES_)(base_array)						\
/**/

#define BSTRM_INIT_MAP_BASES_(base_array)									\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(base_array),								\
				BSTRM_DO_INIT_MAP_BASES_,									\
				BSTRM_DO_NOT_INIT_BASES_)(base_array)						\
/**/

#define BSTRM_DO_INIT_BASES_(base_array)									\
    BSTRM_INIT_BASES_SEQ_(BOOST_PP_ARRAY_TO_SEQ(base_array))				\
/**/

#define BSTRM_DO_INIT_MAP_BASES_(base_array)								\
    BSTRM_INIT_MAP_BASES_SEQ_(BOOST_PP_ARRAY_TO_SEQ(base_array))			\
/**/

#define BSTRM_DO_NOT_INIT_BASES_(base_array) 

#define BSTRM_INIT_BASES_SEQ_(base_seq)									\
    BOOST_PP_SEQ_FOR_EACH_I(BSTRM_INIT_EACH_BASE_, _, base_seq)			\
/**/
    
#define BSTRM_INIT_MAP_BASES_SEQ_(base_seq)								\
    BOOST_PP_SEQ_FOR_EACH_I(BSTRM_INIT_EACH_MAP_BASE_, _, base_seq)		\
/**/
    
#define BSTRM_INIT_EACH_BASE_(r, data, i, elem),							\
    elem{nodeoze::bstream::ibstream_initializer<elem>::get(is)}				\
/**/

#define BSTRM_INIT_EACH_MAP_BASE_(r, data, i, elem),						\
    elem{nodeoze::bstream::ibstream_initializer<elem>::						\
		get(is.check_map_key(BOOST_PP_STRINGIZE(elem)))}						\
/**/

#define BSTRM_INIT_MEMBERS_(member_array)									\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(member_array),								\
				BSTRM_DO_INIT_MEMBERS_,									\
				BSTRM_DO_NOT_INIT_MEMBERS_)(member_array)					\
/**/

#define BSTRM_INIT_MAP_MEMBERS_(member_array)								\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(member_array),								\
				BSTRM_DO_INIT_MAP_MEMBERS_,								\
				BSTRM_DO_NOT_INIT_MEMBERS_)(member_array)					\
/**/

#define BSTRM_DO_NOT_INIT_MEMBERS_(member_array)

#define BSTRM_DO_INIT_MEMBERS_(member_array)								\
    BSTRM_INIT_MEMBERS_SEQ_(BOOST_PP_ARRAY_TO_SEQ(member_array))			\
/**/

#define BSTRM_DO_INIT_MAP_MEMBERS_(member_array)							\
    BSTRM_INIT_MAP_MEMBERS_SEQ_(BOOST_PP_ARRAY_TO_SEQ(member_array))		\
/**/

#define BSTRM_INIT_MEMBERS_SEQ_(member_seq)								\
    BOOST_PP_SEQ_FOR_EACH_I(BSTRM_INIT_EACH_MEMBER_, _, member_seq)		\
/**/

#define BSTRM_INIT_MAP_MEMBERS_SEQ_(member_seq)							\
    BOOST_PP_SEQ_FOR_EACH_I(BSTRM_INIT_EACH_MAP_MEMBER_, _, member_seq)	\
/**/

#define BSTRM_INIT_EACH_MEMBER_(r, data, i, elem),						\
    elem{nodeoze::bstream::ibstream_initializer<decltype(elem)>::get(is)}		\
/**/            

#define BSTRM_INIT_EACH_MAP_MEMBER_(r, data, i, elem),					\
    elem{nodeoze::bstream::ibstream_initializer<decltype(elem)>::				\
		get(is.check_map_key(BOOST_PP_STRINGIZE(elem)))}						\
/**/            

#define BSTRM_ITEM_COUNT_(base_array, member_array)						\
    constexpr std::size_t _streamed_item_count() const {						\
    return BSTRM_ITEM_COUNT_EVAL_(base_array, member_array) ; }			\
/**/
            
#define BSTRM_ITEM_COUNT_EVAL_(base_array, member_array)					\
    BOOST_PP_ADD(																\
        BOOST_PP_ARRAY_SIZE(base_array),										\
        BOOST_PP_ARRAY_SIZE(member_array) )										\
/**/

#define BSTRM_CTOR_DECL_SIG_(name)										\
    name(nodeoze::bstream::ibstream& is)										\
/**/

#define BSTRM_CTOR_DEF_SIG_(scope, name)									\
    BOOST_PP_IIF(UTILS_PP_ISEMPTY(scope),										\
        BSTRM_CTOR_DEF_SIG_UNSCOPED_,										\
        BSTRM_CTOR_DEF_SIG_SCOPED_)(scope, name)							\
/**/

#define BSTRM_CTOR_DEF_SIG_UNSCOPED_(scope, name)							\
    name::name(nodeoze::bstream::ibstream& is)								\
/**/

#define BSTRM_CTOR_DEF_SIG_SCOPED_(scope, name)							\
    scope::name::name(nodeoze::bstream::ibstream& is)							\
/**/

#define BSTRM_CTOR_DEF_(scope, name, base_array, member_array)			\
    BSTRM_CTOR_DEF_SIG_(scope, name) :									\
    BSTRM_INIT_BASE_(name)										\
    BSTRM_INIT_BASES_(base_array)											\
    BSTRM_INIT_MEMBERS_(member_array)										\

#define BSTRM_MAP_CTOR_DEF_(scope, name, base_array, member_array)		\
    BSTRM_CTOR_DEF_SIG_(scope, name) :									\
    BSTRM_INIT_BASE_(name)										\
    BSTRM_INIT_MAP_BASES_(base_array)										\
    BSTRM_INIT_MAP_MEMBERS_(member_array)									\
/*!!!*/


#define BSTRM_SERIALIZE_(name, base_array, member_array)					\
    BSTRM_SERIALIZE_DECL_SIG_()											\
    BSTRM_SERIALIZE_BODY_(name, base_array, member_array)					\
/**/

#define BSTRM_SERIALIZE_MAP_(name, base_array, member_array)				\
    BSTRM_SERIALIZE_DECL_SIG_()											\
    BSTRM_SERIALIZE_MAP_BODY_(name, base_array, member_array)				\
/**/

#define BSTRM_SERIALIZE_DECL_SIG_()										\
    inline nodeoze::bstream::obstream&										\
    serialize(nodeoze::bstream::obstream& os) const							\
/**/

#define BSTRM_SERIALIZE_DEF_(scope, name, base_array, member_array)		\
    BSTRM_SERIALIZE_DEF_SIG_(scope, name)									\
    BSTRM_SERIALIZE_BODY_(name, base_array, member_array)					\
/**/

#define BSTRM_SERIALIZE_MAP_DEF_(scope, name, base_array, member_array)	\
    BSTRM_SERIALIZE_DEF_SIG_(scope, name)									\
    BSTRM_SERIALIZE_MAP_BODY_(name, base_array, member_array)				\
/**/

#define BSTRM_SERIALIZE_DEF_SIG_(scope, name)								\
    BOOST_PP_IIF(UTILS_PP_ISEMPTY(scope),										\
        BSTRM_SERIALIZE_DEF_SIG_UNSCOPED_,								\
        BSTRM_SERIALIZE_DEF_SIG_SCOPED_)(scope, name)						\
/**/

#define BSTRM_SERIALIZE_DEF_SIG_UNSCOPED_(scope, name)					\
        name::serialize(nodeoze::bstream::obstream& os) const,				\
/**/

#define BSTRM_SERIALIZE_DEF_SIG_SCOPED_(scope, name)						\
        scope::name::serialize(nodeoze::bstream::obstream& os) const )		\
/**/

#define BSTRM_SERIALIZE_BODY_(name, base_array, member_array)				\
    {																			\
        BSTRM_SERIALIZE_BASE_(name)								\
        BSTRM_SERIALIZE_BASES_(base_array)								\
        BSTRM_SERIALIZE_MEMBERS_(member_array)							\
        return os;																\
    }																			\
/**/

#define BSTRM_SERIALIZE_MAP_BODY_(name, base_array, member_array)			\
    {																			\
        BSTRM_SERIALIZE_BASE_(name)								\
        BSTRM_SERIALIZE_MAP_BASES_(base_array)							\
        BSTRM_SERIALIZE_MAP_MEMBERS_(member_array)						\
        return os;																\
    }																			\
/**/

#define BSTRM_SERIALIZE_BASE_(name)								\
    BSTRM_BASE_ALIAS_(name)::_serialize(os);								\
/**/

#define BSTRM_SERIALIZE_BASES_(base_array)								\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(base_array),								\
    BSTRM_SERIALIZE_BASES_SEQ_(											\
			BOOST_PP_ARRAY_TO_SEQ(base_array)), )								\
 /*???*/

#define BSTRM_SERIALIZE_MAP_BASES_(base_array)							\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(base_array),								\
    BSTRM_SERIALIZE_MAP_BASES_SEQ_(										\
			BOOST_PP_ARRAY_TO_SEQ(base_array)), )								\
 /*???*/

#define BSTRM_SERIALIZE_BASES_SEQ_(base_seq)								\
    BOOST_PP_SEQ_FOR_EACH_I(													\
			BSTRM_SERIALIZE_EACH_BASE_, _, base_seq)						\
/**/

#define BSTRM_SERIALIZE_MAP_BASES_SEQ_(base_seq)							\
    BOOST_PP_SEQ_FOR_EACH_I(													\
			BSTRM_SERIALIZE_EACH_MAP_BASE_, _, base_seq)					\
/**/

#define BSTRM_SERIALIZE_EACH_MEMBER_(r, data, i, member)					\
    os << member;																\
/**/

#define BSTRM_SERIALIZE_EACH_MAP_MEMBER_(r, data, i, member)				\
	os << std::string(BOOST_PP_STRINGIZE(member));								\
    os << member;																\
/**/

#define BSTRM_SERIALIZE_EACH_BASE_(r, data, i, base)						\
    os << static_cast<const base&>(*this);										\
/**/

#define BSTRM_SERIALIZE_EACH_MAP_BASE_(r, data, i, base)					\
	os << std::string(BOOST_PP_STRINGIZE(base));								\
    os << static_cast<const base&>(*this);										\
/**/

#define BSTRM_SERIALIZE_MEMBERS_(member_array)							\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(member_array),								\
    BSTRM_SERIALIZE_MEMBERS_SEQ_(											\
			BOOST_PP_ARRAY_TO_SEQ(member_array)), )								\
/**/

#define BSTRM_SERIALIZE_MAP_MEMBERS_(member_array)						\
    BOOST_PP_IF(BOOST_PP_ARRAY_SIZE(member_array),								\
    BSTRM_SERIALIZE_MAP_MEMBERS_SEQ_(										\
			BOOST_PP_ARRAY_TO_SEQ(member_array)), )								\
/**/

#define BSTRM_SERIALIZE_MEMBERS_SEQ_(members_seq)							\
    BOOST_PP_SEQ_FOR_EACH_I(													\
			BSTRM_SERIALIZE_EACH_MEMBER_, _, members_seq)					\
/**/
		
#define BSTRM_SERIALIZE_MAP_MEMBERS_SEQ_(members_seq)						\
    BOOST_PP_SEQ_FOR_EACH_I(													\
			BSTRM_SERIALIZE_EACH_MAP_MEMBER_, _, members_seq)				\
/**/

#endif /* BSTREAM_MACROS_H */
