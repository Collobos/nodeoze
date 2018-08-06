#ifndef NODEOZE_RAFT_ERROR_H
#define NODEOZE_RAFT_ERROR_H

#include <system_error>

namespace nodeoze
{
namespace raft
{

enum class errc : int
{
	ok = 0,
	log_checksum_error,
	log_incomplete_record,
	log_corrupt,
	log_index_out_of_range,
	log_unexpected_state,
	log_server_id_error,
	log_frame_type_error,
	log_entry_type_error,
	log_recovery_error,
};

std::error_category const& raft_category() noexcept;

class raft_category_impl : public std::error_category
{
public:
	virtual const char* name() const noexcept override;

	virtual std::string message( int ev ) const noexcept override;
};

std::error_condition make_error_condition( nodeoze::raft::errc e );

std::error_code make_error_code( nodeoze::raft::errc e );


} // namespace raft
} // namespace nodeoze

namespace std
{
  template <>
  struct is_error_condition_enum< nodeoze::raft::errc > : public true_type {};
}

#endif // NODEOZE_RAFT_ERROR_H
