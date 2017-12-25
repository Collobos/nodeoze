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

#include <nodeoze/nmachine.h>

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark machine implementation
#endif

const std::string machine::network_change_event = "machine_network_change_event";
const std::string machine::printer_change_event = "machine_printer_change_event";


const std::string	machine::nif::keys::name					= "name";
const std::string	machine::nif::keys::display_name			= "display_name";
const std::string	machine::nif::keys::mac_address				= "mac_address";
const std::string	machine::nif::keys::ip_address				= "ip_address";
const std::string	machine::nif::keys::ip_netmask				= "ip_netmask";
const std::string	machine::nif::keys::index					= "index";
const std::string	machine::nif::keys::flags					= "flags";


const std::string machine::keys::name							= "name";
const std::string machine::keys::display_name					= "display_name";
const std::string machine::keys::mdnsname						= "mdnsname";
const std::string machine::keys::description					= "description";
const std::string machine::keys::location						= "location";
const std::string machine::keys::memory							= "memory";
const std::string machine::keys::cores							= "cores";
const std::string machine::keys::name_servers					= "name_servers";
const std::string machine::keys::domains						= "domains";
const std::string machine::keys::nifs							= "nifs";
const std::string machine::keys::logon							= "logon";


machine::machine()
{
}


machine::machine( const std::string &name )
:
	m_name( name ),
	m_display_name( name )
{
}


machine::machine( const nodeoze::any &root )
{
	inflate( root );
}


machine::machine( const machine &rhs )
:
	m_name( rhs.m_name ),
	m_display_name( rhs.m_display_name ),
	m_mdnsname( rhs.m_mdnsname ),
	m_description( rhs.m_description ),
	m_location( rhs.m_location ),
	m_name_servers( rhs.m_name_servers ),
	m_domains( rhs.m_domains ),
	m_memory( rhs.m_memory ),
	m_nifs( rhs.m_nifs ),
	m_cores( rhs.m_cores ),
	m_logon( rhs.m_logon )
{
}

	
machine::machine( machine &&rhs )
:
	m_name( std::move( rhs.m_name ) ),
	m_display_name( std::move( rhs.m_display_name ) ),
	m_mdnsname( std::move( rhs.m_mdnsname ) ),
	m_description( std::move( rhs.m_description ) ),
	m_location( std::move( rhs.m_location ) ),
	m_name_servers( std::move( rhs.m_name_servers ) ),
	m_domains( std::move( rhs.m_domains ) ),
	m_memory( rhs.m_memory ),
	m_nifs( std::move( rhs.m_nifs ) ),
	m_cores( rhs.m_cores ),
	m_logon( std::move( rhs.m_logon ) )
{
}


machine::~machine()
{
}

	
machine&
machine::operator=( const machine &rhs )
{
	m_name					= rhs.m_name;
	m_display_name			= rhs.m_display_name;
	m_mdnsname				= rhs.m_mdnsname;
	m_description			= rhs.m_description;
	m_location				= rhs.m_location;
	m_memory				= rhs.m_memory;
	m_cores					= rhs.m_cores;
	m_domains				= rhs.m_domains;
	m_name_servers			= rhs.m_name_servers;
	m_nifs					= rhs.m_nifs;
	m_logon					= rhs.m_logon;

	return *this;
}

	
machine&
machine::operator=( machine &&rhs )
{
	m_name					= std::move( rhs.m_name );
	m_display_name			= std::move( rhs.m_display_name );
	m_mdnsname				= std::move( rhs.m_mdnsname );
	m_description			= std::move( rhs.m_description );
	m_location				= std::move( rhs.m_location );
	m_memory				= rhs.m_memory;
	m_cores					= rhs.m_cores;
	m_domains				= std::move( rhs.m_domains );
	m_name_servers			= std::move( rhs.m_name_servers );
	m_nifs					= std::move( rhs.m_nifs );
	m_logon					= std::move( rhs.m_logon );
	
	return *this;
}


void
machine::deflate( nodeoze::any &root ) const
{
	root[ machine::keys::name ]			= m_name;
	root[ machine::keys::display_name ]	= m_display_name;
	root[ machine::keys::mdnsname ]		= m_mdnsname;
	root[ machine::keys::description ]	= m_description;
	root[ machine::keys::location ]		= m_location.to_any();
	root[ machine::keys::memory ]		= m_memory;
	root[ machine::keys::cores ]		= m_cores;
	
	for ( auto &it : m_name_servers )
	{
		root[ machine::keys::name_servers ].emplace_back( it.to_any() );
	}
	
	for ( auto &it : m_domains )
	{
		root[ machine::keys::domains ].emplace_back( it );
	}
	
	for ( auto &it : m_nifs )
	{
		root[ machine::keys::nifs ].emplace_back( it.to_any() );
	}

	root[ machine::keys::logon ] = m_logon.to_any();
}


machine&
machine::inflate( const nodeoze::any &root )
{
	m_name			= root[ machine::keys::name ].to_string();
	m_display_name	= root[ machine::keys::display_name ].to_string();
	m_mdnsname		= root[ machine::keys::mdnsname ].to_string();
	m_description	= root[ machine::keys::description ].to_string();
	m_location		= root[ machine::keys::location ];
	m_memory		= root[ machine::keys::memory ].to_uint64();
	m_cores			= root[ machine::keys::cores ].to_uint16();
	m_logon			= root[ machine::keys::logon ];
	
	m_name_servers.clear();
	
	for ( auto i = 0u; i < root[ machine::keys::name_servers ].size(); i++ )
	{
		m_name_servers.emplace_back( root[ machine::keys::name_servers ][ i ] );
	}
	
	m_domains.clear();

	for ( auto i = 0u; i < root[ machine::keys::domains ].size(); i++ )
	{
		m_domains.emplace_back( root[ machine::keys::domains ][ i ].to_string() );
	}
	
	m_nifs.clear();
	
	for ( auto i = 0u; i < root[ machine::keys::nifs ].size(); i++ )
	{
		m_nifs.emplace_back( root[ machine::keys::nifs ][ i ] );
	}
	
	return *this;
}


void
machine::dns( dns_reply_f reply )
{
	if ( m_dns_picker )
	{
		m_dns_picker( m_name_servers, m_domains, reply );
	}
	else
	{
		reply( m_name_servers, m_domains );
	}
}

	
void
machine::refresh()
{
}
