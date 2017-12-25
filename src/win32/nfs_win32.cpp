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

#include "nfs_win32.h"
#include <nodeoze/nmacros.h>
#include <shlobj.h>
#include <uv.h>
#include <shellapi.h>
#include <io.h>

#pragma comment( lib, "shell32.lib" )
#pragma comment( lib, "Shlwapi.lib" )

static void
make_writeable( const std::wstring& filename)
{
  DWORD dwAttrs = ::GetFileAttributes(filename.c_str() );
  if (dwAttrs==INVALID_FILE_ATTRIBUTES) return;
 
  if (dwAttrs & FILE_ATTRIBUTE_READONLY)
  {
    ::SetFileAttributes(filename.c_str(),
    dwAttrs & (~FILE_ATTRIBUTE_READONLY));
  }
}
 
using namespace nodeoze;

NODEOZE_DEFINE_SINGLETON( fs )

fs*
fs::create()
{
	return new fs_win32;
}


fs_win32::fs_win32()
{
}


fs_win32::~fs_win32()
{
}


path
fs_win32::make_safe( const path &p )
{
	std::string safe;

	for ( auto &it : p.to_string() )
	{
		if ( ( it == '\\' )	||
		     ( it == '/' )		||
			 ( it == ':' )		||
			 ( it == '*' )		||
			 ( it == '?' )		||
		     ( it == '"' )		||
			 ( it == '<' )		||
			 ( it == '>' )		||
			 ( it == '|' )		||
			 ( it == '\n' ) )
		{
			safe.push_back( '_' );
		}
		else
		{
			safe.push_back( it );
		}
	}

	return safe;
}


bool
fs_win32::exists( const path &p )
{
	return ( GetFileAttributes( widen( p.to_string() ).c_str() ) != INVALID_FILE_ATTRIBUTES ) ? true : false;
}


bool
fs_win32::is_executable( const path &p )
{
	return ( GetFileAttributes( widen( p.to_string() ).c_str() ) & FILE_EXECUTE ) ? true : false;
}


bool
fs_win32::is_directory( const path &p )
{
	return ( GetFileAttributes( widen( p.to_string() ).c_str() ) & FILE_ATTRIBUTE_DIRECTORY ) ? true : false;
}


bool
fs_win32::is_regular_file( const path &p )
{
	return !is_directory( p );
}


std::size_t
fs_win32::size( const path &p )
{
	std::size_t size = 0;
	auto		file = CreateFile( widen( p.to_string() ).c_str(), GENERIC_READ , 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

	if ( file != INVALID_HANDLE_VALUE )
	{
		size = GetFileSize( file, nullptr );
		CloseHandle( file );
	}

	return size;
}


std::chrono::system_clock::time_point
fs_win32::created( const path &p )
{
	uv_stat_t stat;
	auto ret = std::chrono::system_clock::time_point();

	if ( uv_fs_stat( uv_default_loop(), reinterpret_cast< uv_fs_t* >( &stat ), p.to_string().c_str(), nullptr ) == 0 )
	{
		ret = std::chrono::system_clock::from_time_t( stat.st_birthtim.tv_sec );
	}

	return ret;
}
	

std::chrono::system_clock::time_point
fs_win32::modified( const path &p )
{
	uv_stat_t stat;
	auto ret = std::chrono::system_clock::time_point();

	if ( uv_fs_stat( uv_default_loop(), reinterpret_cast< uv_fs_t* >( &stat ), p.to_string().c_str(), nullptr ) == 0 )
	{
		ret = std::chrono::system_clock::from_time_t( stat.st_mtim.tv_sec );
	}

	return ret;
}


std::vector< nodeoze::path >
fs_win32::list( const path &p )
{
	WIN32_FIND_DATA					ffd;
	std::vector< nodeoze::path >	ret;
   
	auto handle = FindFirstFile( widen( p.to_string() + "\\*" ).c_str(), &ffd );

	if ( handle != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( !( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
			{
				ret.emplace_back( narrow( ffd.cFileName ) );
			}
		}
		while ( FindNextFile( handle, &ffd ) != 0 );

		FindClose( handle );
	}

	return ret;
}


std::error_code
fs_win32::mkdir( const path &p )
{
	auto ret = std::error_code();

	auto err = SHCreateDirectoryEx( nullptr, widen( p.to_string() ).c_str(), NULL );
	ncheck_error_action( ( err == ERROR_SUCCESS ) || ( err == ERROR_ALREADY_EXISTS ), ret = std::error_code( err, std::system_category() ), exit, "SHCreateDirectoryEx() failed creating directory % (%)", p, err );

exit:

	return ret;
}


std::error_code
fs_win32::copy( const path &from, const path &to )
{
	auto err = std::error_code();

	if ( !::CopyFile( widen( from.to_string() ).c_str(), widen( to.to_string() ).c_str(), FALSE ) )
	{
		err = std::error_code( ::GetLastError(), std::system_category() );
	}

	return err;
}


std::error_code
fs_win32::move( const path &from, const path &to )
{
	auto err = std::error_code();

	if ( !::MoveFile( widen( from.to_string() ).c_str(), widen( to.to_string() ).c_str() ) )
	{
		err = std::error_code( ::GetLastError(), std::system_category() );
	}

	return err;
}


std::error_code
fs_win32::unlink( const path &p )
{
	auto err = std::error_code();

	if ( !::DeleteFile( widen( p.to_string() ).c_str() ) )
	{
		err = std::error_code( ::GetLastError(), std::system_category() );
	}

	return err;
}


std::error_code
fs_win32::rmdir( const path &p )
{
	WIN32_FIND_DATA					ffd;
	auto							folder = widen( p.to_string() );
	auto							search = folder + std::wstring( TEXT( "\\**" ) );
	auto							err		= std::error_code();
   
	auto handle = FindFirstFile( search.c_str(), &ffd );

	if ( handle != INVALID_HANDLE_VALUE )
	{
		do
		{
			if ( ( _tcscmp( ffd.cFileName, TEXT( "." ) ) != 0 ) &&
				 ( _tcscmp( ffd.cFileName, TEXT( ".." ) ) != 0 ) )
			{
				auto fullpath = p + path( narrow( ffd.cFileName ) );

				if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
				{
					rmdir( fullpath );
				}
				else
				{
					auto wide = widen( fullpath.to_string() );
					make_writeable( wide );
					ncheck_warning( DeleteFile( wide.c_str() ), "unable to delete %", fullpath );
				}
			}
		}
		while ( FindNextFile( handle, &ffd ) != 0 );

		FindClose( handle );
	}

	if ( !RemoveDirectory( folder.c_str() ) )
	{
		err = std::error_code( ::GetLastError(), std::system_category() );
	}

	return err;
}


bool
fs_win32::lock( marker m )
{
	HANDLE		handle		= reinterpret_cast< HANDLE >( _get_osfhandle(_fileno( reinterpret_cast< FILE* >( m.get() ) ) ) );
	DWORD		low			= 1;
	DWORD		high		= 0;
	OVERLAPPED	offset		=
	{
		0, 0, 0, 0, NULL
	};
	bool		ok			= true;

	ncheck_error_action( ( handle != 0 ) && ( handle != INVALID_HANDLE_VALUE ), ok = false, exit, "bad handle" );
	ok = LockFileEx( handle, LOCKFILE_EXCLUSIVE_LOCK + LOCKFILE_FAIL_IMMEDIATELY, 0, low, high, &offset ) ? true : false;
	ncheck_error( ok, exit, "LockFileEx() failed: %", ::GetLastError() );

exit:

	return ok;
}

	
bool
fs_win32::unlock( marker m )
{
	HANDLE		handle		= reinterpret_cast< HANDLE >( _get_osfhandle(_fileno( reinterpret_cast< FILE* >( m.get() ) ) ) );
	DWORD		low			= 1;
	DWORD		high		= 0;
	OVERLAPPED	offset		=
	{
		0, 0, 0, 0, NULL
	};
	bool		ok			= true;

	ncheck_error_action( ( handle != 0 ) && ( handle != INVALID_HANDLE_VALUE ), ok = false, exit, "bad handle" );
	ok = UnlockFileEx( handle, 0, low, high, &offset ) ? true : false;
	ncheck_error( ok, exit, "UnlockFileEx() failed: %", ::GetLastError() );

exit:

	return ok;
}
