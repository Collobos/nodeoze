#ifndef NODEOZE_RAFT_TYPES_H
#define NODEOZE_RAFT_TYPES_H

#include <cstdint>
#include <cstddef>
#include <system_error>
#include <nodeoze/raft/error.h>

namespace nodeoze
{
namespace raft
{

// zero is reserved for null (non-existent) server
using replicant_id_type = 		std::uint64_t;
using index_type = 				std::uint64_t;
using term_type =				std::uint64_t;
using file_position_type =      std::int64_t;

inline void
clear_error( std::error_code& err )
{
	static const auto ok = make_error_code( raft::errc::ok );
	err = ok;
}

} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAFT_TYPES_H
