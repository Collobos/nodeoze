#pragma once

#if defined( __APPLE__ )

#include <experimental/filesystem>

namespace nodeoze {

namespace filesystem {

typedef std::experimental::filesystem::path path;

using std::experimental::filesystem::temp_directory_path;

path
current_directory_path( std::error_code &err );

inline path
current_directory_path()
{
	auto err = std::error_code();
	auto ret = current_directory_path( err );

	if ( err )
	{
		throw std::system_error( err );
	}

    return ret;
} 

path
home_directory_path( std::error_code &err );

inline path
home_directory_path()
{
	auto err = std::error_code();
	auto ret = home_directory_path( err );

	if ( err )
	{
		throw std::system_error( err );
	}

    return ret;
}

path
app_data_directory_path( std::error_code &err );

inline path
app_data_directory_path()
{
	auto err = std::error_code();
	auto ret = app_data_directory_path( err );

	if ( err )
	{
		throw std::system_error( err );
	}

    return ret;
}

using std::experimental::filesystem::exists;

using std::experimental::filesystem::rename;

using std::experimental::filesystem::remove;

}

}

#elif defined( __linux__ )

#elif defined( WIN32 )

#endif

