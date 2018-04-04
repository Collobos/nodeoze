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
  * This code was inspired by katzarsky@gmail.com
  */

#include <nodeoze/nws.h>
#include <nodeoze/ntls.h>
#include <nodeoze/nendian.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nbase64.h>
#include <nodeoze/nuuid.h>
#include <nodeoze/nlog.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nhttp.h>
#include <nodeoze/nsha1.h>
#include <http_parser.h>
#include <algorithm>
#include <assert.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <string>
#include <thread>

using namespace nodeoze;

class hasher
{
public:

	hasher();

	virtual ~hasher();

	void
	reset();

	bool
	result( unsigned *message_digest_array );

	void
	input( const unsigned char *message_array, unsigned length );

	void
	input( const char *message_array, unsigned length );

	void
	input( unsigned char message_element );

	void
	input( char message_element );

	hasher&
	operator<<(const char *message_array);

	hasher&
	operator<<(const unsigned char *message_array);

	hasher&
	operator<<(const char message_element);

	hasher&
	operator<<(const unsigned char message_element);

private:

	void
	process_message_block();

	void
	pad_message();

	inline unsigned
	circular_shift( int bits, unsigned word );

	unsigned		m_h[5];						// Message digest buffers

	unsigned		m_length_low;				// Message length in bits
	unsigned		m_length_high;				// Message length in bits

	unsigned char	m_message_block[64];		// 512-bit message blocks
	int				m_message_block_index;		// Index into message block array

	bool			m_computed;					// Is the digest computed?
	bool			m_corrupted;				// Is the message digest corruped?
};

hasher::hasher()
{
    reset();
}


hasher::~hasher()
{
    // The destructor does nothing
}


void
hasher::reset()
{
    m_length_low          = 0;
    m_length_high         = 0;
    m_message_block_index = 0;

    m_h[0]        = 0x67452301;
    m_h[1]        = 0xEFCDAB89;
    m_h[2]        = 0x98BADCFE;
    m_h[3]        = 0x10325476;
    m_h[4]        = 0xC3D2E1F0;

    m_computed    = false;
    m_corrupted   = false;
}


bool
hasher::result(unsigned *message_digest_array)
{
    int i;                                  // Counter

    if (m_corrupted)
    {
        return false;
    }

    if (!m_computed)
    {
        pad_message();
        m_computed = true;
    }

    for(i = 0; i < 5; i++)
    {
        message_digest_array[i] = m_h[i];
    }

    return true;
}


void
hasher::input( const unsigned char *message_array, unsigned length )
{
    if (!length)
    {
        return;
    }

    if (m_computed || m_corrupted)
    {
        m_corrupted = true;
        return;
    }

    while(length-- && !m_corrupted)
    {
        m_message_block[m_message_block_index++] = (*message_array & 0xFF);

        m_length_low += 8;
        m_length_low &= 0xFFFFFFFF;               // Force it to 32 bits
        if (m_length_low == 0)
        {
            m_length_high++;
            m_length_high &= 0xFFFFFFFF;          // Force it to 32 bits
            if (m_length_high == 0)
            {
                m_corrupted = true;               // Message is too long
            }
        }

        if (m_message_block_index == 64)
        {
            process_message_block();
        }

        message_array++;
    }
}


void
hasher::input( const char  *message_array, unsigned length )
{
	input((unsigned char *) message_array, length);
}


void
hasher::input(unsigned char message_element)
{
    input(&message_element, 1);
}


void
hasher::input( char message_element )
{
	input((unsigned char *) &message_element, 1);
}


hasher&
hasher::operator<<(const char *message_array)
{
    const char *p = message_array;

    while(*p)
    {
        input(*p);
        p++;
    }

    return *this;
}


hasher&
hasher::operator<<(const unsigned char *message_array)
{
    const unsigned char *p = message_array;

    while(*p)
    {
        input(*p);
        p++;
    }

    return *this;
}


hasher&
hasher::operator<<(const char message_element)
{
    input((unsigned char *) &message_element, 1);

    return *this;
}


hasher&
hasher::operator<<(const unsigned char message_element)
{
    input(&message_element, 1);

    return *this;
}


void
hasher::process_message_block()
{
    const unsigned K[] =    {               // Constants defined for SHA-1
                                0x5A827999,
                                0x6ED9EBA1,
                                0x8F1BBCDC,
                                0xCA62C1D6
                            };
    int         t;                          // Loop counter
    unsigned    temp;                       // Temporary word value
    unsigned    W[80];                      // Word sequence
    unsigned    A, B, C, D, E;              // Word buffers

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = ((unsigned) m_message_block[t * 4]) << 24;
        W[t] |= ((unsigned) m_message_block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) m_message_block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) m_message_block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = circular_shift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = m_h[0];
    B = m_h[1];
    C = m_h[2];
    D = m_h[3];
    E = m_h[4];

    for(t = 0; t < 20; t++)
    {
        temp = circular_shift(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = circular_shift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = circular_shift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circular_shift(30,B);
        B = A;
        A = temp;
    }

    m_h[0] = (m_h[0] + A) & 0xFFFFFFFF;
    m_h[1] = (m_h[1] + B) & 0xFFFFFFFF;
    m_h[2] = (m_h[2] + C) & 0xFFFFFFFF;
    m_h[3] = (m_h[3] + D) & 0xFFFFFFFF;
    m_h[4] = (m_h[4] + E) & 0xFFFFFFFF;

    m_message_block_index = 0;
}


void
hasher::pad_message()
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second block.
     */
    if (m_message_block_index > 55)
    {
        m_message_block[m_message_block_index++] = 0x80;
        while(m_message_block_index < 64)
        {
            m_message_block[m_message_block_index++] = 0;
        }

        process_message_block();

        while(m_message_block_index < 56)
        {
            m_message_block[m_message_block_index++] = 0;
        }
    }
    else
    {
        m_message_block[m_message_block_index++] = 0x80;
        while(m_message_block_index < 56)
        {
            m_message_block[m_message_block_index++] = 0;
        }

    }

    /*
     *  Store the message length as the last 8 octets
     */
    m_message_block[56] = (m_length_high >> 24) & 0xFF;
    m_message_block[57] = (m_length_high >> 16) & 0xFF;
    m_message_block[58] = (m_length_high >> 8) & 0xFF;
    m_message_block[59] = (m_length_high) & 0xFF;
    m_message_block[60] = (m_length_low >> 24) & 0xFF;
    m_message_block[61] = (m_length_low >> 16) & 0xFF;
    m_message_block[62] = (m_length_low >> 8) & 0xFF;
    m_message_block[63] = (m_length_low) & 0xFF;

    process_message_block();
}


unsigned
hasher::circular_shift(int bits, unsigned word)
{
    return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32-bits));
}


class ws_filter_impl : public ws::filter
{
public:

	struct frame
	{
		struct type_t
		{
			static const int error				= 0xFF00;
			static const int incomplete			= 0xFE00;
	
			static const int opening			= 0x3300;
			static const int closing			= 0x3400;

			static const int incomplete_text	= 0x01;
			static const int incomplete_binary	= 0x02;
	
			static const int text				= 0x81;
			static const int binary				= 0x82;
	
			static const int close				= 0x88;
			static const int ping				= 0x89;
			static const int pong				= 0x8A;
		};
	};

	ws_filter_impl( type_t type );
	
	ws_filter_impl( const std::string &key, type_t type );
	
	ws_filter_impl( const uri &resource, type_t type );
	
	virtual ~ws_filter_impl();
	
	virtual const std::string&
	name() const
	{
		static std::string s( "ws" );
		return s;
	}
	
	virtual stream::state_t
	state() const
	{
		return m_state;
	}
	
	virtual stream::state_t
	send( buffer &in_buf, buffer &out_buf );
		
	virtual stream::state_t
	recv( std::vector< buffer > &in_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs );
	
	virtual void
	reset();
	
	virtual void
	ping( buffer &out_buf );
	
	virtual void
	close( buffer &out_buf );
	
	bool
	check_client_handshake( http::message &request );
	
	void
	queue_server_handshake();
	
private:

	int
	check_server_handshake( buffer &buf, std::size_t *out_len );
	
	int
	check_client_handshake( buffer &buf, std::size_t *out_len );
	
	void
	make_server_handshake( buffer &buf );
	
	bool
	parse( buffer &buf, http::message &message );
	
	void
	make_frame( int type, std::uint8_t *msg, std::size_t msg_len, buffer &frame );

	int
	get_frame( std::uint8_t *in_buffer, std::size_t in_length, std::uint8_t *out_buffer, std::size_t out_size, std::size_t *out_length, std::size_t *out_parsed );

	inline std::size_t
	msg_size_to_frame_size( std::size_t msg_size )
	{
		if ( msg_size <= 125 )
		{
			return 1 + 1 + msg_size;
		}
		else if ( msg_size <= 65535 )
		{
			return 1 + 1 + 2 + msg_size;
		}
		else
		{
			return 1 + 1 + 8 + msg_size;
		}
	}
	
	void
	setup();
	
	void
	teardown();

protected:

	std::string
	accept_key( const std::string &key );
	
protected:

	std::queue< buffer >	m_pending_send_list;
	buffer					m_unparsed_recv_data;
	bool					m_error;
	role_t					m_role;
	
	bool					m_sent_handshake;
	bool					m_recv_handshake;
	stream::state_t			m_state;
	std::string				m_expected_key;
	uri						m_resource;
	std::string				m_key;
	type_t					m_type;
};


ws::filter*
ws::filter::create_client( const uri &resource, type_t type )
{
	return new ws_filter_impl( resource, type );
}


ws::filter*
ws::filter::create_server( type_t type )
{
	return new ws_filter_impl( type );
}


ws::filter*
ws::filter::create_server( http::message::ptr request, type_t type )
{
	ws_filter_impl	*filter = nullptr;
	std::string		key;
	bool			ok;
	
	ok = request->validate_ws_request( key );
	ncheck_error_quiet( ok, exit );
	
	filter = new ws_filter_impl( key, type );
	
exit:

	return filter;
}


ws::filter::filter()
{
}


ws::filter::~filter()
{
}


ws_filter_impl::ws_filter_impl( type_t type )
:
	m_error( false ),
	m_role( role_t::server ),
	m_sent_handshake( false ),
	m_recv_handshake( false ),
	m_state( stream::state_t::handshaking ),
	m_type( type )
{
}


ws_filter_impl::ws_filter_impl( const std::string &key, type_t type )
:
	m_error( false ),
	m_role( role_t::server ),
	m_sent_handshake( false ),
	m_recv_handshake( true ),
	m_state( stream::state_t::handshaking ),
	m_key( key ),
	m_type( type )
{
}


ws_filter_impl::ws_filter_impl( const uri &resource, type_t type )
:
	m_error( false ),
	m_role( role_t::client ),
	m_sent_handshake( false ),
	m_recv_handshake( false ),
	m_state( stream::state_t::handshaking ),
	m_resource( resource ),
	m_type( type )
{
	m_key = uuid().to_base64();
}


ws_filter_impl::~ws_filter_impl()
{
	mlog( marker::websocket, log::level_t::info, "" );
}


stream::state_t
ws_filter_impl::send( buffer &in_buf, buffer &out_buf )
{
	out_buf.clear();

	if ( m_sent_handshake && m_recv_handshake )
	{
		if ( in_buf.size() > 0 )
		{
			auto frame_type = ( m_type == type_t::text ) ? frame::type_t::text : frame::type_t::binary;
			make_frame( frame_type, in_buf.data(), in_buf.size(), out_buf );
			mlog( marker::websocket, log::level_t::info, "send data (% bytes)", out_buf.size() );
		}

		m_state = stream::state_t::connected;
	}
	else
	{
		if ( !m_sent_handshake )
		{
			switch ( m_role )
			{
				case role_t::client:
				{
					std::ostringstream os;
		
					os << "GET " << m_resource.path() << "?encoding=text HTTP/1.1\r\n";
					os << "Upgrade: websocket\r\n";
					os << "Connection: Upgrade\r\n";
					os << "Host: " << m_resource.host() << "\r\n";
					os << "Sec-WebSocket-Key: " << m_key << "\r\n";
					os << "Sec-WebSocket-Version: 13\r\n\r\n";
				
					m_expected_key = accept_key( m_key );
				
					m_state = stream::state_t::handshaking;
					out_buf.assign( os.str() );
					mlog( marker::websocket, log::level_t::info, "send handshake data (% bytes)", out_buf.size() );
					m_sent_handshake = true;
				}
				break;
				
				case role_t::server:
				{
					if ( m_recv_handshake )
					{
						std::ostringstream os;
		
						os << "HTTP/1.1 101 Switching Protocols\r\n";
						os << "Connection: Upgrade\r\n";
						os << "Upgrade: websocket\r\n";
						os << "Sec-WebSocket-Accept: " << accept_key( m_key ) << "\r\n";
						os << "\r\n";
					
						m_state = stream::state_t::connected;
						out_buf.assign( os.str() );
						mlog( marker::websocket, log::level_t::info, "send handshake data (% bytes)", out_buf.size() );
						m_sent_handshake = true;
					}
					else
					{
						m_state = stream::state_t::handshaking;
					}
				}
				break;
			}
		}
		else
		{
			m_state = stream::state_t::handshaking;
		}
		
		if ( in_buf.size() > 0 )
		{
			auto frame_type = ( m_type == type_t::text ) ? frame::type_t::text : frame::type_t::binary;
			buffer pending_buf;
			make_frame( frame_type, in_buf.data(), in_buf.size(), pending_buf );
			mlog( marker::websocket, log::level_t::info, "queue data while waiting for handshake (% bytes)", pending_buf.size() );
			m_pending_send_list.push( std::move( pending_buf ) );
		}
	}
	
	return m_state;
}


stream::state_t
ws_filter_impl::recv( std::vector< buffer > &in_bufs, buffer &out_send_buf, std::vector< buffer > &out_recv_bufs )
{
	m_state = stream::state_t::connected;
	
	assert( in_bufs.size() > 0 );
	
	for ( auto &it : in_bufs )
	{
		m_unparsed_recv_data.append( it );
	}
			
	out_send_buf.clear();
	out_recv_bufs.clear();
	
	while ( 1 )
	{
		if ( m_recv_handshake )
		{
			int				type		= frame::type_t::incomplete;
			std::size_t		payload_len	= 0;
			std::size_t		parsed_len	= 0;
		
			if ( m_unparsed_recv_data.size() > 0 )
			{
				std::vector< std::uint8_t > parsed_data( m_unparsed_recv_data.size() );

				type = get_frame( &m_unparsed_recv_data[ 0 ], m_unparsed_recv_data.size(), &parsed_data[ 0 ], parsed_data.size(), &payload_len, &parsed_len );
				
				if ( ( type == frame::type_t::text ) || ( type == frame::type_t::binary ) )
				{
					out_recv_bufs.emplace_back( parsed_data.data(), payload_len );
					
					mlog( marker::websocket, log::level_t::info, "recv data (% bytes)", out_recv_bufs.back().size() );
					
					if ( parsed_len == m_unparsed_recv_data.size() )
					{
						m_unparsed_recv_data.clear();
					}
					else
					{
						m_unparsed_recv_data.rotate( 0, parsed_len, m_unparsed_recv_data.size() );
						m_unparsed_recv_data.size( m_unparsed_recv_data.size() - parsed_len );
					}
				}
				else if ( ( type == frame::type_t::incomplete ) || ( type == frame::type_t::incomplete_text ) || ( type == frame::type_t::incomplete_binary ) )
				{
					break;
				}
				else if ( type == frame::type_t::ping )
				{
					mlog( marker::websocket, log::level_t::info, "ping frame" );
					
					if ( parsed_len == m_unparsed_recv_data.size() )
					{
						m_unparsed_recv_data.clear();
					}
					else
					{
						m_unparsed_recv_data.rotate( 0, parsed_len, m_unparsed_recv_data.size() );
						m_unparsed_recv_data.size( m_unparsed_recv_data.size() - parsed_len );
					}
					
					make_frame( frame::type_t::pong, nullptr, 0, out_send_buf );
				}
				else if ( type == frame::type_t::pong )
				{
					mlog( marker::websocket, log::level_t::info, "pong frame" );
					
					if ( parsed_len == m_unparsed_recv_data.size() )
					{
						m_unparsed_recv_data.clear();
					}
					else
					{
						m_unparsed_recv_data.rotate( 0, parsed_len, m_unparsed_recv_data.size() );
						m_unparsed_recv_data.size( m_unparsed_recv_data.size() - parsed_len );
					}
				}
				else if ( type == frame::type_t::close )
				{
					mlog( marker::websocket, log::level_t::info, "close frame" );
					m_state = stream::state_t::disconnected;
					break;
				}
				else if ( type == frame::type_t::error )
				{
					mlog( marker::websocket, log::level_t::info, "error frame" );
					m_state = stream::state_t::error;
					break;
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			std::size_t header_end = m_unparsed_recv_data.find( "\r\n\r\n", 4 );
			
			if ( header_end != buffer::npos )
			{
				switch ( m_role )
				{
					case role_t::client:
					{
						http::message	message;
						int				type;
						std::string		key;
						bool			ok;
	
						ok = parse( m_unparsed_recv_data, message );
						ncheck_error_quiet( ok, exit );
	
						ncheck_error_action_quiet( message.code() == http::code_t::switching_protocols, type = frame::type_t::error, exit );
	
						key = message.find( "Sec-WebSocket-Accept" );
						ncheck_error_action_quiet( key.size() > 0, type = frame::type_t::error, exit );
						ncheck_error_action_quiet( m_expected_key == key, type = frame::type_t::error, exit );
			
						m_recv_handshake = true;
			
						m_state = stream::state_t::connected;
					}
					break;
		
					case role_t::server:
					{
						http::message	message;
						buffer			dummy;
						int				type;
						std::string		val;
						bool			ok;
				
						ok = parse( m_unparsed_recv_data, message );
						ncheck_error_action_quiet( ok, type = frame::type_t::error, exit );
						
						ok = message.validate_ws_request( m_key );
						ncheck_error_action_quiet( ok, type = frame::type_t::error, exit );
						
						m_recv_handshake = true;
						
						assert( !m_sent_handshake );
						m_state = send( dummy, out_send_buf );
						assert( m_state == stream::state_t::connected );
						assert( m_sent_handshake );
					}
					break;
				}
	
				if ( ( header_end + 4 ) < m_unparsed_recv_data.size() )
				{
					m_unparsed_recv_data.rotate( 0, header_end + 4, m_unparsed_recv_data.size() - header_end - 4 );
				}
				else
				{
					m_unparsed_recv_data.clear();
				}
				
				while ( !m_pending_send_list.empty() )
				{
					out_send_buf.append( m_pending_send_list.front() );
					m_pending_send_list.pop();
				}
			}
		}
	}
	
exit:

	return m_state;
}


void
ws_filter_impl::reset()
{
	teardown();
	setup();
}


void
ws_filter_impl::ping( buffer &out_buf )
{
	make_frame( frame::type_t::ping, nullptr, 0, out_buf );
}


void
ws_filter_impl::close( buffer &out_buf )
{
	make_frame( frame::type_t::close, nullptr, 0, out_buf );
}


std::string
ws_filter_impl::accept_key( const std::string &key )
{
	std::string			output( key );
	std::uint8_t		digest[ 20 ];
	hasher				h;

	output += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	h.input( output.data(), ( unsigned int ) output.size() );
	h.result( reinterpret_cast< unsigned* >( digest ) );
	
	for ( int i = 0; i < 20; i += 4 )
	{
		unsigned char c;

		c = digest[i];
		digest[i] = digest[i+3];
		digest[i+3] = c;

		c = digest[i+1];
		digest[i+1] = digest[i+2];
		digest[i+2] = c;
	}
	
	output = codec::base64::encode( nodeoze::buffer{ std::string( digest, digest + 20 ) } );
	
	return output;
}


int
ws_filter_impl::check_client_handshake( buffer &buf, std::size_t *out_len )
{
	std::string		header		= buf.to_string();
	std::size_t		header_end	= header.find( "\r\n\r\n" );
	http::message	message;
	int				type;
	std::string		val;
	bool			ok;
	
	ncheck_error_action_quiet( header_end != std::string::npos, type = frame::type_t::incomplete, exit );
	
	header.resize( header_end );
	
	ok = parse( buf, message );
	ncheck_error_action_quiet( ok, type = frame::type_t::error, exit );
	
	ok = check_client_handshake( message );
	ncheck_error_action_quiet( ok, type = frame::type_t::error, exit );
	
	*out_len = header_end + 4;
	
	type = frame::type_t::opening;
	
exit:

	return type;
}


bool
ws_filter_impl::check_client_handshake( http::message &message )
{
	std::string val;
	
	val = message.find( "Upgrade" );
	ncheck_error_action_quiet( val.size() > 0, m_recv_handshake = false, exit );
	ncheck_error_action_quiet( string::case_insensitive_compare( val, "websocket" ), m_recv_handshake = false, exit );
	
	val = message.find( "Connection" );
	ncheck_error_action_quiet( val.size() > 0, m_recv_handshake = false, exit );
	ncheck_error_action_quiet( string::case_insensitive_compare( val, "Upgrade" ), m_recv_handshake = false, exit );
	
	val = message.find( "Sec-WebSocket-Version" );
	ncheck_error_action_quiet( val.size() > 0, m_recv_handshake = false, exit );
	ncheck_error_action_quiet( val == "13", m_recv_handshake = false, exit );
	
	m_key = message.find( "Sec-WebSocket-Key" );
	ncheck_error_action_quiet( m_key.size() > 0, m_recv_handshake = false, exit );
	
	m_recv_handshake = true;
	
exit:

	return m_recv_handshake;
}


void
ws_filter_impl::queue_server_handshake()
{
	buffer buf;
	
	make_server_handshake( buf );
	
	m_pending_send_list.emplace( std::move( buf ) );
}
	

int
ws_filter_impl::check_server_handshake( buffer &buf, std::size_t *out_len )
{
	std::string		header		= buf.to_string();
	std::size_t		header_end	= header.find( "\r\n\r\n" );
	http::message	message;
	int				type;
	std::string		key;
	bool			ok;
	
	ncheck_error_action_quiet( header_end != std::string::npos, type = frame::type_t::incomplete, exit );
	
	header.resize( header_end );
	
	ok = parse( buf, message );
	ncheck_error_action_quiet( ok, type = frame::type_t::error, exit );
	
	ncheck_error_action_quiet( message.code() == http::code_t::switching_protocols, type = frame::type_t::error, exit );
	
	key = message.find( "Sec-WebSocket-Accept" );
	ncheck_error_action_quiet( key.size() > 0, type = frame::type_t::error, exit );
	ncheck_error_action_quiet( m_expected_key == key, type = frame::type_t::error, exit );

	*out_len = header_end + 4;
	
	type = frame::type_t::opening;
	
exit:

	return type;
}


bool
ws_filter_impl::parse( buffer &buf, http::message &message )
{
	std::streamsize			processed;
	http_parser_settings	settings;
	http_parser				parser;
	bool					ok = false;
	
	memset( &settings, 0, sizeof( http_parser_settings ) );
	memset( &parser, 0, sizeof( http_parser ) );
	
	settings.on_message_begin = []( http_parser *parser )
	{
		http::message *message = reinterpret_cast< http::message* >( parser->data );
		message->on_begin();
		return 0;
	};
	
	settings.on_url = []( http_parser *parser, const char *buf, size_t len )
	{
		http::message *message = reinterpret_cast< http::message* >( parser->data );
		message->on_uri( buf, len );
		return 0;
	};
	
	settings.on_header_field = []( http_parser *parser, const char *buf, size_t len )
	{
		http::message *message = reinterpret_cast< http::message* >( parser->data );
		message->on_header_field( buf, len );
		return 0;
	};
	
	settings.on_header_value = []( http_parser *parser, const char *buf, size_t len )
	{
		http::message *message = reinterpret_cast< http::message* >( parser->data );
		message->on_header_value( buf, len );
		return 0;
	};
	
	settings.on_headers_complete = []( http_parser *parser )
	{
		http::message *message = reinterpret_cast< http::message* >( parser->data );
		message->on_headers();
		return 0;
	};
	
	settings.on_message_complete = []( http_parser *parser )
	{
		http::message *message = reinterpret_cast< http::message* >( parser->data );
		message->on_end();
		return 0;
	};
	
	http_parser_init( &parser, HTTP_BOTH );
	parser.data = &message;
	
	processed = http_parser_execute( &parser, &settings, reinterpret_cast< const char* >( buf.data() ), buf.size() );
	
	message.set_code( static_cast< http::code_t >( parser.status_code ) );
	
	if ( processed != std::streamsize( buf.size() ) )
	{
		mlog( marker::websocket, log::level_t::error, "unable to parse %", buf.to_string() );
		ok = false;
		goto exit;
	}
	
	ok = true;
	
exit:
	
	return ok;
}


void
ws_filter_impl::make_server_handshake( buffer &buf )
{
	std::ostringstream os;
		
	os << "HTTP/1.1 101 Switching Protocols\r\n";
	os << "Connection: Upgrade\r\n";
	os << "Upgrade: websocket\r\n";
	os << "Sec-WebSocket-Accept: " << accept_key( m_key ) << "\r\n";
	os << "\r\n";
	
	buf.assign( os.str() );
}


void
ws_filter_impl::make_frame( int type, std::uint8_t *msg, std::size_t msg_size, buffer &frame )
{
	int pos = 0;
	
	switch ( type )
	{
		case frame::type_t::text:
		case frame::type_t::binary:
		{
			frame.size( msg_size_to_frame_size( msg_size ) );
	
			frame[ pos++ ] = static_cast< std::uint8_t >( type );

			if ( msg_size <= 125 )
			{
				frame[ pos++ ] = static_cast< std::uint8_t >( msg_size );
			}
			else if ( msg_size <= 65535 )
			{
				frame[ pos++ ] = 126;
				codec::big_endian::put( frame.data() + pos, static_cast< std::uint16_t >( msg_size ) );
				pos += 2;
			}
			else
			{
				frame[ pos++ ] = 127;
				codec::big_endian::put( frame.data() + pos, static_cast< std::uint64_t >( msg_size ) );
				pos += 8;
			}
			
			frame.assign( msg, pos, msg_size );
		}
		break;
		
		case frame::type_t::ping:
		case frame::type_t::pong:
		case frame::type_t::close:
		{
			frame.size( 2 );
			frame[ pos++ ] = static_cast< std::uint8_t >( type );
			frame[ pos++ ] = 0;
		}
		break;
		
		default:
		{
			assert( 0 );
		}
		break;
	}
}


int
ws_filter_impl::get_frame( std::uint8_t *in_buffer, std::size_t in_length, std::uint8_t *out_buffer, std::size_t out_size, std::size_t *out_length, std::size_t *out_parsed )
{
	if ( in_length < 2 )
	{
		return frame::type_t::incomplete;
	}

	std::uint8_t	msg_opcode		= in_buffer[ 0 ] & 0x0F;
	std::uint8_t	msg_fin			= ( in_buffer[ 0 ] >> 7 ) & 0x01;
	std::uint8_t	msg_masked		= ( in_buffer[ 1 ] >> 7 ) & 0x01;
	std::uint64_t	payload_length	= 0;
	int				pos				= 2;
	int				length_field	= in_buffer[ 1 ] & ( ~0x80 );
	unsigned int	mask			= 0;

	if ( length_field <= 125 )
	{
		payload_length = length_field;
	}
	else if ( length_field == 126 )
	{
		std::uint16_t tmp;

		if ( in_length < 4 )
		{
			return frame::type_t::incomplete;
		}
		
		codec::big_endian::get( in_buffer + pos, tmp );
		pos += 2;
		
		payload_length = tmp;
	}
	else if ( length_field == 127 )
	{
		std::uint64_t tmp = 0;

		if ( in_length < 10 )
		{
			return frame::type_t::incomplete;
		}
		
		codec::big_endian::get( in_buffer + pos, tmp );
		pos += 8;
		
		payload_length = tmp;
	}

	if ( in_length < payload_length + pos )
	{
		return frame::type_t::incomplete;
	}

	if ( msg_masked )
	{
		mask = *( ( unsigned int* )( in_buffer + pos ) );
		pos += 4;

		std::uint8_t* c = in_buffer + pos;

		for ( auto i = 0u; i < payload_length; i++ )
		{
			c[ i ] = c[ i ] ^ ( ( std::uint8_t* )( &mask ) )[ i % 4 ];
		}
	}

	assert( payload_length <= out_size );

	memcpy( ( void* ) out_buffer, ( void* )( in_buffer + pos ), static_cast< std::size_t >( payload_length ) );
	*out_length	= static_cast< std::size_t >( payload_length );
	*out_parsed = static_cast< std::size_t >( pos + payload_length );

	if ( msg_opcode == 0x0 )
	{
		return ( msg_fin ) ? frame::type_t::text : frame::type_t::incomplete_text;
	}

	if( msg_opcode == 0x1 )
	{
		return ( msg_fin ) ? frame::type_t::text : frame::type_t::incomplete_text;
	}

	if ( msg_opcode == 0x2 )
	{
		return ( msg_fin ) ? frame::type_t::binary : frame::type_t::incomplete_binary;
	}

	if ( msg_opcode == 0x8 )
	{
		return frame::type_t::close;
	}

	if ( msg_opcode == 0x9 )
	{
		return frame::type_t::ping;
	}

	if ( msg_opcode == 0xA )
	{
		return frame::type_t::pong;
	}

	return frame::type_t::error;
}


void
ws_filter_impl::setup()
{
	m_error				= false;
	m_sent_handshake	= false;
	m_recv_handshake	= false;
	m_state				= stream::state_t::handshaking;
	
	if ( m_role == role_t::client )
	{
		m_key = uuid().to_base64();
	}
}


void
ws_filter_impl::teardown()
{
	while ( m_pending_send_list.size() > 0 )
	{
		m_pending_send_list.pop();
	}
	
	m_unparsed_recv_data.clear();
}
