#ifndef NODEOZE_BSTREAM_TYPES_H
#define NODEOZE_BSTREAM_TYPES_H

#include <cstdint>
#include <system_error>

namespace nodeoze 
{
namespace bstream
{

    using size_type = std::size_t;
    using position_type = std::int64_t;
    using offset_type  = std::int64_t;
    using byte_type = std::uint8_t;
    using poly_tag_type = int;

    static constexpr poly_tag_type invalid_tag = -1;
    static constexpr position_type invalid_position = -1;

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

	inline void
	clear_error( std::error_code& ec )
	{
		static const std::error_code good = std::error_code{};
		ec = good;
	}


} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_TYPES_H