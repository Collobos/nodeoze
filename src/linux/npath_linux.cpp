#include <nodeoze/npath.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nlog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <pwd.h>

using namespace nodeoze;

path
path::app_data()
{
	return path( std::string( "/var/lib" ) );
}


path
path::home()
{
	auto pw = getpwuid( getuid() );

	return ( pw ) ? path( pw->pw_dir ) : path();
}


path
path::tmp()
{
	return path( "/tmp" );
}


path::components
path::split() const
{
	path::components ret;

	ret = string::split( m_path, '/' );
	
	return ret;
}


path
path::self()
{
	char arg1[ 20 ];
	char exepath[ PATH_MAX + 1 ] = {0};

	sprintf( arg1, "/proc/%d/exe", getpid() );
	readlink( arg1, exepath, 1024 );
	return std::string( exepath );
}


path
path::parent_folder() const
{
	char me[ PATH_MAX ];
	char *parent;

	sprintf( me, "%s", to_string().c_str() );
	parent = dirname( me );

	return std::string( parent );
}

	
std::string
path::extension() const
{
	std::string	base( filename() );
	auto		idx = base.rfind( '.' );
	std::string ext;

	if ( idx != std::string::npos)
	{
    	ext = base.substr( idx+1 );
	}

	return ext;
}

	
std::string
path::filename() const
{
	char me[ PATH_MAX ];
	char *base;

	sprintf( me, "%s", to_string().c_str() );
	base = basename( me );

	return std::string( base );
}


std::vector< path >
path::children() const
{
	std::vector< path >	kids;
	auto				dfd = opendir( m_path.c_str() );
	dirent				*dp;

	ncheck_error( dfd, exit, "opendir() failed: %", errno );

	while ( ( dp = readdir( dfd ) ) != nullptr )
	{
		char filename_qfd[100] ;
		char new_name_qfd[100] ;
		struct stat stbuf ;

		sprintf( filename_qfd , "%s/%s", m_path.c_str(), dp->d_name );

		if ( stat( filename_qfd,&stbuf ) == 0 )
		{
			kids.push_back( filename_qfd );
		}
	}

exit:

	return kids;
}


path&
path::append( const path &component )
{
	m_path += "/" + component.m_path;
	return *this;
}
