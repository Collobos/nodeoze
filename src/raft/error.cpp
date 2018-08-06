#include <nodeoze/raft/error.h>
#include <string>

using namespace nodeoze;

const char* raft::raft_category_impl::name() const noexcept
{
	return "nodeoze raft";
}

std::string raft::raft_category_impl::message( int ev ) const noexcept
{
	switch ( static_cast< errc > ( ev ) )
	{
	case raft::errc::ok:
		return "success";
	case raft::errc::log_checksum_error:
		return "log checksum error";
	case raft::errc::log_incomplete_record:
		return "log incomplete record";
	case raft::errc::log_corrupt:
		return "log corrupt";
	case raft::errc::log_index_out_of_range:
		return "log index out of range";
	case raft::errc::log_unexpected_state:
		return "log unexpected state";
	case raft::errc::log_server_id_error:
		return "log server id error";
	case raft::errc::log_frame_type_error:
		return "log frame type error";
	case raft::errc::log_entry_type_error:
		return "log entry type error";
	case raft::errc::log_recovery_error:
		return "log recovery error";
	default:
		return "unknown raft error";
	}
}

const std::error_category& raft::raft_category() noexcept
{
    static raft::raft_category_impl instance;
    return instance;
}

std::error_condition raft::make_error_condition( raft::errc e )
{
    return std::error_condition( static_cast< int > ( e ), raft::raft_category() );
}

std::error_code raft::make_error_code( raft::errc e )
{
    return std::error_code( static_cast< int > ( e ), raft::raft_category() );
}


