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

#include "nfs_android.h"
#include <nodeoze/nmacros.h>
#include <sys/stat.h>
#include <dirent.h>

using namespace nodeoze;

NODEOZE_DEFINE_SINGLETON( fs )

fs*
fs::create()
{
	return new fs_android;
}


fs_android::fs_android()
{
}


fs_android::~fs_android()
{
}


path
fs_android::make_safe( const path &p )
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
fs_android::exists( const path &p )
{
	struct stat buf;
	return ::stat( p.to_string().c_str(), &buf ) == 0 ? true : false;
}


std::size_t
fs_android::size( const path &p )
{
	std::size_t	size = 0;
	struct stat	buf;
	
	if ( ::stat( p.to_string().c_str(), &buf ) == 0 )
	{
		size = buf.st_size;
	}
	
	return size;
}


bool
fs_android::is_directory( const path &p )
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
fs_android::is_regular_file( const path &p )
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
fs_android::is_executable( const path &p )
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
fs_android::list( const nodeoze::path &path )
{
	std::vector< nodeoze::path > ret;
	dirent						*entry	= nullptr;
    DIR							*dp		= nullptr;

	dp = ::opendir( path.to_string().c_str() );
	nrequire( dp, exit );
	
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


bool
fs_android::mkdir( const path &p )
{
	return ::mkdir( p.to_string().c_str(), 0777 ) == 0 ? true : false;
}


bool
fs_android::copy( const path &from, const path &to )
{
	return false;
}


bool
fs_android::move( const path &from, const path &to )
{
	return false;
}


bool
fs_android::unlink( const path &p )
{
	return ::unlink( p.to_string().c_str() ) == 0 ? true : false;
}
