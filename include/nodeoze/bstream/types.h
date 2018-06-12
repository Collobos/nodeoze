#ifndef NODEOZE_BSTREAM_TYPES_H
#define NODEOZE_BSTREAM_TYPES_H

#include <cstdint>
#include <nodeoze/bstream/error.h>

namespace nodeoze 
{
namespace bstream
{

    using size_type = std::size_t;
    using position_type = std::int64_t;
    using offset_type  = std::int64_t;
    using byte_type = std::uint8_t;

    static constexpr position_type invalid_position = -1;

    // inline void
    // clear_error( std::error_code& err )
    // {
    //     static const std::error_code ok = make_error_code( bstream::errc::ok );
    //     err = ok;
    // }

    enum class seek_anchor
    {
        begin,
        current,
        end
    };

    enum class open_mode
    {
        truncate,
        append,
        at_end,
        at_begin,
    };


} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_TYPES_H