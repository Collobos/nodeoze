/*
 * Copyright (c) 2013-2017, Collobos Software Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef _nodeoze_error_h
#define _nodeoze_error_h

#include <nodeoze/nlog.h>
#include <unordered_map>
#include <iostream>
#include <string>
#include <memory>

namespace nodeoze {

enum class err_t
{
	ok					= 0,
	expired				= -32000,
	no_memory			= -32001,
	auth_failed			= -32002,
	bad_password		= -32003,
	write_failed		= -32004,
	not_implemented		= -32005,
	unexpected			= -32006,
	connection_aborted	= -32007,
	permission_denied	= -32008,
	limit_error			= -32009,
	network_error		= -32010,
	uninitialized		= -32011,
	component_failure	= -32012,
	eof					= -32013,
	not_configured		= -32014,
	conflict			= -32015,
	not_exist			= -32016,
	bad_token			= -32017,
	timeout				= -32018,
	offline				= -32019,
	unknown_user		= -32020,
	invalid_request		= -32600,
	method_not_found	= -32601,
	invalid_params		= -32602,
	internal_error		= -32603,
	parse_error			= -32700,
	already_exist		= -32800,
	update_conflict		= -32801,
	db_error			= -32802,
	
};

typedef std::function< void ( err_t err ) > reply_f;

std::string
err_to_string( err_t v );

inline std::ostream&
operator<<(std::ostream &os, const nodeoze::err_t err )
{
	switch ( err )
	{
		case nodeoze::err_t::ok:
		{
			os << "ok";
		}
		break;
	
		case nodeoze::err_t::expired:
		{
			os << "expired";
		}
		break;
		
		case nodeoze::err_t::no_memory:
		{
			os << "out of memory";
		}
		break;
		
		case nodeoze::err_t::auth_failed:
		{
			os << "authentication failed";
		}
		break;
		
		case nodeoze::err_t::bad_password:
		{
			os << "bad password";
		}
		break;
		
		case nodeoze::err_t::write_failed:
		{
			os << "write failed";
		}
		break;
		
		case nodeoze::err_t::not_implemented:
		{
			os << "not implemented";
		}
		break;
		
		case nodeoze::err_t::unexpected:
		{
			os << "unexpected";
		}
		break;
		
		case nodeoze::err_t::connection_aborted:
		{
			os << "connection aborted";
		}
		break;
		
		case nodeoze::err_t::parse_error:
		{
			os << "parse error";
		}
		break;
		
		case nodeoze::err_t::permission_denied:
		{
			os << "permission denied";
		}
		break;
		
		case nodeoze::err_t::limit_error:
		{
			os << "limit error";
		}
		break;
		
		case nodeoze::err_t::network_error:
		{
			os << "network error";
		}
		break;
		
		case nodeoze::err_t::uninitialized:
		{
			os << "uninitialized";
		}
		break;
		
		case nodeoze::err_t::component_failure:
		{
			os << "component failure";
		}
		break;
		
		case nodeoze::err_t::eof:
		{
			os << "eof";
		}
		break;
		
		case nodeoze::err_t::not_configured:
		{
			os << "not configured";
		}
		break;
		
		case nodeoze::err_t::conflict:
		{
			os << "conflict";
		}
		break;
		
		case nodeoze::err_t::not_exist:
		{
			 os << "not exist";
		}
		break;
		
		case nodeoze::err_t::timeout:
		{
			os << "timeout";
		}
		break;
		
		case nodeoze::err_t::offline:
		{
			os << "offline";
		}
		break;
		
		case nodeoze::err_t::invalid_request:
		{
			os << "invalid request";
		}
		break;
		
		case nodeoze::err_t::bad_token:
		{
			os << "bad token";
		}
		break;
		
		case nodeoze::err_t::method_not_found:
		{
			os << "method not found";
		}
		break;
		
		case nodeoze::err_t::invalid_params:
		{
			os << "invalid params";
		}
		break;
		
		case nodeoze::err_t::internal_error:
		{
			os << "internal error";
		}
		break;
		
		case nodeoze::err_t::already_exist:
		{
			os << "already exist";
		}
		break;
		
		case nodeoze::err_t::update_conflict:
		{
			os << "update conflict";
		}
		break;
		
		case nodeoze::err_t::db_error:
		{
			os << "database error";
		}
		break;
		
		default:
		{
			os << "unknown error";
		}
		break;
	}
	
	return os;
}

inline int
system_error()
{
#if defined( WIN32 )

	return GetLastError();
	
#else

	return errno;

#endif
}


inline void
set_system_error( int error )
{
#if defined( WIN32 )

	SetLastError( error );
	
#else

	errno = error;

#endif
}

const std::error_category&
error_category();

err_t
canonicalize( std::error_code error );

}

namespace std
{
	template<>
	struct is_error_code_enum< nodeoze::err_t > : public std::true_type {};
	
	inline std::error_code
	make_error_code( nodeoze::err_t val )
	{
		return std::error_code( static_cast<int>( val ), nodeoze::error_category() );
	}
}

#endif
