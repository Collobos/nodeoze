#include <nodeoze/nstackwalk.h>
#include <WinSock2.h>
#include <dbghelp.h>
#include <sstream>

#pragma comment( lib, "dbghelp.lib" )

using namespace nodeoze;

std::string
stackwalk::copy()
{
	PVOID				addrs[ 25 ] = { 0 };
	std::ostringstream	os;

	::SymSetOptions( SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES );

	if ( !::SymInitialize( ::GetCurrentProcess(), "http://msdl.microsoft.com/download/symbols", TRUE ) )
	{
		goto exit;
	}
 
	// Capture up to 25 stack frames from the current call stack.  We're going to
	// skip the first stack frame returned because that's this function
	// itself, which we don't care about.

	auto frames = CaptureStackBackTrace( 1, 25, addrs, NULL );
 
	for ( auto i = 0; i < frames; i++ )
	{
		ULONG64		buffer[ (sizeof( SYMBOL_INFO ) + 1024 + sizeof( ULONG64 ) - 1) / sizeof( ULONG64 ) ] = { 0 };
		SYMBOL_INFO	*info = ( SYMBOL_INFO* ) buffer;

		info->SizeOfStruct	= sizeof( SYMBOL_INFO );
		info->MaxNameLen	= 1024;
 
		// Attempt to get information about the symbol and add it to our output parameter.

		DWORD64 displacement = 0;

		if (::SymFromAddr( ::GetCurrentProcess(), ( DWORD64 ) addrs[ i ], &displacement, info ) )
		{
			DWORD			dwDisplacement;
			IMAGEHLP_LINE64	line;

			line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );

			if ( SymGetLineFromAddr64( ::GetCurrentProcess(), (DWORD64)addrs[ i ], &dwDisplacement, &line ) )
			{
				std::string	filename	= line.FileName;
				auto		pos			= filename.rfind( '\\' );

				if ( pos != std::string::npos )
				{
					filename = filename.substr( pos + 1, filename.size() - pos - 1 );
				}

				os << filename << "(" << line.LineNumber << "): " << std::string( info->Name, info->NameLen ) <<  "()" << std::endl;
			}
			else
			{
				os << std::string( info->Name, info->NameLen ) << std::endl;
			}
        }
    }
 
	::SymCleanup( ::GetCurrentProcess() );

exit:

	return os.str();
}
