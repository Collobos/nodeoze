#include <nodeoze/npath.h>

using namespace nodeoze;

path
path::self()
{
	return path( "" );
}


path
path::parent_folder()
{
	return *this;
}

	
std::string
path::extension()
{
	return "";
}

	
std::string
path::filename()
{
	return "";
}


void
path::append( const path &component )
{
}
