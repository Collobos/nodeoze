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

#include "nfs_apple.h"
#include <Foundation/Foundation.h>
#include <nodeoze/nmacros.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/file.h>


using namespace nodeoze;

NODEOZE_DEFINE_SINGLETON( fs )

fs*
fs::create()
{
	return new fs_apple;
}


fs_apple::fs_apple()
{
}


fs_apple::~fs_apple()
{
}


path
fs_apple::make_safe( const path &p )
{
	std::string s;
	
	for ( auto &it : p.to_string() )
    {
		if ( ( it == ':' ) || ( it == '\n' ) )
		{
			s.push_back( '_' );
        }
		else
		{
			s.push_back( it );
		}
    }
	
	return path( s );
}


bool
fs_apple::exists( const path &p )
{
	struct stat buf;
	return ::stat( p.to_string().c_str(), &buf ) == 0 ? true : false;
}


std::size_t
fs_apple::size( const path &p )
{
	std::size_t	size = 0;
	struct stat	buf;
	
	if ( ::stat( p.to_string().c_str(), &buf ) == 0 )
	{
		size = buf.st_size;
	}
	
	return size;
}


std::chrono::system_clock::time_point
fs_apple::created( const path &p )
{
	struct stat	buf;
	auto ret = std::chrono::system_clock::time_point();
	
	if ( ::stat( p.to_string().c_str(), &buf ) == 0 )
	{
		ret = std::chrono::system_clock::from_time_t( buf.st_birthtimespec.tv_sec );
	}
	
	return ret;
}

	
std::chrono::system_clock::time_point
fs_apple::modified( const path &p )
{
	struct stat	buf;
	auto ret = std::chrono::system_clock::time_point();
	
	if ( ::stat( p.to_string().c_str(), &buf ) == 0 )
	{
		ret = std::chrono::system_clock::from_time_t( buf.st_mtimespec.tv_sec );
	}
	
	return ret;
}


bool
fs_apple::is_directory( const path &p )
{
	struct stat	buf;
	bool		ok = false;
	
	if ( ::stat( p.to_string().c_str(), &buf ) == 0 )
	{
		ok = S_ISDIR( buf.st_mode );
	}
	
	return ok;
}


bool
fs_apple::is_regular_file( const path &p )
{
	struct stat	buf;
	bool		ok = false;
	
	if ( ::stat( p.to_string().c_str(), &buf ) == 0 )
	{
		ok = S_ISREG( buf.st_mode );
	}
	
	return ok;
}


bool
fs_apple::is_executable( const path &p )
{
	struct stat	buf;
	bool		ok = false;
	
	if ( ::stat( p.to_string().c_str(), &buf ) == 0 )
	{
		ok  = ( buf.st_mode & S_IXUSR ) ? true : false;
	}
	
	return ok;
}


std::vector< path >
fs_apple::list( const nodeoze::path &path )
{
	std::vector< nodeoze::path > ret;
	dirent						*entry	= nullptr;
    DIR							*dp		= nullptr;

	dp = ::opendir( path.to_string().c_str() );
	ncheck_error_quiet( dp, exit );
	
	while ( ( entry = readdir( dp ) ) )
	{
		ret.emplace_back( path + entry->d_name );
	}

exit:

	if ( dp )
	{
		closedir(dp);
	}

	return ret;
}


std::error_code
fs_apple::mkdir( const path &p )
{
	auto mask	= umask( 0 );
	auto err	= std::error_code();
	
	if ( !is_directory( p ) )
	{
		if ( p.split().size() != 0 )
		{
			err = mkdir( p.parent_folder() );
			
			if ( !err )
			{
				if ( ::mkdir( p.to_string().c_str(), 0777 ) != 0 )
				{
					err = std::error_code( errno, std::generic_category() );
				}
			}
		}
	}
	
	umask( mask );
	
	return err;
}


std::error_code
fs_apple::copy( const path &from, const path &to )
{
	NSError *error	= nil;
	auto	ret		= std::error_code();
	
	auto ok = [ [ NSFileManager defaultManager ] copyItemAtPath:[ NSString stringWithUTF8String:from.to_string().c_str() ] toPath:[ NSString stringWithUTF8String:to.to_string().c_str() ] error:&error ];
	
	if ( !ok )
	{
		ret = std::error_code( static_cast< int >( [ error code ] ), std::generic_category() );
	}
	
	return ret;
}


std::error_code
fs_apple::move( const path &from, const path &to )
{
	auto ret = std::error_code();
	
	if ( ::rename( from.to_string().c_str(), to.to_string().c_str() ) != 0 )
	{
		ret = std::error_code( errno, std::generic_category() );
	}
	
	return ret;
}


std::error_code
fs_apple::unlink( const path &p )
{
	auto ret = std::error_code();
	
	if ( ::unlink( p.to_string().c_str() ) != 0 )
	{
		ret = std::error_code( errno, std::generic_category() );
	}
	
	return ret;
}


std::error_code
fs_apple::rmdir( const path &p )
{
	NSError *error = nil;
	auto ret = std::error_code();
	
	auto ok = [ [ NSFileManager defaultManager ] removeItemAtPath:[ NSString stringWithUTF8String:p.to_string().c_str() ] error:&error ];
	
	if ( !ok )
	{
		ret = std::error_code( static_cast< int >( [ error code ] ), std::generic_category() );
	}
	
	return ret;
}


bool
fs_apple::lock( marker m )
{
	int fd = fileno( reinterpret_cast< FILE* >( m.get() ) );
	if ( fd < 0 )
	{
		return false;
	}
	int rc = flock( fd, LOCK_EX | LOCK_NB );
	return rc == 0;
}
	
bool
fs_apple::unlock( marker m )
{
	int fd = fileno( reinterpret_cast< FILE* >( m.get() ) );
	if ( fd < 0 )
	{
		return false;
	}
	int rc = flock( fd, LOCK_UN );
	return rc == 0;
}
	


