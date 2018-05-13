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
/*
 * Copyright (c) 1996-1999 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
 
#include <nodeoze/address.h>
#include <nodeoze/runloop.h>
#include <nodeoze/unicode.h>
#include <nodeoze/machine.h>
#include <nodeoze/test.h>
#include <nodeoze/log.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <thread>
#include "uv.h"
#if defined( __linux__ )
#	include <arpa/inet.h>
#endif

using namespace nodeoze;

#define NS_INT16SZ			2
#define NS_INADDRSZ			4
#define NS_IN6ADDRSZ		16
#define INET_ADDRSTRLEN		16
#define INET6_ADDRSTRLEN	46

#ifdef SPRINTF_CHAR
#	define SPRINTF(x) strlen(sprintf/**/x)
#else
#	define SPRINTF(x) ((size_t)sprintf x)
#endif


#if defined( __APPLE__ )
#	pragma mark ip::address implementation
#endif


inline bool
operator==( sockaddr_storage s1, sockaddr_storage s2 )
{
    return memcmp( &s1, &s2, sizeof( sockaddr_storage ) ) == 0;
}

promise< std::vector< ip::address > >
ip::address::resolve( std::string host )
{
	promise< std::vector< ip::address > > ret;

	assert( host.size() > 0 );
	
	std::thread t( [=]()
	{
		std::vector< address >	addrs;
		struct addrinfo			*result;
		struct addrinfo			*res;
		std::ostringstream		os;
		int						err;
		
		err = getaddrinfo( host.c_str(), "0", NULL, &result );
    
		if ( err == 0 )
		{
			std::deque< struct sockaddr_storage > natives;

			for ( res = result; res != NULL; res = res->ai_next)
			{
				std::deque< struct sockaddr_storage >::iterator it;
				struct sockaddr_storage							storage;

				memset( &storage, 0, sizeof( storage ) );
				memcpy( &storage, res->ai_addr, res->ai_addrlen );
				
				it = std::find( natives.begin(), natives.end(), storage );
				
				if ( it == natives.end() )
				{
					if ( res->ai_family == AF_INET )
					{
						addrs.emplace_back( ( ( struct sockaddr_in* ) res->ai_addr )->sin_addr );
					}
					else if ( res->ai_family == AF_INET6 )
					{
						addrs.emplace_back( ( ( struct sockaddr_in6* ) res->ai_addr )->sin6_addr );
					}
					
					natives.push_back( storage );
				}
			}
 
			freeaddrinfo( result );
		}

		runloop::shared().dispatch( [=]() mutable
		{
			switch ( err )
			{
				case 0:
				{
					ret.resolve( std::move( addrs ) );
				}
				break;

#if !defined( WIN32 )

				case EAI_ADDRFAMILY:
				{
					ret.reject( make_error_code( std::errc::address_family_not_supported ), reject_context );
				}
				break;

#endif

				case EAI_AGAIN:
				{
					ret.reject( make_error_code( std::errc::resource_unavailable_try_again ), reject_context );
				}
				break;

				case EAI_BADFLAGS:
				{
					ret.reject( make_error_code( std::errc::invalid_argument ), reject_context );
				}
				break;

				case EAI_FAIL:
				{
					ret.reject( make_error_code( std::errc::bad_address ), reject_context );
				}
				break;

				case EAI_FAMILY:
				{
					ret.reject( make_error_code( std::errc::address_family_not_supported ), reject_context );
				}
				break;

				case EAI_MEMORY:
				{
					ret.reject( make_error_code( std::errc::not_enough_memory ), reject_context );
				}
				break;

				case EAI_NODATA:
				{
					ret.reject( make_error_code( std::errc::no_message ), reject_context );
				}
				break;

#if !defined( WIN32 )

				case EAI_NONAME:
				{
					ret.reject( make_error_code( std::errc::no_message ), reject_context );
				}
				break;

#endif

				case EAI_SERVICE:
				{
					ret.reject( make_error_code( std::errc::wrong_protocol_type ), reject_context );
				}
				break;

				case EAI_SOCKTYPE:
				{
					ret.reject( make_error_code( std::errc::wrong_protocol_type ), reject_context );
				}
				break;

#if !defined( WIN32 )

				case EAI_SYSTEM:
				{
					ret.reject( std::error_code( errno, std::generic_category() ), reject_context );
				}
				break;

#endif
				
				default:
				{
					ret.reject( make_error_code( std::errc::host_unreachable ), reject_context );
				}
			}
		} );
	} );
	
	t.detach();

	return ret;
}


const ip::address&
ip::address::v4_loopback()
{
	static std::uint8_t bytes[] = { 127, 0, 0, 1 };
	static ip::address a( bytes );
	return a;
}


const ip::address&
ip::address::v6_loopback()
{
	static std::uint8_t bytes[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };
	static ip::address a( bytes );
	return a;
}

	
const ip::address&
ip::address::v4_any()
{
	static ip::address a( type_t::v4 );
	return a;
}

	
const ip::address&
ip::address::v6_any()
{
	static ip::address a( type_t::v6 );
	return a;
}


ip::address::address( const char *val )
:
	m_type( type_t::unknown )
{
	memset( &m_addr, 0, sizeof( m_addr ) );
	from_string( val );
}


ip::address::address( const std::string &val )
:
	m_type( type_t::unknown )
{
	memset( &m_addr, 0, sizeof( m_addr ) );
	from_string( val );
}


ip::address::address( const any &root )
:
	m_type( type_t::unknown )
{
	memset( &m_addr, 0, sizeof( m_addr ) );
	
	if ( root.is_array() )
	{
		if ( root.size() == 1 )
		{
			m_type			= type_t::v4;
			m_addr.m_l[ 0 ] = root[ 0 ].to_uint32();
		}
		else if ( root.size() == 4 )
		{
			auto is_greater_than_255 = []( std::uint32_t val ) -> bool
			{
				return ( val > 255 );
			};
			
			if ( is_greater_than_255( root[ 0 ].to_uint32() ) ||
				 is_greater_than_255( root[ 1 ].to_uint32() ) ||
				 is_greater_than_255( root[ 2 ].to_uint32() ) ||
			     is_greater_than_255( root[ 3 ].to_uint32() ) )
			{
				m_type			= type_t::v6;
				m_addr.m_l[ 0 ] = root[ 0 ].to_uint32();
				m_addr.m_l[ 1 ] = root[ 1 ].to_uint32();
				m_addr.m_l[ 2 ] = root[ 2 ].to_uint32();
				m_addr.m_l[ 3 ] = root[ 3 ].to_uint32();
			}
			else if ( ( root[ 0 ].to_uint8() == 0 ) &&
					  ( root[ 1 ].to_uint8() == 0 ) &&
			          ( root[ 2 ].to_uint8() == 0 ) &&
			          ( root[ 3 ].to_uint8() == 1 ) )
			{
				m_type			= type_t::v6;
				m_addr.m_l[ 0 ] = 0;
				m_addr.m_l[ 1 ] = 0;
				m_addr.m_l[ 2 ] = 0;
				m_addr.m_l[ 3 ] = 0;
				m_addr.m_b[ 15 ] = 1;
			}
			else
			{
				m_type			= type_t::v4;
				m_addr.m_b[ 0 ] = root[ 0 ].to_uint8();
				m_addr.m_b[ 1 ] = root[ 1 ].to_uint8();
				m_addr.m_b[ 2 ] = root[ 2 ].to_uint8();
				m_addr.m_b[ 3 ] = root[ 3 ].to_uint8();
			}
		}
		else if ( root.size() == 16 )
		{
			m_type			= type_t::v6;
			m_addr.m_b[ 0 ] = root[ 0 ].to_uint8();
			m_addr.m_b[ 1 ] = root[ 1 ].to_uint8();
			m_addr.m_b[ 2 ] = root[ 2 ].to_uint8();
			m_addr.m_b[ 3 ] = root[ 3 ].to_uint8();
			m_addr.m_b[ 4 ] = root[ 4 ].to_uint8();
			m_addr.m_b[ 5 ] = root[ 5 ].to_uint8();
			m_addr.m_b[ 6 ] = root[ 6 ].to_uint8();
			m_addr.m_b[ 7 ] = root[ 7 ].to_uint8();
			m_addr.m_b[ 8 ] = root[ 8 ].to_uint8();
			m_addr.m_b[ 9 ] = root[ 9 ].to_uint8();
			m_addr.m_b[ 10 ] = root[ 10 ].to_uint8();
			m_addr.m_b[ 11 ] = root[ 11 ].to_uint8();
			m_addr.m_b[ 12 ] = root[ 12 ].to_uint8();
			m_addr.m_b[ 13 ] = root[ 13 ].to_uint8();
			m_addr.m_b[ 14 ] = root[ 14 ].to_uint8();
			m_addr.m_b[ 15 ] = root[ 15 ].to_uint8();
		}
	}
}


any
ip::address::to_any() const
{
	any root;
	
	if ( m_type == type_t::v4 )
	{
		root.emplace_back( m_addr.m_b[ 0 ] );
		root.emplace_back( m_addr.m_b[ 1 ] );
		root.emplace_back( m_addr.m_b[ 2 ] );
		root.emplace_back( m_addr.m_b[ 3 ] );
	}
	else if ( m_type == type_t::v6 )
	{
		root.emplace_back( m_addr.m_b[ 0 ] );
		root.emplace_back( m_addr.m_b[ 1 ] );
		root.emplace_back( m_addr.m_b[ 2 ] );
		root.emplace_back( m_addr.m_b[ 3 ] );
		root.emplace_back( m_addr.m_b[ 4 ] );
		root.emplace_back( m_addr.m_b[ 5 ] );
		root.emplace_back( m_addr.m_b[ 6 ] );
		root.emplace_back( m_addr.m_b[ 7 ] );
		root.emplace_back( m_addr.m_b[ 8 ] );
		root.emplace_back( m_addr.m_b[ 9 ] );
		root.emplace_back( m_addr.m_b[ 10 ] );
		root.emplace_back( m_addr.m_b[ 11 ] );
		root.emplace_back( m_addr.m_b[ 12 ] );
		root.emplace_back( m_addr.m_b[ 13 ] );
		root.emplace_back( m_addr.m_b[ 14 ] );
		root.emplace_back( m_addr.m_b[ 15 ] );
	}
	
	return root;
}


std::string
ip::address::to_string() const
{
	std::ostringstream os;
	
	if ( is_v4() )
	{
		to_v4( os );
	}
	else if ( is_v6() )
	{
		to_v6( os );
	}
	
	return os.str();
}


std::string
ip::address::to_reverse_string() const
{
	std::ostringstream os;
	
	if ( is_v4() )
	{
		to_reverse_v4( os );
	}
	else if ( is_v6() )
	{
		to_reverse_v6( os );
	}
	
	return os.str();
}


void
ip::address::to_v4( std::ostream &os ) const
{
	os << static_cast< std::uint32_t >( m_addr.m_b[ 0 ] ) << '.' << static_cast< std::uint32_t >( m_addr.m_b[ 1 ] ) << '.' << static_cast< std::uint32_t >( m_addr.m_b[ 2 ] ) << '.' << static_cast< std::uint32_t >( m_addr.m_b[ 3 ] );
}


void
ip::address::to_reverse_v4( std::ostream &os ) const
{
	os << static_cast< std::uint32_t >( m_addr.m_b[ 3 ] ) << '.' << static_cast< std::uint32_t >( m_addr.m_b[ 2 ] ) << '.' << static_cast< std::uint32_t >( m_addr.m_b[ 1 ] ) << '.' << static_cast< std::uint32_t >( m_addr.m_b[ 0 ] );
}


void
ip::address::to_v6( std::ostream &os ) const
{
	char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
	
	struct
	{
		int base, len;
	} best, cur;
	
	u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
	int i;

	/*
	* Preprocess:
	*      Copy the input (bytewise) array into a wordwise array.
	*      Find the longest run of 0x00's in src[] for :: shorthanding.
	*/
	memset(words, '\0', sizeof words);
	
	for (i = 0; i < NS_IN6ADDRSZ; i += 2)
	{
		words[i / 2] = (m_addr.m_b[i] << 8) | m_addr.m_b[i + 1];
	}
	
	best.base	= -1;
	cur.base	= -1;
	best.len	= 0;
	cur.len		= 0;
	
	for ( i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++ )
	{
		if (words[i] == 0 )
		{
			if (cur.base == -1)
				cur.base = i, cur.len = 1;
			else
				cur.len++;
		}
		else
		{
			if (cur.base != -1)
			{
				if (best.base == -1 || cur.len > best.len)
					best = cur;
				cur.base = -1;
			}
		}
	}
	
	if (cur.base != -1)
	{
		if (best.base == -1 || cur.len > best.len)
			best = cur;
	}
	
	if (best.base != -1 && best.len < 2)
		best.base = -1;

	/*
	* Format the result.
	*/
	
	tp = tmp;
	
	for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
	{
		/* Are we inside the best run of 0x00's? */
		if (best.base != -1 && i >= best.base && i < (best.base + best.len))
		{
			if (i == best.base)
			{
				os << ':';
				*tp++ = ':';
			}
			continue;
		}
	
		/* Are we following an initial run of 0x00s or any real hex? */
		if (i != 0)
		{
			os << ':';
			*tp++ = ':';
		}
		
		/* Is this address an encapsulated IPv4? */
		if (i == 6 && best.base == 0 && (best.len == 6 || (best.len == 5 && words[5] == 0xffff)))
		{
			os << m_addr.m_b[ 15 ] << '.' << m_addr.m_b[ 14 ] << m_addr.m_b[ 13 ] << '.' << m_addr.m_b[ 12 ];
			tp += strlen(tp);
			break;
		}
		
		os << std::hex << words[ i ] << std::dec;
	
		tp += SPRINTF((tp, "%x", words[i]));
	}
	
	/* Was it a trailing run of 0x00's? */
	if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
	{
		os << ':';
		*tp++ = ':';
	}
	
	*tp++ = '\0';

	/*
	* Check for overflow, copy, and we're done.
	*/
}



void
ip::address::to_reverse_v6( std::ostream &os ) const
{
	for ( auto i = 15; i >= 0; i-- )
	{
		os << m_addr.m_b[ i ] << ".";
	}
	
	os << "ip6.arpa";
}


/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal, octal (with the
 *	exception of 0) and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
void
ip::address::from_string( const std::string &s )
{
	if ( s.find( ':' ) == std::string::npos )
	{
		from_v4( s );
	}
	else
	{
		from_v6( s );
	}
}


void
ip::address::from_v4( const std::string &s )
{
	int saw_digit, octets, ch;
	u_char tmp[NS_INADDRSZ], *tp;
	char * src = const_cast< char* >( s.c_str() );

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {

		if (ch >= '0' && ch <= '9') {
			u_int new_int = *tp * 10 + (ch - '0');

			if (saw_digit && *tp == 0)
				goto exit;
			if (new_int > 255)
				goto exit;
			*tp = new_int;
			if (! saw_digit) {
				if (++octets > 4)
					goto exit;
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				goto exit;
			*++tp = 0;
			saw_digit = 0;
		} else
			goto exit;
	}
	if (octets < 4)
		goto exit;
	m_type = type_t::v4;
	memcpy( &m_addr, tmp, NS_INADDRSZ );
	
exit:

	return;
}

/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
void
ip::address::from_v6( const std::string &s )
{
	static const char xdigits[] = "0123456789abcdef";
	u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
	const char *curtok;
	int ch, saw_xdigit;
	u_int val;
	char * src = const_cast< char* >( s.c_str() );

	memset(tmp, '\0', NS_IN6ADDRSZ);
	tp = tmp;
	endp = tp + NS_IN6ADDRSZ;
	colonp = NULL;
	/* Leading :: requires some special handling. */
	if (*src == ':')
		if (*++src != ':')
			goto exit;
	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = tolower (*src++)) != '\0') {
		const char *pch;

		pch = strchr(xdigits, ch);
		if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if (val > 0xffff)
				goto exit;
			saw_xdigit = 1;
			continue;
		}
		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					goto exit;
				colonp = tp;
				continue;
			} else if (*src == '\0') {
				goto exit;
			}
			if (tp + NS_INT16SZ > endp)
				goto exit;
			*tp++ = (u_char) (val >> 8) & 0xff;
			*tp++ = (u_char) val & 0xff;
			saw_xdigit = 0;
			val = 0;
			continue;
		}
		if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) )
		{
		#if 0
		    inet_pton4(curtok, tp) > 0) {
			tp += NS_INADDRSZ;
			saw_xdigit = 0;
			break;	/* '\0' was seen by inet_pton4(). */
		#endif
		}
		
		goto exit;
	}
	if (saw_xdigit) {
		if (tp + NS_INT16SZ > endp)
			goto exit;
		*tp++ = (u_char) (val >> 8) & 0xff;
		*tp++ = (u_char) val & 0xff;
	}
	if (colonp != NULL) {
		/*
		 * Since some memmove()'s erroneously fail to handle
		 * overlapping regions, we'll do the shift by hand.
		 */
		const long n = static_cast< const long >( tp - colonp );
		int i;

		if (tp == endp)
			goto exit;
		for (i = 1; i <= n; i++) {
			endp[- i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}
	if (tp != endp)
		goto exit;
	
	m_type = type_t::v6;
	memcpy( &m_addr, tmp, NS_IN6ADDRSZ);
	
exit:

	return;
}


TEST_CASE( "nodeoze/smoke/address" )
{
	SUBCASE( "less than" )
	{
		ip::address lhs( ip::address::type_t::unknown );
		ip::address rhs( ip::address::type_t::unknown );
		
		CHECK( !( lhs < rhs ) );
		
		rhs = ip::address( ip::address::type_t::v4 );
		
		CHECK( lhs < rhs );
		
		lhs = ip::address( ip::address::type_t::v4 );
		
		CHECK( !( lhs < rhs ) );
		
		rhs = ip::address::v4_loopback();
		
		CHECK( lhs < rhs );
		
		rhs = ip::address::v6_any();
		
		CHECK( lhs < rhs );
		
		lhs = ip::address::v6_any();
		
		CHECK( !( lhs < rhs ) );
	}

	SUBCASE( "resolve correctly" )
	{
		if ( machine::self().has_internet_connection() )
		{
			runloop::shared().run( runloop::mode_t::nowait );

			auto done = std::make_shared< bool >( false );

			ip::address::resolve( "xtheband.com" )
			.then( [=]( std::vector< ip::address > addresses )
			{
				CHECK( addresses.size() > 0 );
				*done = true;
			},
			[=]( std::error_code err ) 
			{
				nunused( err );

				CHECK( 0 );
			} );

			while ( !done )
			{
				runloop::shared().run( runloop::mode_t::once );
			}
		}
	}

	SUBCASE( "resolve incorrectly" )
	{
		auto done = std::make_shared< bool >( false );

		ip::address::resolve( "thishostdoesnotexist.xyz" )
		.then( [=]( std::vector< ip::address > addresses )
		{
			nunused( addresses );
			assert( 0 );
		},
		[=]( std::error_code err ) 
		{
			CHECK( err.value() != 0 );
			*done = true;
		} );

		while ( !done )
		{
			runloop::shared().run( runloop::mode_t::once );
		}
	}
	
	SUBCASE( "check v4 loopback" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( 127 );
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v4() );
		CHECK( a.is_loopback_v4() );
	}
	
	SUBCASE( "check v6 loopback" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v6() );
		CHECK( a.is_loopback_v6() );
	}
	
	SUBCASE( "check v4 link local" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( 169 );
		bytes.push_back( 254 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v4() );
		CHECK( a.is_link_local_v4() );
	}
	
	SUBCASE( "check v6 link local" )
	{
		nodeoze::any bytes;
		
		bytes.push_back( htons( 0xfe80 ) );
		bytes.push_back( 0 );
		bytes.push_back( 0 );
		bytes.push_back( 1 );
		
		ip::address a( bytes );
		
		CHECK( a.is_v6() );
		CHECK( a.is_link_local_v6() );
	}
}
