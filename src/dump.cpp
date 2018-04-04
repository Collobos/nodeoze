#include <nodeoze/bstream/utils/dump.h>
#include <string>
#include <iomanip>
#include <ctype.h>

using namespace nodeoze::bstream;

void utils::dump( std::ostream& os, const void * src, std::size_t nbytes)
{
    static const std::size_t bytes_per_line = 16;

    std::ios sstate(nullptr);
    sstate.copyfmt( os );

    os << std::endl;
    std::size_t remaining = nbytes;
    std::size_t line_index = 0ul;
    auto start = reinterpret_cast<const std::uint8_t*>(src);
    while ( remaining > 0ul )
    {

            if ( remaining >= bytes_per_line )
            {
                    os << std::hex << std::setfill( '0' );
                    os << std::setw(8) << line_index << ": ";
                    for ( auto i = 0ul; i < bytes_per_line; ++i )
                    {
                            auto byte = start[ line_index + i ];
                            os << std::setw(2) << (unsigned) byte << ' ';
                    }
                    os << "    ";
                    os.copyfmt( sstate );
                    for ( auto i = 0ul; i < bytes_per_line; ++i )
                    {
                            auto byte = start[ line_index + i ];
                            if ( isprint( byte ) )
                            {
                                    os << (char)byte;
                            }
                            else
                            {
                                    os << '.';
                            }
                    }
                    line_index += bytes_per_line;
                    remaining -= bytes_per_line;

                    os << std::endl;
            }
            else
            {
                    os << std::hex << std::setfill( '0' );
                    os << std::setw(8) << line_index << ": ";
                    for ( auto i = 0ul; i < remaining; ++i )
                    {
                            auto byte = start[ line_index + i ];
                            os << std::setw(2) << (unsigned)byte << ' ';
                    }
                    for ( auto i = 0ul; i < bytes_per_line - remaining; ++i )
                    {
                            os << "   ";
                    }
                    os << "    ";
                    os.copyfmt( sstate );
                    for ( auto i = 0ul; i < remaining; ++i )
                    {
                            auto byte = start[ line_index + i ];
                            if ( isprint( byte ) )
                            {
                                    os << (char) byte;
                            }
                            else
                            {
                                    os << '.';
                            }
                    }
                    os << std::endl;
                    remaining = 0;
            }

    }
    os.copyfmt( sstate ); // just to make sure
}

