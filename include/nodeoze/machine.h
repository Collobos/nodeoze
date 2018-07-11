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
 
#ifndef _nodeoze_machine_h
#define _nodeoze_machine_h

#include <nodeoze/any.h>
#include <nodeoze/json.h>
#include <nodeoze/location.h>
#include <nodeoze/endpoint.h>
#include <nodeoze/mac.h>
#include <chrono>
#include <cstdint>
#include <string>

namespace nodeoze {

class machine
{
public:

	static const std::string																														network_change_event;
	static const std::string																														printer_change_event;
	
	typedef std::function< void ( const std::vector< ip::endpoint > &name_servers, const std::vector< std::string > &domains ) >					dns_reply_f;
	typedef std::function< void ( const std::vector< ip::endpoint > &name_servers, const std::vector< std::string > &domains, dns_reply_f reply ) >	dns_picker_f;

	class logon
	{
	public:

		logon()
		{
		}

		logon( const logon &rhs )
		:
			m_name( rhs.m_name ),
			m_display_name( rhs.m_display_name ),
			m_guid( rhs.m_guid ),
			m_groups( rhs.m_groups )
		{
		}

		logon( logon &&rhs )
		:
			m_name( std::move( rhs.m_name ) ),
			m_display_name( std::move( rhs.m_display_name ) ),
			m_guid( std::move( rhs.m_guid ) ),
			m_groups( std::move( rhs.m_groups ) )
		{
		}

		logon( const any &root )
		:
			m_name( root[ "name" ].to_string() ),
			m_display_name( root[ "display_name" ].to_string() ),
			m_guid( root[ "guid" ].to_string() )
		{
			for ( auto &val : root[ "groups" ] )
			{
				m_groups.push_back( std::string{ val.to_string() } );
			}
		}

		logon&
		operator=( const logon &rhs )
		{
			m_name			= rhs.m_name;
			m_display_name	= rhs.m_display_name;
			m_guid			= rhs.m_guid;
			m_groups		= rhs.m_groups;

			return *this;
		}

		logon&
		operator=( logon &&rhs )
		{
			m_name			= std::move( rhs.m_name );
			m_display_name	= std::move( rhs.m_display_name );
			m_guid			= std::move( rhs.m_guid );
			m_groups		= std::move( rhs.m_groups );

			return *this;
		}

		inline const std::string&
		name() const
		{
			return m_name;
		}

		inline void
		set_name( const std::string &val )
		{
			m_name = val;
		}

		inline const std::string&
		display_name() const
		{
			return m_display_name;
		}

		inline void
		set_display_name( const std::string &val )
		{
			m_display_name = val;
		}

		inline const std::string&
		guid() const
		{
			return m_guid;
		}

		inline void
		set_guid( const std::string &val )
		{
			m_guid = val;
		}

		inline const deque< std::string >&
		groups() const
		{
			return m_groups;
		}

		inline void
		set_groups( const deque< std::string > &val )
		{	
			m_groups = val;
		}

		inline any
		to_any() const
		{
			any root;

			root[ "guid" ] 			= m_guid;
			root[ "name" ] 			= m_name;
			root[ "display_name" ]	= m_display_name;

			for ( auto &group : m_groups )
			{
				root[ "groups" ].push_back( group );
			}

			return root;
		}

	private:

		std::string				m_name;
		std::string				m_display_name;
		std::string				m_guid;
		deque< std::string >	m_groups;
	};
	
	class nif
	{
	public:

		struct flags_t
		{
			static const std::uint32_t up				= std::uint32_t( 1 << 0 );
			static const std::uint32_t loopback			= std::uint32_t( 1 << 1 );
			static const std::uint32_t point_to_point	= std::uint32_t( 1 << 2 );
			static const std::uint32_t multicast		= std::uint32_t( 1 << 3 );
			static const std::uint32_t broadcast		= std::uint32_t( 1 << 4 );
		};
		
		inline
		nif()
		:
			m_index( 0 ),
			m_flags( flags_t::up )
		{
		}

		nif( const nif &rhs )
		:
			m_name( rhs.m_name ),
			m_display_name( rhs.m_display_name ),
			m_mac_address( rhs.m_mac_address ),
			m_address( rhs.m_address ),
			m_netmask( rhs.m_netmask ),
			m_index( rhs.m_index ),
			m_flags( rhs.m_flags )
		{
		}

		nif( nif &&rhs )
		:
			m_name( std::move( rhs.m_name ) ),
			m_display_name( std::move( rhs.m_display_name ) ),
			m_mac_address( rhs.m_mac_address ),
			m_address( rhs.m_address ),
			m_netmask( rhs.m_netmask ),
			m_index( rhs.m_index ),
			m_flags( rhs.m_flags )
		{
		}

		nif( const any &root )
		:
			m_name( root[ machine::nif::keys::name ].to_string() ),
			m_display_name( root[ machine::nif::keys::display_name ].to_string() ),
			m_address( root[ machine::nif::keys::ip_address ] ),
			m_netmask( root[ machine::nif::keys::ip_netmask ] ),
			m_index( root[ machine::nif::keys::index ].to_uint32() ),
			m_flags( root[ machine::nif::keys::flags ].to_uint32() )
		{
			if ( root[ machine::nif::keys::mac_address ].size() > 0 )
			{
				m_mac_address = mac::address( root[ machine::nif::keys::mac_address ].to_blob() );
			}
		}

		nif&
		operator=( const nif &rhs )
		{
			m_name			= rhs.m_name;
			m_display_name	= rhs.m_display_name;
			m_mac_address	= rhs.m_mac_address;
			m_address		= rhs.m_address;
			m_netmask		= rhs.m_netmask;
			m_index			= rhs.m_index;
			m_flags			= rhs.m_flags;
			
			return *this;
		}

		nif&
		operator=( nif &&rhs )
		{
			m_name			= std::move( rhs.m_name );
			m_display_name	= std::move( rhs.m_display_name );
			m_mac_address	= std::move( rhs.m_mac_address );
			m_address		= std::move( rhs.m_address );
			m_netmask		= std::move( rhs.m_netmask );
			m_index			= std::move( rhs.m_index );
			m_flags			= std::move( rhs.m_flags );
			
			return *this;
		}
		
		inline bool
		operator==( const nif &rhs ) const
		{
			return ( m_mac_address && ( m_mac_address == rhs.m_mac_address ) );
		}

		inline bool
		is_facsimile( const nif &rhs ) const
		{
			return (
				( m_name == rhs.m_name )	&&
				( m_display_name == rhs.m_display_name ) &&
				( m_mac_address == rhs.m_mac_address ) &&
				( m_address == rhs.m_address ) &&
				( m_netmask == rhs.m_netmask ) &&
				( m_index == rhs.m_index ) &&
				( m_flags == rhs.m_flags ) );
		}
	
		inline const std::string&
		name() const
		{
			return m_name;
		}
		
		inline void
		set_name( const std::string &val )
		{
			m_name = val;
		}

		inline const std::string&
		display_name() const
		{
			return m_display_name;
		}
		
		inline void
		set_display_name( const std::string &val )
		{
			m_display_name = val;
		}
		
		inline const nodeoze::mac::address&
		mac_address() const
		{
			return m_mac_address;
		}
		
		inline void
		set_mac_address( const nodeoze::mac::address &val )
		{
			m_mac_address = val;
		}

		inline const nodeoze::ip::address&
		address() const
		{
			return m_address;
		}
		
		inline void
		set_address( const nodeoze::ip::address &val )
		{
			m_address = val;
		}
		
		inline const nodeoze::ip::address&
		netmask() const
		{
			return m_netmask;
		}
		
		inline void
		set_netmask( const nodeoze::ip::address &val )
		{
			m_netmask = val;
		}
		
		inline std::uint32_t
		index() const
		{
			return m_index;
		}

		inline std::uint32_t
		flags() const
		{
			return m_flags;
		}
		
		inline void
		set_flags( std::uint32_t val )
		{
			m_flags = val;
		}
		
		inline bool
		is_up() const
		{
			return ( static_cast< int >( m_flags ) & static_cast< int >( flags_t::up ) ) ? true : false;
		}
		
		inline explicit operator bool() const
		{
			return ( is_up() && m_address );
		}
		
		void
		deflate( any &root ) const
		{
			root[ machine::nif::keys::name ]			= m_name;
			root[ machine::nif::keys::display_name ]	= m_display_name;
			root[ machine::nif::keys::mac_address ]		= m_mac_address.to_any();
			root[ machine::nif::keys::ip_address ]		= m_address.to_any();
			root[ machine::nif::keys::ip_netmask ]		= m_netmask.to_any();
			root[ machine::nif::keys::index ]			= m_index;
			root[ machine::nif::keys::flags ]			= nodeoze::any( m_flags );
		}

		any
		to_any() const
		{
			any root;
			
			deflate( root );
			
			return root;
		}

		struct keys
		{
			static const std::string	name;
			static const std::string	display_name;
			static const std::string	mac_address;
			static const std::string	ip_address;
			static const std::string	ip_netmask;
			static const std::string	index;
			static const std::string	flags;
		};

	protected:
	
		std::string 	m_name;
		std::string		m_display_name;
		mac::address	m_mac_address;
		ip::address		m_address;
		ip::address		m_netmask;
		std::uint32_t	m_index	= 0;
		std::uint32_t	m_flags = 0;
	};
	
	static machine&
	self();
	
	machine();
	
	machine( const std::string &name );
	
	machine( const nodeoze::any &root );

	machine( const machine &rhs );
	
	machine( machine &&rhs );
	
	virtual ~machine();
	
	machine&
	operator=( const machine &rhs );
	
	machine&
	operator=( machine &&rhs );
	
	inline bool
	has_internet_connection()
	{
		bool ok = false;
		
		for ( auto &nif : m_nifs )
		{
			if ( !nif.address().is_loopback() && !nif.address().is_link_local() && nif.is_up() )
			{
				ok = true;
				break;
			}
		}
		
		return ok;
	}

	inline bool
	is_facsimile( const machine &rhs ) const
	{
		return 
			( true &&
			( m_name == rhs.m_name )					&&
			( m_mdnsname == rhs.m_mdnsname )			&&
			( m_display_name == rhs.m_display_name )	&&
			( m_description == rhs.m_description )		&&
			( m_location == rhs.m_location )			&&
			( m_memory == rhs.m_memory )				&&
			( m_cores == rhs.m_cores )					&&
			( m_name_servers == rhs.m_name_servers )	&&
			( m_domains == rhs.m_domains )				&&
			( m_nifs.size() == rhs.m_nifs.size() )		&&
			( std::equal( m_nifs.begin(), m_nifs.end(), rhs.m_nifs.begin(), []( const nif &lhs, const nif &rhs ) { return lhs.is_facsimile( rhs ); } ) ) );
	}
	
	inline bool
	operator==( const machine &rhs ) const
	{
		auto lhs_has_at_least_one_mac_address	= false;
		auto rhs_has_at_least_one_mac_address	= false;
		auto equal								= false;

		for ( auto it = m_nifs.begin(); it != m_nifs.end() && !equal; it++ )
		{
			if ( it->mac_address() )
			{
				lhs_has_at_least_one_mac_address = true;

				for ( auto &nif : rhs.m_nifs )
				{
					if ( nif.mac_address() )
					{
						rhs_has_at_least_one_mac_address = true;

						if ( *it == nif )
						{
							equal = true;
							break;
						}
					}
				}
			}
		}

		// fallback

		if ( !equal && ( !lhs_has_at_least_one_mac_address || !rhs_has_at_least_one_mac_address ) )
		{
			equal = m_name == rhs.m_name;
		}
		
		return equal;
	}
	
	inline bool
	operator==( const std::vector< nodeoze::mac::address > &mac_addresses ) const
	{
		auto equal = false;

		for ( auto it = m_nifs.begin(); it != m_nifs.end() && !equal; it++ )
		{
			if ( it->mac_address() )
			{
				for ( auto &mac_address : mac_addresses )
				{
					if ( mac_address )
					{
						if ( it->mac_address() == mac_address )
						{
							equal = true;
							break;
						}
					}
				}
			}
		}

		return equal;
	}
	
	inline bool
	operator!=( const std::vector< nodeoze::mac::address > &mac_addresses ) const
	{
		return !operator==( mac_addresses );
	}
	
	inline bool
	contains( const ip::address &address ) const
	{
		return std::find_if( m_nifs.begin(), m_nifs.end(), [&]( const machine::nif &nif ) mutable
		{
			return nif.address() == address;
		} ) != m_nifs.end();
	}
	
	inline const std::string&
	name() const
	{
		return m_name;
	}
	
	inline const std::string&
	display_name() const
	{
		return m_display_name;
	}
	
	inline const std::string&
	mdnsname() const
	{
		return m_mdnsname;
	}
	
	inline const std::string&
	description() const
	{
		return m_description;
	}
	
	inline const location&
	location() const
	{
		return m_location;
	}
	
	inline void
	set_location( const nodeoze::location &val )
	{
		m_location = val;
	}
	
	inline std::uint64_t
	memory() const
	{
		return m_memory;
	}
	
	inline const std::vector< nif >&
	nifs() const
	{
		return m_nifs;
	}
	
	inline std::uint16_t
	cores()
	{
		return m_cores;
	}
	
	inline void
	set_cores( std::uint16_t cores )
	{
		m_cores = cores;
	}
	
	inline void
	set_nifs( const std::vector< nif > &nifs )
	{
		m_nifs = nifs;
	}

	inline const logon&
	logon() const
	{
		return m_logon;
	}

	inline void
	set_logon( const class logon &val )
	{
		m_logon = val;
	}
	
	friend std::ostream&
	operator<<( std::ostream &os, const machine &rhs )
	{
		return os << json::deflate_to_string( rhs.to_any() );
	}
	
	virtual void
	dns( dns_reply_f reply );
	
	inline void
	set_dns_picker( dns_picker_f func )
	{
		m_dns_picker = func;
	}
	
	virtual void
	refresh();

	struct keys
	{
		static const std::string					name;
		static const std::string					display_name;
		static const std::string					mdnsname;
		static const std::string					description;
		static const std::string					location;
		static const std::string					memory;
		static const std::string					cores;
		static const std::string					name_servers;
		static const std::string					domains;
		static const std::string					nifs;
		static const std::string					logon;
	};
	
	inline nodeoze::any
	to_any() const
	{
		nodeoze::any root;
		
		deflate( root );
		
		return root;
	}
	
	void
	deflate( nodeoze::any &root ) const;
	
	machine&
	inflate( const nodeoze::any &root );
	
protected:

	std::string										m_name;
	std::string										m_display_name;
	std::string										m_mdnsname;
	std::string										m_description;
	nodeoze::location								m_location;
	dns_picker_f									m_dns_picker;
	std::vector< ip::endpoint >						m_name_servers;
	std::vector< std::string >						m_domains;
	std::uint64_t									m_memory;
	std::vector< nif >								m_nifs;
	std::uint16_t									m_cores;
	class logon										m_logon;
};

}

#endif
