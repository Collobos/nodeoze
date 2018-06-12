#ifndef NODEOZE_RAFT_CONFIG_H
#define NODEOZE_RAFT_CONFIG_H

#include <string>
#include <chrono>

namespace nodeoze
{
namespace raft
{

class configuration
{
public:
	configuration(server_id_type id)
	:
	m_id{id},
	m_log_dir{"logs/"},
	m_log_file_name{std::string("server_").append(std::to_string(id).append(".log")},
	m_log_temp_name{std::string("server_").append(std::to_string(id).append(".tmp")},
	m_election_timeout{std::chrono::milliseconds{200}},
	m_heartbeat_timeout{std::chrono::milliseconds{50}}
	{}

	inline server_id_type
	id() const noexcept
	{
		return m_id;
	}

	std::string const&
	log_directory() const noexcept
	{
		return m_log_dir;
	}

	inline void
	log_directory(std::string const& dir)
	{
		m_log_dir = dir;
	}

	inline std::string const&
	log_filename() const noexcept
	{
		return m_log_filename;
	}

	inline void
	log_filename(std::string const& filename) 
	{
		m_log_filename = filename;
	}

	inline std::string const&
	log_temp_filename() const noexcept
	{
		return m_log_temp_filename;
	}

	inline void
	log_temp_filename(std::string const& filename) 
	{
		m_log_temp_filename = filename;
	}

	inline std::string 
	log_full_pathname() const noexcept
	{
		std::string pathname{m_log_dir};
		pathname.append(m_log_filename);
		return pathname;
	}

	inline std::string
	log_temp_full_pathname() const noexcept
	{
		std::string pathname{m_log_dir};
		pathname.append(m_log_temp_filename);
		return pathname;
	}
		
private:
	server_id_type	m_id;
	std::string m_log_dir;
	std::string m_log_filename;
	std::string m_log_temp_filename;
	std::chrono::milliseconds m_election_timeout;
	std::chrono::milliseconds m_heartbeat_timeout;
};

} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAFT_CONFIG_H
