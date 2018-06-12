#ifndef NODEOZE_RAFT_TYPES_H
#define NODEOZE_RAFT_TYPES_H

#include <cstdint>
#include <cstddef>
#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>

namespace nodeoze
{
namespace raft
{

// zero is reserved for null (non-existent) server
using replicant_id_type = 		std::uint64_t;
using index_type = 				std::uint64_t;
using term_type =				std::uint64_t;

} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAFT_TYPES_H
