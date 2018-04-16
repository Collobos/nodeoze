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
 
#ifndef _nodeoze_json_h
#define _nodeoze_json_h

#include <nodeoze/nany.h>
#include <nodeoze/nstream.h>
#include <nodeoze/nrpc.h>

namespace nodeoze {

namespace json {

struct parser_context;

std::string
deflate_to_string( const any &root, bool pretty = false );

template< typename T >
inline T&
deflate_to_stream( T &os, const any &root )
{
	switch ( root.type() )
	{
		case any::type_t::null:
		{
			os << "null";
		}
		break;
	
		case any::type_t::boolean:
		{
			os << ( root.to_bool() ? "true" : "false" );
		}
		break;
	
		case any::type_t::integer:
		{
			os << root.to_int64();
		}
		break;
	
		case any::type_t::floating:
		{
			os << root.to_floating();
		}
		break;
	
		case any::type_t::string:
		{
			os << '\"';
			
			for ( const auto &c : root.to_string() )
			{
				switch ( c )
				{
					// quotation mark (0x22)
					case '"':
					// reverse solidus (0x5c)
					case '\\':
					{
						os << '\\' << c;
					}
					break;

					// backspace (0x08)
					case '\b':
					{
						os << '\\' << 'b';
					}
					break;

					// formfeed (0x0c)
					case '\f':
					{
						os << '\\' << 'f';
					}
					break;

					// newline (0x0a)
					case '\n':
					{
						os << '\\' << 'n';
					}
					break;

					// carriage return (0x0d)
					case '\r':
					{
						os << '\\' << 'r';
					}
					break;

					// horizontal tab (0x09)
					case '\t':
					{
						os << '\\' << 't';
					}
					break;

					default:
					{
						if ( c >= 0x00 && c <= 0x1f )
						{
							char buf[ 8 ];
							
							// print character c as \uxxxx
#if defined( WIN32 )
							sprintf_s( buf, "\\u%04x", static_cast< int >( c ) );
#else
							sprintf( buf, "\\u%04x", static_cast< int >( c ) );
#endif
							os << buf;
						}
						else
						{
							os << c;
						}
					}
					break;
				}
			}
			
			os << '\"';
		}
		break;
		
		case any::type_t::blob:
		{
			os << '\"' << root.to_string() << '\"';
		}
		break;
	
		case any::type_t::array:
		{
			auto first = true;
			
			os << "[";
			
			for ( auto &child : root.to_array() )
			{
				if ( first ) first = false; else os << ",";
				deflate_to_stream( os, child );
			}
			
			os << "]";
		}
		break;
	
		case any::type_t::object:
		{
			auto first = true;
			
			os << "{";
			
			for ( auto &child : root.to_object() )
			{
				if ( first ) first = false; else os << ",";
				os << "\"" << child.first << "\":";
				deflate_to_stream( os, child.second );
			}
			
			os << "}";
		}
		break;
	}
	
	return os;
}

any
inflate( const std::string &s );

bool
inflate( any &root, const std::string &s );

void
link();

class parser
{
public:

	typedef std::function< void ( any &root ) > object_f;

	parser( any &root );
	
	~parser();
	
	void
	on_root_object( object_f func );
	
	std::error_code
	process( const std::uint8_t *buf, std::size_t len );
	
	any&
	root();
	
	const any&
	root() const;
	
protected:

	void
	reset();

	std::unique_ptr< parser_context >	m_context;
	any									&m_root;
};

namespace rpc {

class connection : public nodeoze::rpc::connection
{
public:

	typedef std::shared_ptr< connection > ptr;
	
	connection( const uri &resource );

	connection( ip::tcp::socket sock );

	connection( http::connection::ptr wrapped );
	
	virtual ~connection();

	virtual nodeoze::buffer
	deflate( const nodeoze::any &message );

protected:

	virtual std::error_code
	process( const buffer &buf );
	
	any		m_root;
	parser	m_parser;
};

}

}

}

#endif
