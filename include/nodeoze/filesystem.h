#pragma once

#if defined( __APPLE__ )

#include <experimental/filesystem>

namespace nodeoze {

namespace filesystem = std::experimental::filesystem;

}

#elif defined( __linux__ )

#elif defined( WIN32 )

#endif

