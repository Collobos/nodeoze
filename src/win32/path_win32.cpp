#include <nodeoze/npath.h>
#include <nodeoze/nlog.h>
#include <nodeoze/nmacros.h>
#include <WinSock2.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <tchar.h>

using namespace nodeoze;

path
path::home()
{
	TCHAR	root[ MAX_PATH ];
	path	p;

	auto res = SHGetFolderPath( NULL, CSIDL_PROFILE, NULL, 0, root ); 
	ncheck_error( SUCCEEDED( res ), exit, "SHGetFolderPath() failed: %", ::GetLastError() );
	p = narrow( root );

exit:

	return p;
}


path
path::app_data()
{
	TCHAR	root[ MAX_PATH ];
	path	p;

	auto res = SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, root ); 
	ncheck_error( SUCCEEDED( res ), exit, "SHGetFolderPath() failed: %", ::GetLastError() );
	p = narrow( root );

exit:

	return p;
}


path
path::system_drive()
{
	TCHAR buffer[ MAX_PATH ];

    GetSystemWindowsDirectory( buffer, sizeof( buffer ) );

	return narrow( std::wstring( buffer, buffer + 2 ) );
}


path
path::system()
{
	TCHAR	root[ MAX_PATH ];
	path	p;

	auto res = SHGetFolderPath( NULL, CSIDL_SYSTEM, NULL, 0, root ); 
	ncheck_error( SUCCEEDED( res ), exit, "SHGetFolderPath() failed: %", ::GetLastError() );
	p = narrow( root );

exit:

	return p;
}


path
path::tmp()
{
	wchar_t filename[ MAX_PATH ];

	GetTempPath( MAX_PATH, filename );

	return path( narrow( filename ) );
}


path
path::self()
{
	wchar_t filename[ MAX_PATH ];

	GetModuleFileName( nullptr, filename, MAX_PATH );

	return path( narrow( filename ) );
}


path
path::parent_folder() const
{
	wchar_t	dir[ MAX_PATH ];
	wchar_t drive[ MAX_PATH ];

	_tsplitpath_s( widen( m_path ).c_str(), drive, 8, dir, MAX_PATH, nullptr, 0, nullptr, 0 );

	return path( narrow( _tcscat( drive, dir ) ) );
}

	
std::string
path::extension() const
{
	wchar_t	ext[ MAX_PATH ];
	wchar_t drive[8];

	_tsplitpath_s( widen( m_path ).c_str(), drive, 8, nullptr, 0, nullptr, 0, ext, MAX_PATH );

	return narrow( ext );
}

	
std::string
path::filename() const
{
	wchar_t	file[ MAX_PATH ];
	wchar_t ext[ MAX_PATH ];
	wchar_t drive[8];

	_tsplitpath_s( widen( m_path ).c_str(), drive, 8, nullptr, 0, file, MAX_PATH, ext, MAX_PATH );
	_tcscat_s( file, sizeof( file ) / sizeof( TCHAR ), ext );

	return narrow( file );
}


std::vector< path >
path::children() const
{
	std::vector< path > kids;
	HANDLE				handle = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA		find_file_data;
	TCHAR				fullpath[ MAX_PATH ];

    GetFullPathName( widen( m_path ).c_str(), MAX_PATH, fullpath, 0);
    std::wstring fp( fullpath );

	fp += TEXT( "\\*" );

	handle = FindFirstFile( fp.c_str(), &find_file_data );
	ncheck_error( handle != INVALID_HANDLE_VALUE, exit, "FindFirstFile() failed: %", ::GetLastError() );

	do 
	{
		if ( find_file_data.cFileName[0] != '.' )
		{
			kids.push_back( path( narrow( find_file_data.cFileName ) ) );
		}
	}
	while ( FindNextFile( handle, &find_file_data ) != 0 );

exit:
	
	return kids;
}


path&
path::append( const path &component )
{
	wchar_t tmp[ MAX_PATH ];

	_tcscpy_s( tmp, widen( m_path ).c_str() );
	PathAppend( tmp, widen( component.m_path ).c_str() );
	m_path = narrow( tmp );

	return *this;
}
