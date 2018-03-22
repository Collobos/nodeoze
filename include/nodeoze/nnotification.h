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

#ifndef _nodeoze_notification_h
#define _nodeoze_notification_h

#include <nodeoze/nsingleton.h>
#include <nodeoze/nscoped_operation.h>
#include <nodeoze/nuri.h>
#include <nodeoze/nuuid.h>
#include <nodeoze/nany.h>
#include <nodeoze/noid.h>
#include <unordered_map>
#include <vector>

namespace nodeoze {

namespace rpc {

class connection;

}

namespace http {

class connection;

}
 
class notification
{
	NODEOZE_DECLARE_SINGLETON( notification )
	
public:

	static const std::string								subscribe_event;
	
	static const uri										local;
	
	typedef std::function< bool ( const any &info ) >		handler_f;
	typedef std::function< void ( scoped_operation obj ) >	subscribe_reply_f;

	~notification();

	void
	subscribe( const uri &uri, std::uint64_t source_oid, const std::string &event, handler_f handler, subscribe_reply_f reply = nullptr );
	
	void
	publish( const uri &uri, std::uint64_t subscription_oid, std::uint64_t source_oid, const std::string &event, any info = any::null() );
	
protected:

	notification();
	
	struct local_subscriber
	{
		typedef std::shared_ptr< local_subscriber >	ptr;
		typedef std::vector< ptr >					list;
		
		local_subscriber( std::uint64_t source_oid, const std::string &event, handler_f handler )
		:
			m_source_oid( source_oid ),
			m_event( event ),
			m_handler( handler )
		{
		}
	
		uri				m_resource;
		std::uint64_t	m_source_oid;
		std::string		m_event;
		handler_f		m_handler;
		bool			m_extant	= true;
	};
	
	struct remote_subscriber
	{
		typedef std::shared_ptr< remote_subscriber > ptr;
		
		remote_subscriber( const uri &resource, std::shared_ptr< rpc::connection > server, std::uint64_t source_oid, const std::string &event, handler_f handler )
		:
			m_resource( resource ),
			m_server( server ),
			m_oid( uuid().to_string() ),
			m_source_oid( source_oid ),
			m_event( event ),
			m_handler( handler )
		{
		}
	
		uri									m_resource;
		std::shared_ptr< rpc::connection >	m_server;
		std::string							m_oid;
		std::uint64_t						m_source_oid;
		std::string							m_event;
		handler_f							m_handler;
	};
	
	struct proxy_subscriber
	{
		typedef std::shared_ptr< proxy_subscriber > ptr;
		
		proxy_subscriber( const std::string &remote_oid )
		:
			m_remote_oid( remote_oid )
		{
		}
		
		scoped_operation	m_local_subscription;
		scoped_operation	m_close_subscription;
		std::string			m_remote_oid;
	};
	
	template < typename deleter >
	scoped_operation
	create_subscription( deleter d )
	{
		return scoped_operation::create( d );
	}
	
	void
	subscribe_local( std::uint64_t source_oid, const std::string &event, handler_f handler, subscribe_reply_f reply );

	void
	subscribe_remote( const uri &resource, std::uint64_t source_oid, const std::string &event, handler_f handler, subscribe_reply_f reply );
	
	void
	subscribe_proxy( std::shared_ptr< http::connection > client, const std::string &remote_oid, std::uint64_t source_oid, const std::string &event );
	
	void
	unsubscribe_proxy( const std::string &remote_oid );
	
	void
	publish_local( std::uint64_t subscription_oid, std::uint64_t source_oid, const std::string &event, const any &info );
	
	void
	publish_remote( const uri &resource, std::uint64_t subscription_oid, std::uint64_t source_oid, const std::string &event, const any &info );
	
	void
	notify( const std::string &subscription_oid, const std::string &event, const any &info );
	
	std::unordered_map< std::string, local_subscriber::list >	m_local_subscribers;
	std::unordered_map< std::string, remote_subscriber::ptr >	m_remote_subscribers;
	std::unordered_map< std::string, proxy_subscriber::ptr >	m_proxy_subscribers;
};

}

#endif
