#include <nodeoze/filesystem.h>
#include <sys/stat.h>
#include <cstdio>

bool
std::experimental::filesystem::v1::__remove( const path& p, std::error_code *errp )
{
    bool result = true;
    if ( errp != nullptr )
    {
        *errp = std::error_code{};
    }
    auto status = ::remove( p.string().c_str() );
    if ( status != 0 )
    {
        if ( errp != nullptr )
        {
            *errp = std::error_code{ errno, std::generic_category() };
            result = false;
        }
        else
        {
            throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }
    }
    return result;
}

std::experimental::filesystem::file_status 
std::experimental::filesystem::v1::__status( const path& p, std::error_code *errp )
{
    if ( errp != nullptr )
    {
        *errp = std::error_code{};
    }
    struct stat sbuf;
    auto s = ::stat( p.string().c_str(), &sbuf );
    if ( s != 0 )
    {
        if ( errno == ENOENT )
        {
            return file_status{ file_type::not_found };
        }
        else
        {
            if ( errp != nullptr )
            {
                *errp = std::error_code{ errno, std::generic_category() };
                return file_status{ file_type::none };
            }
            else
            {
                throw std::system_error{ std::error_code{ errno, std::generic_category() } };
            }
        }
    }
    else
    {
        file_type ft = file_type::unknown;

        if ( S_ISREG( sbuf.st_mode ) )
        {
            ft = file_type::regular;
        }
        else if ( S_ISDIR( sbuf.st_mode ) )
        {
            ft = file_type::directory;
        }
        else if ( S_ISBLK( sbuf.st_mode ) )
        {
            ft = file_type::block;
        }
        else if ( S_ISCHR( sbuf.st_mode ) )
        {
            ft = file_type::character;
        }
        else if ( S_ISFIFO( sbuf.st_mode ) )
        {
            ft = file_type::fifo;
        }
        else if ( S_ISLNK( sbuf.st_mode ) )
        {
            ft = file_type::symlink;
        }
        else if ( S_ISSOCK( sbuf.st_mode ) )
        {
            ft = file_type::socket;
        }
        return file_status{ ft };
    }
}

void 
std::experimental::filesystem::v1::__rename( const path& from, const path& to, error_code *errp )
{
    if ( errp != nullptr )
    {
        *errp = std::error_code{};
    }
	auto result = ::rename( from.string().c_str(), to.string().c_str() );
    if ( result != 0 )
    {
        if ( errp != nullptr )
        {
            *errp = std::error_code{ errno, std::generic_category() };
        }
        else
        {
            throw std::system_error{ std::error_code{ errno, std::generic_category() } };
        }
    }
}
