#include <nodeoze/path.h>
#include <Foundation/Foundation.h>
#include <sys/stat.h>
#include <dlfcn.h>

using namespace nodeoze;

extern "C"
{
	static void
	symbol()
	{
	}
}


path
path::self()
{
	Dl_info		module_info;
	std::string	ret;
	
	if ( dladdr( reinterpret_cast< void* >( symbol ), &module_info ) != 0 )
	{
		ret = module_info.dli_fname;
	}
	
	return path( ret );
}


path
path::home()
{
	return path( [ NSHomeDirectory() UTF8String ] );
}


path
path::app_data()
{
	auto paths = NSSearchPathForDirectoriesInDomains( NSApplicationSupportDirectory, NSLocalDomainMask, YES );
	return path( [ [ paths objectAtIndex:0 ] UTF8String ] );
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
	
	@autoreleasepool
	{
		NSArray *components = [ [ NSString stringWithUTF8String:m_path.c_str() ] pathComponents ];
		
		for ( NSString *component in components )
		{
			ret.push_back( [ component UTF8String ] );
		}
	}
	
	return ret;
}


path
path::parent_folder() const
{
	std::string ret;
	
	@autoreleasepool
	{
		ret = ( [ [ [ NSString stringWithUTF8String:m_path.c_str() ] stringByDeletingLastPathComponent ] UTF8String ] );
	}
	
	return ret;
}

	
std::string
path::extension() const
{
	std::string ret;
	
	@autoreleasepool
	{
		ret = [ [ [ NSString stringWithUTF8String:m_path.c_str() ] pathExtension ] UTF8String ];
	}
	
	return ret;
}

	
std::string
path::filename() const
{
	std::string ret;
	
	@autoreleasepool
	{
		ret = [ [ [ NSString stringWithUTF8String:m_path.c_str() ] lastPathComponent ] UTF8String ];
	}
	
	return ret;
}


std::vector< path >
path::children() const
{
	std::vector< path > kids;
	
	@autoreleasepool
	{
		NSDirectoryEnumerator	*enumerator = [ [ NSFileManager defaultManager ] enumeratorAtPath:[ NSString stringWithUTF8String:to_string().c_str() ] ];
		NSString				*filename;
		
		while ( ( filename = [ enumerator nextObject ] ) )
		{
			kids.emplace_back( [ filename UTF8String ] );
		}
	}
	
	return kids;
}


path&
path::append( const path &component )
{
	@autoreleasepool
	{
		m_path = [ [ [ NSString stringWithUTF8String:m_path.c_str() ] stringByAppendingPathComponent:[ NSString stringWithUTF8String:component.m_path.c_str() ] ] UTF8String ];
	}
	
	return *this;
}

