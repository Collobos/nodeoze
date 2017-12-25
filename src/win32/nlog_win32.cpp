#include <nodeoze/nlog.h>
#include <nodeoze/nunicode.h>
#include "event_log.h"
#include <winsock2.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include <mutex>

using namespace nodeoze;

static HANDLE g_event_source	= nullptr;

void
log::init( const std::string &name )
{
	m_name = name;

	HKEY key = NULL;

	if ( !m_mutex )
	{
		m_mutex = new std::recursive_mutex;
		init_limiter();
	}

	if ( g_event_source == nullptr )
	{
		TCHAR			path[ MAX_PATH ];
		int				typesSupported;
		int				err;
		int				n;
	
		// Build the path string using the fixed registry path and app name.
	
		auto key_name = std::wstring( TEXT( "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" ) ) + widen( name );
	
    	// Add/Open the source name as a sub-key under the Application key in the EventLog registry key.
	
    	err = RegCreateKeyEx( HKEY_LOCAL_MACHINE, key_name.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, NULL );
   	 
    	if ( err )
    	{
			goto exit;
		}
	
    	// Set the path in the EventMessageFile subkey. Add 1 to the std::tchar_t count to include the null terminator.
	
    	n = GetModuleFileName( NULL, path, sizeof( path ) / sizeof( TCHAR ) );
   	 
    	if ( n == 0 )
    	{
			goto exit;
		}
	
    	n += 1;
    	n *= sizeof( TCHAR );
	
    	err = RegSetValueEx( key, TEXT( "EventMessageFile" ), 0, REG_EXPAND_SZ, (const LPBYTE) path, n );
   	 
    	if ( err )
    	{
			goto exit;
		}
	
    	// Set the supported event types in the TypesSupported subkey.

    	typesSupported = EVENTLOG_SUCCESS | EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE |  EVENTLOG_AUDIT_SUCCESS | EVENTLOG_AUDIT_FAILURE;
    	err = RegSetValueEx( key, TEXT( "TypesSupported" ), 0, REG_DWORD, (const LPBYTE) &typesSupported, sizeof( DWORD ) );
	
		if ( err )
		{
			goto exit;
		}
	
		g_event_source = RegisterEventSource( NULL, widen( name ).c_str() );
		
		std::atexit( []()
		{
			if ( g_event_source )
			{
				DeregisterEventSource( g_event_source );
			}
			
		exit:

			return;
		} );
	}

exit:

	if ( key )
	{
		RegCloseKey( key );
	}
}


void
log::cleanup( const std::string &name )
{
}


void
log::put_system_log( log::level_t l, const std::string &message )
{
	if ( g_event_source )
	{
		WORD		type;
		const char	*array[ 1 ];
		BOOL		ok;

		// Map the debug level to a Windows EventLog type.
	
		if ( l == log::level_t::warning )
		{
			type = EVENTLOG_WARNING_TYPE;
		}
		else if ( l == log::level_t::error )
		{
			type = EVENTLOG_ERROR_TYPE;
		}
		else
		{
			type = EVENTLOG_INFORMATION_TYPE;
		}
	
		// Add the the string to the event log.

		array[ 0 ] = message.c_str();
	
		ok = ReportEventA( g_event_source, type, 0, NETKIT_LOG, NULL, 1, 0, array, NULL );
	}
}


void
log::put_console_log( const std::string &message )
{
	OutputDebugStringA( message.c_str() );
	OutputDebugStringA( "\n" );
	fprintf( stderr, "%s\n", message.c_str() );
}
