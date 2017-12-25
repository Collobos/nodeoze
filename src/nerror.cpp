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

#include <nodeoze/nerror.h>

using namespace nodeoze;

std::string
nodeoze::err_to_string( nodeoze::err_t err )
{
	std::ostringstream os;

	os << err;

	return os.str();
}


std::ostream&
operator<<( std::ostream &output, const nodeoze::err_t err )
{
	return output << ( int ) err;
}


namespace nodeoze {

err_t
canonicalize( std::error_code error )
{
	if ( error.category() == error_category() )
	{
		return static_cast< err_t >( error.value() );
	}
	else
	{
		return err_t::unexpected;
	}
}

class category : public std::error_category
{
public:

	virtual const char*
	name() const noexcept override
    {
		return "nodeoze";
    }
	
    virtual std::string
	message( int value ) const override
    {
		std::ostringstream os;
		
		switch ( static_cast< err_t >( value ) )
        {
			case err_t::ok:
			{
				os << "ok";
			}
			break;
			
			case err_t::expired:
			{
				os << "expired";
			}
			break;
			
			case err_t::no_memory:
			{
				os << "no memory";
			}
			break;
			
			case err_t::auth_failed:
			{
				os << "authentication failed";
			}
			break;
			
			case err_t::bad_password:
			{
				os << "bad password";
			}
			break;
			
			case err_t::write_failed:
			{
				os << "write failed";
			}
			break;
			
			case err_t::not_implemented:
			{
				os << "not implemented";
			}
			break;
			
			case err_t::unexpected:
			{
				os << "unexpected";
			}
			break;
			
			case err_t::connection_aborted:
			{
				os << "connection aborted";
			}
			break;
			
			case err_t::permission_denied:
			{
				os << "permission denied";
			}
			break;
			
			case err_t::limit_error:
			{
				os << "limit";
			}
			break;
			
			case err_t::network_error:
			{
				os << "network error";
			}
			break;
			
			case err_t::uninitialized:
			{
				os << "uninitialized";
			}
			break;
			
			case err_t::component_failure:
			{
				os << "component failure";
			}
			break;
			
			case err_t::eof:
			{
				os << "eof";
			}
			break;
			
			case err_t::not_configured:
			{
				os << "not configured";
			}
			break;
			
			case err_t::conflict:
			{
				os << "conflict";
			}
			break;
			
			case err_t::not_exist:
			{
				os << "not exist";
			}
			break;
			
			case err_t::bad_token:
			{
				 os << "bad token";
			}
			break;
			
			case err_t::timeout:
			{
				os << "timeout";
			}
			break;
			
			case err_t::offline:
			{
				os << "offline";
			}
			break;
			
			case err_t::unknown_user:
			{
				os << "unknown user";
			}
			break;
			
			case err_t::invalid_request:
			{
				os << "invalid request";
			}
			break;
			
			case err_t::method_not_found:
			{
				os << "method not found";
			}
			break;
			
			case err_t::invalid_params:
			{
				os << "invalid params";
			}
			break;
			
			case err_t::internal_error:
			{
				os << "internal error";
			}
			break;
			
			case err_t::parse_error:
			{
				os << "parse error";
			}
			break;
			
			case err_t::already_exist:
			{
				os << "already exists";
			}
			break;
			
			case err_t::update_conflict:
			{
				os << "update conflict";
			}
			break;
			
			case err_t::db_error:
			{
				os << "database error";
			}
			break;
			
		}
		
		return os.str();
    }
};

const std::error_category&
error_category()
{
	static auto instance = new category;
    return *instance;
}

}
