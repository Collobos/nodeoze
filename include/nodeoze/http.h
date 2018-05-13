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
 
#ifndef _nodeoze_http_h
#define _nodeoze_http_h

#include <nodeoze/connection.h>
#include <nodeoze/notification.h>
#include <nodeoze/proxy.h>
#include <nodeoze/uri.h>
#include <nodeoze/string.h>
#include <nodeoze/types.h>
#include <nodeoze/ws.h>
#include <unordered_map>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <list>
#include <map>

#if defined( major )
#	undef major
#endif

#if defined( minor )
#	undef minor
#endif

struct http_parser_settings;
struct http_parser;

namespace nodeoze {

namespace http {

enum class method_t
{
	delet,
	get,
	head,
	post,
	put,
	connect,
	options,
	trace,
	copy,
	lock,
	mkcol,
	move,
	propfind,
	proppatch,
	search,
	unlock,
	report,
	mkactivity,
	checkout,
	merge,
	msearch,
	notify,
	subscribe,
	unsubscribe,
	patch,
	purge
};

enum class code_t
{
	cont					= 100,
	switching_protocols		= 101,
	ok						= 200,
	created					= 201,
	accepted				= 202,
	not_authoritative		= 203,
	no_content				= 204,
	reset_content			= 205,
	partial_content			= 206,
	multiple_choices		= 300,
	moved_permanently		= 301,
	moved_temporarily		= 302,
	see_other				= 303,
	not_modified			= 304,
	use_proxy				= 305,
	bad_request				= 400,
	unauthorized			= 401,
	payment_required		= 402,
	forbidden				= 403,
	not_found				= 404,
	method_not_allowed		= 405,
	not_acceptable			= 406,
	proxy_authentication	= 407,
	request_timeout			= 408,
	conflict				= 409,
	gone					= 410,
	length_required			= 411,
	precondition			= 412,
	request_too_large		= 413,
	uri_too_long			= 414,
	unsupported_media_type	= 415,
	requested_range			= 416,
	expectation_failed		= 417,
	upgrade_required		= 426,
	site_name_conflict		= 452,
	server_error			= 500,
	not_implemented			= 501,
	bad_gateway				= 502,
	service_unavailable		= 503,
	gateway_timeout			= 504,
	not_supported			= 505,
	authorized_cancelled	= 1000,
	pki_error				= 1001,
	webif_disabled			= 1002
};


extern const std::string&
to_string( code_t c );


extern const std::string&
to_string( method_t m );


class message
{
public:

	enum class type_t
	{
		request = 1,
		response,
	};

	typedef std::function< bool ( uint32_t status ) >							auth_f;
	typedef std::map< std::string, std::string, string::case_insensitive_less >	header_type;
	typedef std::shared_ptr< message >											ptr;
	
public:

	message();
	
	message( method_t method, const uri &uri );			// construct a normal request
	
	message( const message &request, code_t code );		// construct a response
	
	message( const message &rhs );						// copy constructor

	message( message &&rhs );							// move constructor
	
	virtual ~message();

	message&
	operator=( const message &rhs );

	message&
	operator=( message &&rhs );
	
	virtual void
	preflight();
	
	virtual void
	prologue( std::ostream &os ) const;
	
	inline method_t
	method() const
	{
		return m_method;
	}
	
	inline void
	set_method( method_t val )
	{
		m_method = val;
	}
	
	inline class uri&
	resource()
	{
		return m_resource;
	}
	
	inline const class uri&
	resource() const
	{
		return m_resource;
	}
	
	inline void
	set_resource( const uri &resource )
	{
		m_resource = resource;
	}
	
	inline code_t
	code() const
	{
		return m_code;
	}
	
	inline void
	set_code( code_t val )
	{
		m_code = val;
	}
	
	inline std::uint16_t
	major() const
	{
		return m_major;
	}
	
	inline void
	set_major( std::uint16_t val )
	{
		m_major = val;
	}
	
	inline std::uint16_t
	minor() const
	{
		return m_minor;
	}
	
	inline void
	set_minor( std::uint16_t val )
	{
		m_minor = val;
	}
	
	inline const header_type&
	header() const
	{
		return m_header;
	}
	
	void
	add_basic_authentication( const std::string &username, const std::string &password );
	
	virtual void
	header( std::ostream &os ) const;
	
	void
	add_header_fields( const header_type &header );

	inline void
	add_header_field( const std::string &key, int val )
	{
		add_header_field( key, std::to_string( val ) );
	}

	void
	add_header_field( const std::string &key, const std::string &val );
	
	inline void
	add_proxy_authorization_header_fields()
	{
		add_header_field( "Proxy-Authorization", "basic " + proxy::manager::shared().authorization() );
		add_header_field( "Proxy-Connection", "keep-alive" );
	}
	
	bool
	validate_ws_request( std::string &key ) const;
	
	std::string
	find( const std::string &key ) const;

	void
	remove_header_field( const std::string &key );
	
	inline const std::string&
	content_type() const
	{
		return m_content_type;
	}
	
	inline void
	set_content_type( const std::string &val )
	{
		m_content_type = val;
	}
	
	inline size_t
	content_length() const
	{
		return m_content_length;
	}
	
	inline const std::string&
	expect() const
	{
		return m_expect;
	}
	
	inline const std::string&
	user_agent() const
	{
		return m_user_agent;
	}

	inline const std::string&
	upgrade() const
	{
		return m_upgrade;
	}

	inline const std::string&
	ws_key() const
	{
		return m_ws_key;
	}

	inline bool
	keep_alive() const
	{
		return m_keep_alive;
	}

	inline void
	set_keep_alive( bool val )
	{
		m_keep_alive = val;
	}
	
	inline std::uint32_t
	max_redirects() const
	{
		return m_max_redirects;
	}

	inline void
	set_max_redirects( std::uint32_t val )
	{
		m_max_redirects = val;
	}

	inline bool
	can_redirect()
	{
		return ( m_num_redirects < m_max_redirects ) ? true : false;
	}

	inline std::uint32_t&
	num_redirects()
	{
		return m_num_redirects;
	}
	
	inline bool
	is_redirect() const
	{
		return m_is_redirect;
	}
	
	inline void
	set_is_redirect( bool val )
	{
		m_is_redirect = val;
	}
	
	inline const std::string&
	username() const
	{
		return m_username;
	}
	
	inline const std::string&
	password() const
	{
		return m_password;
	}
	
	inline const nodeoze::ip::endpoint&
	from() const
	{
		return m_from;
	}
	
	inline void
	set_from( const nodeoze::ip::endpoint &from )
	{
		m_from = from;
	}
	
	inline bool
	has_body() const
	{
		return ( ( m_code == http::code_t::moved_permanently ) || ( m_code == http::code_t::moved_temporarily ) || ( m_code == http::code_t::proxy_authentication ) ) ? false : true;
	}
	
	virtual void
	write( const uint8_t *buf, size_t len );
	
	inline nodeoze::buffer&
	body()
	{
		return m_body;
	}
	
	inline const nodeoze::buffer&
	body() const
	{
		return m_body;
	}
	
	virtual promise< void >
	body( std::ostream &os ) const;
	
	template < class U >
	inline message&
	operator<<( U t )
	{
		std::ostringstream os;
		os << t;
		m_body.append( os.str().c_str(), os.str().size() );
		return *this;
	}
	
	inline message&
	operator<<( message& ( *func )( message& ) )
	{
		return func( *this );
	}
	
	inline bool
	redirect() const
	{
		return m_redirect;
	}

	inline void
	set_redirect( bool val )
	{
		m_redirect = val;
	}

	inline int32_t
	tries() const
	{
		return m_tries;
	}
	
	inline void
	new_try()
	{
		++m_tries;
	}

	void
	auth( int status );

	inline void
	on_auth( auth_f auth )
	{
		m_auth = auth;
	}
	
	
	
	inline type_t
	type() const
	{
		return m_type;
	}
	
	inline void
	set_type( type_t type )
	{
		m_type = type;
	}
	
	inline message::ptr&
	link()
	{
		return m_link;
	}
	
	inline const message::ptr&
	link() const
	{
		return m_link;
	}
	
	inline void
	set_link( const message::ptr &link )
	{
		m_link = link;
	}
	
	// The following methods are for interfacing with joyent's HTTP parser
	
	virtual void
	on_begin();
	
	virtual void
	on_uri( const char *buf, std::size_t len );
	
	virtual void
	on_header_field( const char *buf, std::size_t len );
	
	virtual void
	on_header_value( const char *buf, std::size_t len );
	
	virtual void
	on_headers();
	
	virtual void
	on_body( nodeoze::buffer &buf );
	
	virtual void
	on_end();
	
	virtual std::shared_ptr< message >
	create() const;
	
protected:

	enum
	{
		NONE = 0,
		FIELD,
		VALUE
	};
	
	method_t				m_method;
	uri						m_resource;
	code_t					m_code;
	std::uint16_t			m_major				= 1;
	std::uint16_t			m_minor				= 1;
	header_type				m_header;
	std::string				m_content_type		= "text/plain";
	size_t					m_content_length	= 0;
	std::string				m_upgrade;
	std::string				m_ws_key;
	bool					m_keep_alive;
	nodeoze::buffer			m_body;
	
	message::ptr			m_link;
	std::uint32_t			m_max_redirects		= 3;
	std::uint32_t			m_num_redirects		= 0;
	auth_f					m_auth;
	
	bool					m_is_redirect		= false;

	std::string				m_resource_value;
	int						m_parse_state;
	std::string				m_header_field;
	std::string				m_header_value;
	std::string				m_peer_host;
	bool					m_redirect;
	std::string				m_host;
	std::string				m_expect;
	std::string				m_user_agent;
	std::string				m_authorization;
	std::string				m_username;
	std::string				m_password;
	nodeoze::ip::endpoint	m_from;
	std::int32_t			m_tries;
	type_t					m_type;
};


class connection : public nodeoze::connection, public std::streambuf
{
public:

	typedef std::function< void ( nodeoze::buffer &buf ) >										on_body_handler_f;
	typedef std::function< void ( buffer buf, bool close ) >									websocket_reply_f;
	typedef std::function< void ( const buffer &buf, websocket_reply_f ) >						websocket_handler_f;
	typedef std::function< void ( void ) >														will_close_f;
	
	typedef std::shared_ptr< connection > ptr;
	typedef std::list< ptr > list;

	connection( const nodeoze::uri &resource );
	
	connection( ip::tcp::socket sock );

	connection( connection::ptr wrapped );

	virtual ~connection();
	
	bool
	upgrade_to_tls( const message &request, std::function< void ( http::message &response, bool close ) > reply );
	
	bool
	upgrade_to_websocket( nodeoze::ws::filter::type_t type, websocket_handler_f handler );

	int
	method() const;

	int
	status_code() const;

	int
	http_major() const;
	
	int
	http_minor() const;
	
	http::message::ptr&
	out_message()
	{
		return m_out_message;
	}
	
	http::message::ptr&
	in_message()
	{
		return m_in_message;
	}
	
	inline connection&
	operator<<( connection& ( *func )( connection& ) )
	{
		return func( *this );
	}
	
	promise< message::ptr >
	send_request( message::ptr &request );
	
	inline void
	on_body_handler( on_body_handler_f handler )
	{
		m_on_body_handler = handler;
	}
	
	promise< void >
	send_response( message &response );
	
	virtual void
	close();
	
	inline void*
	data()
	{
		return m_data;
	}
	
	inline void
	set_data( void *v )
	{
		m_data = v;
	}

protected:

	static const std::size_t default_buffer_size;

	connection();

	virtual nodeoze::uri
	destination() const;
	
	virtual std::streamsize
	xsputn( const char* s, std::streamsize n );
	
	int
	flush();

	virtual int
	overflow ( int c = EOF );
	
    virtual int
	sync();

	virtual std::error_code
	process( const buffer &buf );
	
	static int
	message_will_begin( http_parser *parser );

	static int
	uri_was_received( http_parser *parser, const char *buf, size_t len );

	static int
	header_field_was_received( http_parser *parser, const char *buf, size_t len );

	static int
	header_value_was_received( http_parser *parser, const char *buf, size_t len );
	
	static int
	headers_were_received( http_parser *parser );
	
	static int
	body_was_received( http_parser *parser, const char *buf, size_t len );
	
	static int
	message_was_received( http_parser *parser );
	
	void
	really_send_request( message::ptr &message, promise< message::ptr > ret );
	
	void
	maybe_invoke_reply( std::error_code error, http::message::ptr &response );
	
	void
	init();
	
	void
	setup_buffer();
	
	friend class							server;
	
	time_t									m_start;
	bool									m_okay;
	std::vector< std::uint8_t >				m_body;
	
	std::unique_ptr< http_parser_settings >	m_settings;
	std::unique_ptr< http_parser >			m_parser;
	bool									m_reset_parser	= false;
	
	websocket_handler_f						m_web_socket_handler;
	bool									m_web_socket	= false;
	
	message::ptr							m_out_message;
	std::string								m_redirect;
	promise< message::ptr >					m_ret;
	
	message::type_t							m_in_message_type;
	message::ptr							m_in_message;
	on_body_handler_f						m_on_body_handler;
	
	nodeoze::buffer							m_buffer;

	void									*m_data;
};


class loopback : public connection 
{
public:

	loopback();

	virtual ~loopback();

	inline void
	set_dest( nodeoze::connection::ptr dest )
	{
		m_dest = dest;
	}

	virtual promise< void >
	connect();
	
	virtual promise< void >
	send( buffer buf );

	virtual bool
	is_colocated();

private:

	virtual std::error_code
	process( const buffer &buf );

	nodeoze::connection::ptr m_dest;
};


class ostream : public std::ostream
{
public:

	ostream( connection *buf )
	:
		std::ios( 0 ), 
		std::ostream( buf )
	{
	}
	
	~ostream()
	{
	}
};


class server
{
	NODEOZE_DECLARE_SINGLETON( server )
	
public:

	typedef std::unordered_map< std::uint64_t, http::connection::ptr >							connections;
	typedef std::function< void ( http::message &response, bool close ) >						response_f;
	typedef std::function< void ( http::message::ptr &request ) >								request_will_begin_f;
	typedef std::function< int ( http::message &request, buffer &buf, response_f response ) >	request_body_was_received_f;
	typedef std::function< int ( http::message &request, response_f func ) >					request_f;
	typedef std::function< void ( http::connection &conn ) >									upgrade_f;

	virtual ~server();
	
	connection::ptr adopt( ip::tcp::socket sock, std::function< void () > callback );

	void
	bind_request( method_t method, const std::string &path, const std::string &type, request_f r );
	
	void
	bind_request( method_t method, const std::string &path, const std::string &type, request_body_was_received_f rbwr, request_f r );
	
	void
	bind_request( method_t method, const std::string &path, const std::string &type, request_will_begin_f rwb, request_body_was_received_f rbwr, request_f r );
	
	void
	bind_upgrade( method_t method, const std::string &path, const std::string &type, upgrade_f f );
	
	inline connection::ptr
	active_connection()
	{
		return m_active_connection;
	}

	template< class T >
	inline std::shared_ptr< T >
	loopback()
	{
		auto loopback	= std::make_shared< class loopback >();
		auto wrapped	= std::make_shared< T >( loopback );

		m_connections[ make_oid( loopback ) ] = loopback;

		loopback->set_dest( wrapped );

		return wrapped;
	}

	inline void
	set_active_connection( connection *c )
	{
		if ( c )
		{
			auto it = m_connections.find( make_oid( c ) );
			
			if ( it != m_connections.end() )
			{
				m_active_connection = it->second;
			}
		}
		else
		{
			m_active_connection = nullptr;
		}
	}

	inline connections::iterator
	begin_connections()
	{
		return m_connections.begin();
	}

	inline connections::iterator
	end_connections()
	{
		return m_connections.end();
	}
	
protected:

	class binding
	{
	public:

		typedef std::unique_ptr< binding >	ptr;
		typedef std::list< ptr >			list;

		binding( const std::string &path, const std::string &type, request_f r )
		:
			m_path( path ),
			m_type( type ),
			m_r( r )
		{
			m_rwb = [=]( http::message::ptr &request )
			{
				nunused( request );
			};

			m_rbwr = [=]( http::message &request, buffer &buf, response_f response )
			{
				nunused( response );
				
				request << buf.to_string();
				return 0;
			};
		}

		binding( const std::string &path, const std::string &type, request_body_was_received_f rbwr, request_f r )
		:
			m_path( path ),
			m_type( type ),
			m_rbwr( rbwr ),
			m_r( r )
		{
			m_rwb = [=]( http::message::ptr &request )
			{
				nunused( request );
			};
		}

		binding( const std::string &path, const std::string &type, request_will_begin_f rwb, request_body_was_received_f rbwr, request_f r )
		:
			m_path( path ),
			m_type( type ),
			m_rwb( rwb ),
			m_rbwr( rbwr ),
			m_r( r )
		{
		}
		
		binding( const std::string &path, const std::string &type, upgrade_f u )
		:
			m_path( path ),
			m_type( type ),
			m_u( u )
		{
		}
		
		std::string					m_path;
		std::string					m_type;
		request_will_begin_f		m_rwb;
		request_body_was_received_f	m_rbwr;
		request_f					m_r;
		upgrade_f					m_u;
	};

	server();
	
	void
	bind( method_t method, binding::ptr binding );
	
	static int
	message_will_begin( http_parser *parser );

	static int
	uri_was_received( http_parser *parser, const char *buf, size_t len );

	static int
	header_field_was_received( http_parser *parser, const char *buf, size_t len );

	static int
	header_value_was_received( http_parser *parser, const char *buf, size_t len );
	
	static int
	headers_were_received( http_parser *parser );
	
	static int
	body_was_received( http_parser *parser, const char *buf, size_t len );
	
	static int
	message_was_received( http_parser *parser );
	
	binding*
	resolve( const message::ptr &message );
	
	bool
	remove( http::connection *conn );

	std::string
	regexify( const std::string &s );

	void
	replace( std::string& str, const std::string& oldStr, const std::string& newStr);
	

	typedef std::map< method_t, binding::list > bindings;

	connection::ptr	m_active_connection;
	connections		m_connections;
	bindings		m_bindings;
	
	friend class http::connection;
};

inline std::ostream&
endl( std::ostream &os )
{
	os << "\r\n";
	return os;
}

inline std::ostream&
operator<<( std::ostream &os, nodeoze::http::method_t method )
{
	os << http::to_string( method );
	return os;
}

inline std::ostream&
operator<<( std::ostream &os, nodeoze::http::code_t code )
{
	os << static_cast< std::uint16_t >( code );
	return os;
}

std::ostream&
operator<<( std::ostream &os, const nodeoze::http::message &obj );

const std::error_category&
error_category();
	
inline std::error_code
make_error_code( code_t val )
{
	return std::error_code( static_cast<int>( val ), error_category() );
}

}

}

namespace std
{
	template<>
	struct is_error_code_enum< nodeoze::http::code_t > : public std::true_type {};
}

#endif
