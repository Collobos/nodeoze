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

#include "nmachine_win32.h"
#include <nodeoze/nmachine.h>
#include <nodeoze/nrunloop.h>
#include <nodeoze/nnotification.h>
#include <nodeoze/nunicode.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nmarkers.h>
#include <nodeoze/nlog.h>
#include <IPHlpApi.h>
#include <strsafe.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <uv.h>

#pragma comment( lib, "iphlpapi.lib" )
#pragma comment( lib, "userenv.lib" )

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark machine_win32 implementation
#endif

machine&
machine::self()
{
	static machine_win32 obj;
	
	return obj;
}


machine_win32::machine_win32()
{
	std::thread t( [=]()
	{
		int error_count = 0;

		while ( error_count < 10 )
		{
			DWORD ret;

			ret = NotifyAddrChange( nullptr, nullptr );

			if ( ret == NO_ERROR )
			{
				runloop::shared().dispatch( [=]()
				{
					nlog( log::level_t::info, "received network change event" );
					refresh();
					notification::shared().publish( notification::local, 0, make_oid( self ), network_change_event, this->to_any() );
				} );
			}
			else if ( WSAGetLastError() != WSA_IO_PENDING )
			{
				mlog( marker::machine, log::level_t::info, "NotifyAddrChange() failed: %", WSAGetLastError() );
				error_count++;
			}
		}
	} );

	t.detach();

	setup_printer_notifications();

	schedule_printer_notifications();

	refresh();
}


machine_win32::~machine_win32()
{
}


void
machine_win32::refresh()
{
	DWORD		len;
	DWORD		err;
	ULONG													iterations	= 0;
	buffer		buf( sizeof( FIXED_INFO ) );
	IP_ADAPTER_ADDRESSES *adapter;
	DWORD tries = 0;
	const DWORD MAX_TRIES = 10;
	DWORD ret;

	m_name_servers.clear();
	m_nifs.clear();
	
	m_name			= get_hostname();
	m_display_name	= m_name;
	m_mdnsname		= get_mdnsname();
	m_description	= get_description();
	m_domains.clear();

	m_memory		= uv_get_total_memory();
	
	len = static_cast< DWORD >( buf.size() );

	err = GetNetworkParams( reinterpret_cast< FIXED_INFO* >( buf.data() ), &len );

	if ( err == ERROR_BUFFER_OVERFLOW )
	{
		buf.size( len );

		err = GetNetworkParams( reinterpret_cast< FIXED_INFO* >( buf.data() ), &len );
    }

	if ( err == NO_ERROR )
	{
		FIXED_INFO	*info = reinterpret_cast< FIXED_INFO* >( buf.data() );

		m_domains.push_back( info->DomainName );

		for ( auto addr = &info->DnsServerList; addr; addr = addr->Next )
		{
			m_name_servers.emplace_back( ip::address( addr->IpAddress.String ), 53 );
		}
	}

    do
	{
		len = static_cast< DWORD >( buf.size() );

		ret = GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, nullptr, reinterpret_cast< IP_ADAPTER_ADDRESSES* >( buf.data() ), &len );

		if ( ret == ERROR_BUFFER_OVERFLOW )
		{
			buf.size( buf.size() * 2 );
		}

		tries++;

    }
	while ( ( ret == ERROR_BUFFER_OVERFLOW) && ( tries < MAX_TRIES ) );

	ncheck_error_quiet( ret == NO_ERROR, exit );

	for ( adapter = reinterpret_cast< IP_ADAPTER_ADDRESSES* >( buf.data() ), adapter; adapter; adapter = adapter->Next )
	{
		auto						addr_index	= 0;
		PIP_ADAPTER_UNICAST_ADDRESS addr		= nullptr;

		for ( addr_index = 0, addr = adapter->FirstUnicastAddress; addr; ++addr_index, addr = addr->Next )
		{			
			int						family;
			IP_ADAPTER_PREFIX *		prefix;
			std::uint32_t			ipv4_index;
			sockaddr_in				ipv4_netmask;
			IF_INDEX				ipv6IfIndex;
			PIP_ADAPTER_PREFIX		firstPrefix;
			nif_win32				obj;

			family = addr->Address.lpSockaddr->sa_family;
			if ( ( family != AF_INET ) && ( family != AF_INET6 ) ) continue;
			
			ipv4_index = 0;
			memset( &ipv4_netmask, 0, sizeof( ipv4_netmask ) );
			
			if ( adapter->Length >= sizeof( IP_ADAPTER_ADDRESSES ) )
			{
				ipv6IfIndex = adapter->Ipv6IfIndex;
				firstPrefix = adapter->FirstPrefix;
			}
			else
			{
				ipv6IfIndex	= 0;
				firstPrefix = NULL;
			}

			obj.m_name			= adapter->AdapterName;
			obj.m_mac_address	= get_mac_address( adapter->AdapterName );

			obj.m_display_name	= narrow( adapter->FriendlyName );
			obj.m_flags			= static_cast< nif::flags_t >( 0 );

			if ( adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK )
			{
				obj.m_flags |= nif::flags_t::loopback;
			}
			
			if ( !( adapter->Flags & IP_ADAPTER_NO_MULTICAST ) )
			{
				obj.m_flags |= nif::flags_t::multicast;
			}
			
			if ( adapter->IfType == IF_TYPE_PPP )
			{
				obj.m_flags |= nif::flags_t::point_to_point;
			}
			
			if ( adapter->OperStatus == IfOperStatusUp )
			{
				obj.m_flags |= nif::flags_t::up;
			}

			if ( addr->Address.lpSockaddr->sa_family == AF_INET )
			{
				obj.m_address = ip::address( reinterpret_cast< sockaddr_in* >( addr->Address.lpSockaddr )->sin_addr );
			}
			else if ( addr->Address.lpSockaddr->sa_family == AF_INET6 )
			{
				obj.m_address = ip::address( reinterpret_cast< sockaddr_in6* >( addr->Address.lpSockaddr )->sin6_addr );
			}

			// According to MSDN:
			// "On Windows Vista and later, the linked IP_ADAPTER_PREFIX structures pointed to by the FirstPrefix member
			// include three IP adapter prefixes for each IP address assigned to the adapter. These include the host IP address prefix,
			// the subnet IP address prefix, and the subnet broadcast IP address prefix.
			// In addition, for each adapter there is a multicast address prefix and a broadcast address prefix.
			// On Windows XP with SP1 and later prior to Windows Vista, the linked IP_ADAPTER_PREFIX structures pointed to by the FirstPrefix member

			switch ( family )
			{
				case AF_INET:
				{
					get_index_and_mask( addr->Address.lpSockaddr, obj.m_index, obj.m_netmask );
				}
				break;

				case AF_INET6:
				{
					obj.m_index = adapter->Ipv6IfIndex;
					struct sockaddr_in6 sa6;

					sa6.sin6_family = AF_INET6;
					memset( sa6.sin6_addr.s6_addr, 0xFF, sizeof( sa6.sin6_addr.s6_addr ) );

					for ( prefix = firstPrefix; prefix; prefix = prefix->Next )
					{
						IN6_ADDR	mask;
						IN6_ADDR	masked_addr;
						int			mask_len;
						int			mask_index;

						if ( ( prefix->PrefixLength == 0 ) || ( prefix->PrefixLength > 128 ) || ( addr->Address.iSockaddrLength != prefix->Address.iSockaddrLength ) || ( memcmp( addr->Address.lpSockaddr, prefix->Address.lpSockaddr, addr->Address.iSockaddrLength ) == 0 ) )
						{
							continue;
						}

						// Compute the mask

						memset( mask.s6_addr, 0, sizeof( mask.s6_addr ) );

						for ( mask_len = (int) prefix->PrefixLength, mask_index = 0; mask_len > 0; mask_len -= 8 )
						{
							uint8_t maskByte = ( mask_len >= 8 ) ? 0xFF : (uint8_t)( ( 0xFFU << ( 8 - mask_len ) ) & 0xFFU );
							mask.s6_addr[ mask_index++ ] = maskByte;
						}

						// Apply the mask

						for ( auto i = 0; i < 16; i++ )
						{
							masked_addr.s6_addr[ i ] = ( ( struct sockaddr_in6* ) addr->Address.lpSockaddr )->sin6_addr.s6_addr[ i ] & mask.s6_addr[ i ];
						}

						// Compare

						if ( memcmp( ( ( struct sockaddr_in6* ) prefix->Address.lpSockaddr )->sin6_addr.s6_addr, masked_addr.s6_addr, sizeof( masked_addr.s6_addr ) ) == 0 )
						{
							obj.m_netmask = ip::address( masked_addr.s6_addr );
							break;
						}
					}
				}
				break;

				default:
				{
				}
				break;
			}

			if ( !obj.address().is_link_local() )
			{
				m_nifs.emplace_back( std::move( obj ) );
			}
		}
	}

exit:

	return;
}


std::string
machine_win32::get_hostname()
{                                              
    std::string		name;
	std::tchar_t	buf[ 256 ];
	DWORD			bufLen = sizeof( buf );
		
	if ( !GetComputerName( buf, &bufLen ) )
	{
		_tcscpy_s( buf, sizeof( buf ) / sizeof( std::tchar_t ), TEXT( "localhost" ) );
	}

	name = narrow( buf );
	
	return name;                        
}


std::string
machine_win32::get_mdnsname()
{
	HKEY		desc_key = NULL;
	std::string	utf8;
	DWORD		namelen;
	TCHAR		*s;
	DWORD		err;
	BOOL		ok;
	
	// Set up the nice name.
	utf8[0] = '\0';

	// First try and open the registry key that contains the computer description value
	s = TEXT("SYSTEM\\CurrentControlSet\\Services\\lanmanserver\\parameters");
	err = RegOpenKeyEx( HKEY_LOCAL_MACHINE, s, 0, KEY_READ, &desc_key);

	if ( !err )
	{
		TCHAR	desc[256];
		DWORD	descSize = sizeof( desc );

		// look for the computer description
		err = RegQueryValueEx( desc_key, TEXT("srvcomment"), 0, NULL, (LPBYTE) &desc, &descSize);
		
		if ( !err )
		{
			utf8 = narrow( desc );
		}
	}

	// if we can't find it in the registry, then use the hostname of the machine
	if ( err || ( utf8.size() == 0 ) )
	{
		TCHAR hostname[256];
		
		namelen = sizeof( hostname ) / sizeof( TCHAR );

		ok = GetComputerNameExW( ComputerNamePhysicalDnsHostname, hostname, &namelen );
		
		if ( ok )
		{
			utf8 = narrow( hostname );
		}
	}

	// if we can't get the hostname
	if ( err || ( utf8.size() == 0 ) )
	{
		// Invalidate name so fall back to a default name.
		
	//	strcpy( utf8, kMDNSDefaultName );
	}

	utf8 += ".local.";

	if ( desc_key != nullptr )
	{
		RegCloseKey( desc_key );
	}

	return utf8;
}

#include <string>
#include <lm.h>
#pragma comment(lib, "netapi32.lib")

bool GetWinMajorMinorVersion(DWORD& major, DWORD& minor)
{
    bool bRetCode = false;
    LPBYTE pinfoRawData = 0;
    if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
    {
        WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
        major = pworkstationInfo->wki100_ver_major;
        minor = pworkstationInfo->wki100_ver_minor;
        ::NetApiBufferFree(pinfoRawData);
        bRetCode = true;
    }
    return bRetCode;
}


std::string
machine_win32::get_description()
{
    std::string     winver;
    OSVERSIONINFOEX osver;
    SYSTEM_INFO     sys_info;
    typedef void(__stdcall *GETSYSTEMINFO) (LPSYSTEM_INFO);

    __pragma(warning(push))
    __pragma(warning(disable:4996))
    memset(&osver, 0, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    GetVersionEx((LPOSVERSIONINFO)&osver);
    __pragma(warning(pop))
    DWORD major = 0;
    DWORD minor = 0;

	if ( GetWinMajorMinorVersion( major, minor ) )
    {
        osver.dwMajorVersion = major;
        osver.dwMinorVersion = minor;
    }
    else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 )
    {
        OSVERSIONINFOEXW osvi;
        ULONGLONG cm = 0;
        cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
        ZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMinorVersion = 3;
        if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm))
        {
            osver.dwMinorVersion = 3;
        }
    }

	GETSYSTEMINFO getSysInfo = (GETSYSTEMINFO)GetProcAddress( GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo" );

    if ( getSysInfo == nullptr )
	{
		getSysInfo = ::GetSystemInfo;
	}

    getSysInfo(&sys_info);

    if ( osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType != VER_NT_WORKSTATION )
	{
		winver = "Windows Server 2016";
	}
	else if ( osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType == VER_NT_WORKSTATION )
	{
		winver = "Windows 10";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType != VER_NT_WORKSTATION )
	{
		winver = "Windows Server 2012 R2";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType == VER_NT_WORKSTATION )
	{
		winver = "Windows 8.1";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType != VER_NT_WORKSTATION )
	{
		winver = "Windows Server 2012";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION )
	{
		winver = "Windows 8";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType != VER_NT_WORKSTATION )
	{
		winver = "Windows Server 2008 R2";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType == VER_NT_WORKSTATION )
	{
		winver = "Windows 7";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType != VER_NT_WORKSTATION )
	{
		winver = "Windows Server 2008";
	}
	else if ( osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType == VER_NT_WORKSTATION )
	{
		winver = "Windows Vista";
	}
	else if ( osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION && sys_info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 )
	{
		winver = "Windows XP x64";
	}
	else if ( osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 )
	{
		winver = "Windows Server 2003";
	}
	else if ( osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1 )
	{
		winver = "Windows XP";
	}
	else if ( osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0 )
	{
		winver = "Windows 2000";
	}
    else
	{
		winver = "unknown";
	}

    if ( osver.wServicePackMajor != 0 )
    {
        std::string sp;
        char buf[128] = { 0 };
        sp = " Service Pack ";
        sprintf_s(buf, sizeof(buf), "%hd", osver.wServicePackMajor);
        sp.append(buf);
        winver += sp;
    }

    return winver;
}


nodeoze::mac::address
machine_win32::get_mac_address( const std::string &name )
{
	IP_ADAPTER_INFO		adapterInfo[16];	// Allocate information for up to 16 NICs
	DWORD				dwBufLen = sizeof( adapterInfo );  // Save memory size of buffer
	DWORD				dwStatus = GetAdaptersInfo( adapterInfo, &dwBufLen );
	PIP_ADAPTER_INFO	pAdapterInfo;
	mac::address		ret;

	ncheck_error( dwStatus == ERROR_SUCCESS, exit, "GetAdaptersInfo() failed: %", ::GetLastError() );
 
	pAdapterInfo = adapterInfo; // Contains pointer to current adapter info
		
	while ( pAdapterInfo )
	{
		if ( ( pAdapterInfo->AddressLength == 6 ) && ( ( name.size() == 0 ) || ( pAdapterInfo->AdapterName == name ) ) )
		{
			ret = mac::address( pAdapterInfo->Address, pAdapterInfo->AddressLength );
			break;
		}

		pAdapterInfo = pAdapterInfo->Next;
	}

exit:

	return ret;
}


std::error_code
machine_win32::get_index_and_mask( sockaddr *addr, std::uint32_t &index, ip::address &mask )
{
	// Before calling AddIPAddress we use GetIpAddrTable to get
	// an adapter to which we can add the IP.
	
	PMIB_IPADDRTABLE	pIPAddrTable	= NULL;
	DWORD				dwSize			= 0;
	DWORD				i;
	auto				err				= std::error_code();

	// For now, this is only for IPv4 addresses.  That is why we can safely cast
	// addr's to sockaddr_in.

	ncheck_error_action_quiet( addr->sa_family == AF_INET, err = make_error_code( std::errc::invalid_argument ), exit );

	// Make an initial call to GetIpAddrTable to get the
	// necessary size into the dwSize variable

	for ( i = 0; i < 100; i++ )
	{
		auto res = GetIpAddrTable( pIPAddrTable, &dwSize, 0 );
		ncheck_error_action_quiet( ( res == ERROR_SUCCESS ) || ( res == ERROR_INSUFFICIENT_BUFFER ), err = std::error_code( res, std::system_category() ), exit );

		if ( res != ERROR_INSUFFICIENT_BUFFER )
		{
			break;
		}

		pIPAddrTable = (MIB_IPADDRTABLE *) realloc( pIPAddrTable, dwSize );
		ncheck_error_action_quiet( pIPAddrTable, err = make_error_code( std::errc::not_enough_memory ), exit );
	}

	err = make_error_code( std::errc::no_such_device_or_address );

	for ( i = 0; i < pIPAddrTable->dwNumEntries; i++ )
	{
		if ( ( ( struct sockaddr_in* ) addr )->sin_addr.s_addr == pIPAddrTable->table[i].dwAddr )
		{
			index	= pIPAddrTable->table[i].dwIndex;
			mask	= ip::address( static_cast< std::uint32_t >( pIPAddrTable->table[i].dwMask ) );
			err		= std::error_code();
			break;
		}
	}

exit:

	if ( pIPAddrTable )
	{
		free( pIPAddrTable );
	}

	return err;
}


void
machine_win32::setup_printer_notifications()
{
	mlog( marker::machine, log::level_t::info, "establishing connection to local print spooler..." );

	if ( !OpenPrinter( nullptr, &m_local_printer_spooler, NULL ) )
	{
		mlog( marker::machine, log::level_t::warning, "OpenPrinter() failed with err %. Waiting % seconds to retry...", ::GetLastError(), 5 );

		runloop::shared().schedule_oneshot_timer( std::chrono::seconds( 5 ), [=]( runloop::event event ) 
		{
			setup_printer_notifications();
		} );

		goto exit;
	}

	m_local_printer_change_handle = FindFirstPrinterChangeNotification( m_local_printer_spooler, PRINTER_CHANGE_ADD_PRINTER | PRINTER_CHANGE_DELETE_PRINTER, 0, NULL );

	if ( m_local_printer_change_handle == INVALID_HANDLE_VALUE )
	{
		mlog( marker::machine, log::level_t::warning, "FindFirstPrinterChangeNotification() failed: %", GetLastError() );
		goto exit;
	}

	m_local_printer_change_event = runloop::shared().create( m_local_printer_change_handle );

exit:

	return;
}


void
machine_win32::schedule_printer_notifications()
{
	if ( m_local_printer_change_event )
	{
		runloop::shared().schedule( m_local_printer_change_event, [=]( runloop::event event ) mutable
		{
			DWORD change;

			mlog( marker::machine, log::level_t::info, "got local printer notification. starting printer task..." );

			if ( FindNextPrinterChangeNotification( m_local_printer_change_handle, &change, NULL, NULL ) )
			{
				runloop::shared().dispatch( [=]()
				{
					notification::shared().publish( notification::local, 0, make_oid( self ), printer_change_event, any::null() );
				} );
			}
			else
			{
				mlog( marker::machine, log::level_t::error, "FindNextPrinterChangeNotification() failed: %", ::GetLastError() );
				teardown_printer_notifications();
			}
		} );
	}
}


void
machine_win32::teardown_printer_notifications()
{
	if ( m_local_printer_change_event )
	{
		runloop::shared().cancel( m_local_printer_change_event );
		m_local_printer_change_event = nullptr;
	}

	if ( m_local_printer_change_handle != INVALID_HANDLE_VALUE )
	{
		FindClosePrinterChangeNotification( m_local_printer_change_handle );
		m_local_printer_change_handle = INVALID_HANDLE_VALUE;
	}

	if ( m_local_printer_spooler )
	{
		ClosePrinter( m_local_printer_spooler);
		m_local_printer_spooler = nullptr;
	}
}
