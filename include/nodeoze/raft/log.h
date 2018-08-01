#ifndef NODEOZE_RAFT_LOG_H
#define NODEOZE_RAFT_LOG_H

#include <memory>
#include <cstdint>
#include <system_error>
#include <unistd.h>
#include <cerrno>
#include <nodeoze/filesystem.h>
#include <nodeoze/raft/log_frames.h>

#ifndef NODEOZE_RAFT_LOG_FRAME_SIZE_HINT
#define NODEOZE_RAFT_LOG_FRAME_SIZE_HINT  4096ul
#endif // NODEOZE_RAFT_LOG_FRAME_SIZE_HINT

namespace nodeoze
{
namespace raft
{

class log
{
public:

	log( replicant_id_type id, std::string const& log_pathname, std::string const& log_temp_pathname )
	:
	m_self{ id },
	m_state{ std::make_shared< replicant_state >( id ) },
	m_log_pathname{ log_pathname },
	m_log_temp_pathname{ log_temp_pathname },
	m_os{}
	{}


	void
	restart( replicant_id_type self, std::error_code& err )
	{
		clear_error( err );
		m_self = self;
		m_state->clear( self );
		m_log.clear();

		filesystem::path tmp_path{ m_log_temp_pathname };

		auto found = filesystem::exists( tmp_path, err );
		if ( err ) goto exit;

		if ( found )
		{
			filesystem::remove( tmp_path, err );
			if ( err ) goto exit;
		}

		recover( err );
		if ( err ) goto exit;

		m_os.open( m_log_pathname, bstream::open_mode::append, err );
		if ( err ) goto exit;

		write_frame( m_state, err );
	exit:
		return;
	}

	void
	initialize( replicant_id_type self, term_type current_term, replicant_id_type voted_for, std::error_code& err )
	{
		clear_error( err );
		m_self = self;
		m_state->update( current_term, voted_for );
		m_log.clear();

		filesystem::path tmp_path{ m_log_temp_pathname };

		auto found = filesystem::exists( tmp_path, err );
		if ( err ) goto exit;

		if ( found )
		{
			filesystem::remove( tmp_path, err );
			if ( err ) goto exit;
		}
	
		m_os.open( m_log_pathname, bstream::open_mode::truncate, err );
		if ( err ) goto exit;

		write_frame( m_state, err );	

	exit:
		return;
	}

	inline void
	close( std::error_code& err )
	{
		write_frame( m_state, err );
		if ( err ) goto exit;

		m_os.close( err );

	exit:
		return;
	}

	inline void
	append( entry::ptr ep, std::error_code& err )
	{
		m_log.push_back( ep );

		write_frame( m_log.back(), err );
		if ( err ) goto exit;

		m_os.flush( err );

	exit:
		return;
	}

	inline void
	update_replicant_state( replicant_id_type self, term_type current_term, replicant_id_type voted_for, std::error_code& err )
	{
		update_replicant_state( replicant_state{ self, current_term, voted_for }, err );
	}

	inline void
	update_replicant_state( replicant_state const& new_state, std::error_code& err )
	{
		m_state->update( new_state );
		if ( m_state->is_dirty() )
		{
			write_frame( m_state, err );
			if ( ! err )
			{
				m_state->clean();
			}
		}
	}

	inline replicant_state const&
	current_replicant_state() const
	{
		return *m_state;
	}

	void
	write_frame( frame::ptr fp, bool flush = true )
	{
		fp->file_position( m_os.position() );
		bstream::ombstream frame_os{ NODEOZE_RAFT_LOG_FRAME_SIZE_HINT, get_log_context() };
		frame_os << fp;
		auto frame_buffer = frame_os.get_buffer();
		m_os.write_blob( frame_buffer );
		m_os.put_num( frame_buffer.checksum() );
		if ( flush )
		{
			m_os.flush();
		}
	}

	inline void
	write_frame( frame::ptr fp, std::error_code& err, bool flush = true )
	{
		clear_error( err );
		try
		{
			write_frame( fp, flush );
		}
		catch ( std::system_error const& e )
		{
			err = e.code();
		}
	}
	
	frame::ptr
	read_frame( bstream::ifbstream& is )
	{
		buffer framebuf = is.read_blob();
		auto buffer_checksum = framebuf.checksum();
		auto frame_checksum = is.get_num< buffer::checksum_type >();
		if ( buffer_checksum != frame_checksum )
		{
			throw std::system_error{ make_error_code( raft::errc::log_checksum_error ) };
		}
		bstream::imbstream framebuf_strm{ framebuf, get_log_context() };
		return framebuf_strm.read_as< frame::ptr >();
	}

	void
	truncate( file_position_type pos, std::error_code& err )
	{

		clear_error( err );
		m_os.position( pos, err );
		if ( err ) goto exit;

		m_os.truncate( err );

	exit:
		return;
	}

	void
	recover( std::error_code& err )
	{

		auto found = filesystem::exists( filesystem::path{ m_log_pathname }, err );
		if ( err ) return;

		if ( !found )
		{
			err = make_error_code( std::errc::no_such_file_or_directory );
		}
		else
		{
			try 
			{		
				bstream::ifbstream is;
				is.open( m_log_pathname );
				std::size_t file_size = is.size();
				m_state->clear( m_self );

				file_position_type frame_position = is.position();
				while ( static_cast< std::size_t >( frame_position ) < file_size )
				{
					auto fp = read_frame( is );
					switch ( fp->get_type() )
					{
						case frame_type::replicant_state_frame:
						{
							auto rp = std::dynamic_pointer_cast< replicant_state >( fp );
							assert( rp );
							assert( rp->file_position() == frame_position );
							m_state->update( *rp );
						}
						break;

						case frame_type::state_machine_update_frame:
						{
							auto ep = std::dynamic_pointer_cast< entry >( fp );
							assert( ep );
							m_log.push_back( ep );
							assert( m_log.back()->file_position() == frame_position );
						}
						break;

						default:
						{
							throw std::system_error{ make_error_code( raft::errc::log_frame_type_error ) };
						}
					}
					frame_position = is.position();
				}

				// if ( ! m_state.is_dirty() )
				// {
				// 	throw std::system_error{ make_error_code( raft::errc::log_recovery_error ) };
				// }
				is.close();
			}
			catch ( std::system_error const& e )
			{
				err = e.code();
			}
		}
	}

	inline bool
	empty() const
	{
		return m_log.empty();
	}

	inline entry::ptr
	back() const
	{
		if ( m_log.empty() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_index_out_of_range ) };
		}
		return m_log.back();
	}

	inline  entry::ptr
	front() const
	{
		if ( m_log.empty() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_index_out_of_range ) };
		}
		return m_log.front();
	}

	inline  entry::ptr
	operator[](index_type index) const
	{
		if (index_check(index))
		{
			throw std::system_error{ make_error_code( raft::errc::log_index_out_of_range ) };
		}
		auto i = index - front()->index();
		return m_log[i];
	}

	/*
	 *  remove entries with indices higher than the specified index
	 */
	void
	prune_back(index_type index, std::error_code& err)
	{
		clear_error( err );
		assert( ! m_log.empty() );
		if ( index < m_log.front()->index() || index > m_log.back()->index() )
		{
			err = make_error_code( std::errc::invalid_argument );
			goto exit;
		}

		if ( index < m_log.back()->index() )
		{
			file_position_type truncate_at = m_log.back()->file_position();
			while( ! m_log.empty() && m_log.back()->index() > index )
			{
				truncate_at = m_log.back()->file_position();
				m_log.pop_back();
			}
			if ( m_log.empty() || m_log.back()->index() != index )
			{
				err = make_error_code( std::errc::state_not_recoverable );
				goto exit;
			}
			truncate( truncate_at, err );
			if ( err ) goto exit;
		}

	exit:
		return;
	}

	/*
	 *  remove entries with indices lower than the specified index
	 */
	void
	prune_front(index_type index, std::error_code& err)
	{
		clear_error( err );
		assert( ! m_log.empty() );
		if ( index < m_log.front()->index() || index > m_log.back()->index() )
		{
			err = make_error_code( std::errc::invalid_argument );
		}
		else
		{
			try
			{
				if ( index > m_log.front()->index() )
				{
					while( ! m_log.empty() && m_log.front()->index() < index )
					{
						m_log.pop_front();
					}

					if ( m_log.empty() || m_log.front()->index() != index )
					{
						throw std::system_error{ make_error_code( std::errc::state_not_recoverable ) };
					}

					m_os.close();
					m_os.open( m_log_temp_pathname, bstream::open_mode::truncate );

					for ( auto it = m_log.begin(); it != m_log.end(); ++it )
					{
						auto pos = m_os.position();
						(*it)->file_position( pos );
						write_frame( *it, false );
					}

					write_frame( m_state );
					m_os.close();

					filesystem::rename( filesystem::path{ m_log_temp_pathname }, filesystem::path{ m_log_pathname } );

					m_os.open( m_log_pathname, bstream::open_mode::append );
				}
			}
			catch ( std::system_error const& e )
			{
				err = e.code();
			}
		}

		if ( err && m_os.is_open() )
		{
			std::error_code discard;
			m_os.close( discard );
		}
	}

	std::size_t
	size() const
	{
		return m_log.size();
	}
	
protected:

	bool 
	index_check(index_type index) const noexcept
	{
		return !m_log.empty() && index >= front()->index() && index <= back()->index();
	}
	
	bool
	integrity_check() const noexcept
	{
		return m_log.empty() || (m_log.size() == ((m_log.back()->index() + 1) - m_log.front()->index()));
	}

	replicant_id_type							m_self;
	replicant_state::ptr						m_state;
	std::string									m_log_pathname;
	std::string									m_log_temp_pathname;
	bstream::ofbstream							m_os;
	std::deque< entry::ptr >					m_log;
};

} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAFT_LOG_H
