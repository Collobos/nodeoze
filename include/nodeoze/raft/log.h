#ifndef NODEOZE_RAFT_LOG_H
#define NODEOZE_RAFT_LOG_H

#include <memory>
#include <cstdint>
#include <system_error>
#include <unistd.h>
#include <cerrno>
#include <nodeoze/fs2.h>
#include <nodeoze/bstream/ifbstream.h>
#include <nodeoze/bstream/ofbstream.h>
#include <nodeoze/bstream/imbstream.h>
#include <nodeoze/bstream/ombstream.h>
#include <nodeoze/bstream/macros.h>
#include <nodeoze/raft/types.h>
#include <nodeoze/raft/state_machine.h>
#include <nodeoze/raft/error.h>

#ifndef NODEOZE_RAFT_LOG_FRAME_SIZE_HINT
#define NODEOZE_RAFT_LOG_FRAME_SIZE_HINT  4096ul
#endif // NODEOZE_RAFT_LOG_FRAME_SIZE_HINT

namespace nodeoze
{
namespace raft
{

inline void
clear_error(std::error_code& err)
{
	static const auto ok = make_error_code( raft::errc::ok );
	err = ok;
}

using file_position_type = std::int64_t;

enum class frame_type
{
	invalid,
	replicant_state_frame,
	state_machine_update_frame,
};

	/*
	*	Requirements for log record types:
	*		ibstream constructible
	*		default constructible (only for return on error conditions, need not be valid )
	*		implement virtual serialize()
	*		copy/move constructible
	*		must support static type() function and get_type()
	*/


class frame : BSTRM_BASE( frame )
{
public:

	inline
	frame()
	:
	m_pos{ -1 }
	{}

	frame( frame const& rhs ) = default;
	frame( frame&& rhs ) = default;

	virtual ~frame() {}

	BSTRM_FRIEND_BASE( frame )
	BSTRM_CTOR( frame, , ( m_pos ) )
	BSTRM_ITEM_COUNT( , ( m_pos ) )

	virtual bstream::obstream&
	serialize(nodeoze::bstream::obstream& os) const
	{
		base_type::_serialize( os );
		os << m_pos;
		return os;
	}
	
	virtual frame_type
	get_type() const noexcept = 0;

	template< class T >
	inline typename std::enable_if_t< std::is_base_of< frame, T >::value, T& >
	as()
	{
		if ( T::type() != get_type() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_frame_type_error ) };
		}
		return reinterpret_cast< T& >( *this );
	}

	template< class T >
	inline typename std::enable_if_t< std::is_base_of< frame, T >::value, const T& >
	as() const
	{
		if ( T::type() != get_type() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_frame_type_error ) };
		}
		return reinterpret_cast< const T& >( *this );
	}

	inline file_position_type
	file_position() const noexcept
	{
		return m_pos;
	}

	inline void
	file_position(file_position_type pos)
	{
		m_pos = pos;
	}

protected:

	frame_type 				m_type;
	file_position_type 		m_pos;
};

class replicant_state : BSTRM_BASE( replicant_state ), public frame
{
public:
	BSTRM_FRIEND_BASE( replicant_state )
	BSTRM_CTOR( replicant_state, ( frame ) , ( m_self, m_term, m_vote ) )
	BSTRM_ITEM_COUNT( ( frame ) , ( m_self, m_term, m_vote ) )

	static constexpr frame_type
	type()
	{ 
		return frame_type::replicant_state_frame;
	}

	virtual enum frame_type
	get_type() const noexcept override
	{
		return type();
	}

	inline
	replicant_state()
	:
	m_dirty{ false },
	m_self{ 0 },
	m_term{ 0 },
	m_vote{ 0 }
	{}

	inline
	replicant_state( replicant_id_type self, term_type term = 1, replicant_id_type vote = 0 )
	:
	m_dirty{ false },
	m_self{ self },
	m_term{ term },
	m_vote{ vote }
	{}

	replicant_state( replicant_state const& rhs ) = default;

	replicant_state( replicant_state&& rhs ) = default;

	replicant_state&
	operator=( replicant_state const& ) = delete;

	replicant_state&
	operator=( replicant_state&& ) = delete;

	virtual bstream::obstream&
	serialize(nodeoze::bstream::obstream& os) const override
	{
		base_type::_serialize( os );
		frame::serialize( os );
		os << m_self << m_term << m_vote;
		return os;
	}

	inline void
	update( term_type t, replicant_id_type v )
	{
		term( t );
		vote( v );
	}

	inline void
	update( replicant_state const& rhs )
	{
		if ( rhs.m_self != m_self )
		{
			throw std::system_error{ make_error_code( raft::errc::log_server_id_error ) };
		}
		term( rhs.term() );
		vote( rhs.vote() );
	}

	inline void
	update( replicant_state const& rhs, std::error_code& err )
	{
		clear_error( err );
		if ( rhs.m_self != m_self )
		{
			err = make_error_code( raft::errc::log_server_id_error );
		}
		else
		{
			term( rhs.term() );
			vote( rhs.vote() );
		}
	}

	inline void
	clean()
	{
		m_dirty = false;
	}

	inline void
	clear( replicant_id_type self )
	{
		m_self = self;
		m_dirty = false;
		m_term = 1;
		m_vote = 0;
	}

	inline replicant_id_type
	self() const noexcept
	{
		return m_self;
	}

	inline term_type
	term() const noexcept
	{
		return m_term;
	}

	inline replicant_id_type
	vote() const noexcept
	{
		return m_vote;
	}

	inline void
	term( term_type trm )
	{
		assert( trm >= m_term );
		if ( trm != m_term )
		{
			m_dirty = true;
			m_term = trm;
		}
	}

	inline void
	vote( replicant_id_type id )
	{
		if ( id != m_vote )
		{
			m_dirty = true;
			m_vote = id;
		}
	}

	inline bool
	is_dirty() const noexcept
	{
		return m_dirty;
	}

private:
	bool						m_dirty = false;
	replicant_id_type			m_self;
	term_type					m_term;
	replicant_id_type			m_vote;
};

class entry : BSTRM_BASE( entry ), public frame
{
public:

	using ptr = std::unique_ptr< entry >;
	
	entry( term_type term, index_type index )
	:
	frame{},
	m_term{ term },
	m_index{ index }
	{}

	inline
	entry()
	:
	frame{},
	m_term{ 0 },
	m_index{ 0 }
	{}

	entry( entry const& rhs ) = default;
	entry( entry&& rhs ) = default;

	BSTRM_FRIEND_BASE( entry )
	BSTRM_CTOR( entry, ( frame ), ( m_term, m_index ) )
	BSTRM_ITEM_COUNT( ( frame ) , ( m_term, m_index ) )

	virtual bstream::obstream&
	serialize(nodeoze::bstream::obstream& os) const
	{
		base_type::_serialize( os );
		frame::serialize( os );
		os << m_term << m_index;
		return os;
	}

	inline index_type
	index() const noexcept
	{
		return m_index;
	}
	
	inline term_type
	term() const noexcept
	{
		return m_term;
	}

protected:

	term_type					m_term;
	index_type					m_index;
};

class state_machine_update : BSTRM_BASE( state_machine_update ), public entry
{
public:
	BSTRM_FRIEND_BASE( state_machine_update )
	BSTRM_ITEM_COUNT( ( entry ), ( m_payload ) )
//	BSTRM_CTOR( state_machine_update, ( entry ), ( m_payload ) )

	state_machine_update( nodeoze::bstream::ibstream& is )
	:
	base_type{ is },
	entry{ is },
	m_payload{ nodeoze::bstream::ibstream_initializer<decltype(m_payload)>::get(is) }
	{}

	virtual bstream::obstream&
	serialize(nodeoze::bstream::obstream& os) const override
	{
		base_type::_serialize( os );
		entry::serialize( os );
		os << m_payload;
		return os;
	}

	inline
	state_machine_update( term_type term, index_type index, buffer&& payload )
	:
	entry{ term, index },
	m_payload{ std::move( payload ) }
	{}

	inline
	state_machine_update()
	:
	entry{},
	m_payload{}
	{}

	static constexpr frame_type
	type()
	{ 
		return frame_type::state_machine_update_frame;
	}

	virtual frame_type
	get_type() const noexcept override
	{
		return type();
	}

	inline buffer
	payload() const
	{
		return m_payload;
	}

private:
	buffer	m_payload;
};

class log
{
public:

	log( replicant_id_type id, std::string const& log_pathname, std::string const& log_temp_pathname )
	:
	m_self{ id },
	m_state{ id },
	m_log_pathname{ log_pathname },
	m_log_temp_pathname{ log_temp_pathname },
	m_frame_writer{ NODEOZE_RAFT_LOG_FRAME_SIZE_HINT }
	{}


	void
	restart( replicant_id_type self, std::error_code& err )
	{
		clear_error( err );
		m_self = self;
		m_state.clear( self );
		m_log.clear();

		path tmp_path{ m_log_temp_pathname };
		if (fs::shared().exists( path{ tmp_path } ) )
		{
			err = fs::shared().unlink( tmp_path );
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
		m_state.update( current_term, voted_for );
		m_log.clear();

		path tmp_path{ m_log_temp_pathname };
		if (fs::shared().exists( path{ tmp_path } ) )
		{
			err = fs::shared().unlink( tmp_path );
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
	append( std::unique_ptr< entry > entry_ptr, std::error_code& err )
	{
		m_log.push_back( std::move( entry_ptr ) );

		write_frame( *m_log.back(), err );
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
		m_state.update( new_state );
		if ( m_state.is_dirty() )
		{
			write_frame( m_state, err );
			if ( ! err )
			{
				m_state.clean();
			}
		}
	}

	inline replicant_state const&
	current_replicant_state() const
	{
		return m_state;
	}

	void
	write_frame( frame& f, bool flush = true )
	{
		f.file_position( m_os.position() );
		assert( m_frame_writer.get_membuf().get_buffer_ref().is_unique() ); 
		m_frame_writer.clear();
		assert( m_frame_writer.get_membuf().get_buffer_ref().is_unique() ); 
		m_frame_writer << f;
		auto frame_buffer = m_frame_writer.get_buffer();
		m_os.put_num( static_cast< std::uint32_t >( frame_buffer.size() ) );
		m_os.put_num( static_cast< std::uint32_t >( f.get_type() ) );
		m_os.write_blob( frame_buffer );
		m_os.put_num( frame_buffer.checksum() );
		if ( flush )
		{
			m_os.flush();
		}
	}

	inline void
	write_frame( frame& f, std::error_code& err, bool flush = true )
	{
		clear_error( err );
		try
		{
			write_frame( f, flush );
		}
		catch ( std::system_error const& e )
		{
			err = e.code();
		}
	}
	
	std::pair< std::uint32_t, frame_type >
	read_frame_prefix( bstream::ifbstream& is )
	{
		auto frame_size = is.get_num< std::uint32_t >();
		auto type = static_cast< frame_type >( is.get_num< std::uint32_t >() );
		return std::make_pair( frame_size, type );
	}

	template< class T >
	T
	read_frame( bstream::ifbstream& is )
	{
		buffer framebuf = is.read_blob();
		auto buffer_checksum = framebuf.checksum();
		auto frame_checksum = is.get_num< buffer::checksum_type >();
		if ( buffer_checksum != frame_checksum )
		{
			throw std::system_error{ make_error_code( raft::errc::log_checksum_error ) };
		}
		assert( framebuf.is_unique() );
		bstream::imbstream framebuf_strm{ std::move( framebuf ) };
		assert( framebuf_strm.get_membuf().get_buffer_ref().is_unique() );
		return T{ framebuf_strm };
	}

	template< class T >
	std::unique_ptr< T >
	read_frame_as_unique_ptr( bstream::ifbstream& is )
	{
		buffer framebuf = is.read_blob();
		auto buffer_checksum = framebuf.checksum();
		auto frame_checksum = is.get_num< buffer::checksum_type >();
		if ( buffer_checksum != frame_checksum )
		{
			throw std::system_error{ make_error_code( raft::errc::log_checksum_error ) };
		}
		assert( framebuf.is_unique() );
		bstream::imbstream framebuf_strm{ std::move( framebuf ) };
		assert( framebuf_strm.get_membuf().get_buffer_ref().is_unique() );
		return std::make_unique< T >( framebuf_strm );
	}

	void
	truncate( file_position_type pos, std::error_code& err )
	{
		m_os.close( err );
		if ( err ) goto exit;

		{
			auto result = ::truncate( m_log_pathname.c_str(), static_cast< off_t >( pos ) );
			if ( result < 0 )
			{
				err = std::error_code{ errno, std::generic_category() };
				goto exit;
			}
		}
		m_os.open( m_log_pathname, bstream::open_mode::append, err );

	exit:
		return;
	}

	void
	recover( std::error_code& err )
	{
		if ( !fs::shared().exists( path( m_log_pathname ) ) )
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
				m_state.clear( m_self );

				file_position_type frame_position = is.position();
				while ( static_cast< std::size_t >( frame_position ) < file_size )
				{
					auto header = read_frame_prefix( is );
					switch ( header.second )
					{
						case frame_type::replicant_state_frame:
						{
							auto repl_state = read_frame< replicant_state >( is );
							assert( repl_state.file_position() == frame_position );
							m_state.update( repl_state );
						}
						break;

						case frame_type::state_machine_update_frame:
						{
							m_log.push_back( read_frame_as_unique_ptr< state_machine_update >( is ) );
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

				if ( ! m_state.is_dirty() )
				{
					throw std::system_error{ make_error_code( raft::errc::log_recovery_error ) };
				}
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

	inline const entry&
	back() const
	{
		if ( m_log.empty() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_index_out_of_range ) };
		}
		return *m_log.back();
	}

	inline const entry&
	front() const
	{
		if ( m_log.empty() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_index_out_of_range ) };
		}
		return *m_log.front();
	}

	inline const entry&
	operator[](index_type index) const
	{
		if (index_check(index))
		{
			throw std::system_error{ make_error_code( raft::errc::log_index_out_of_range ) };
		}
		auto i = index - front().index();
		return *m_log[i];
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
						write_frame( *( *it ), false );
					}

					write_frame( m_state );
					m_os.close();

					auto ecode = fs::shared().move( path{ m_log_temp_pathname }, path{ m_log_pathname } );
					if ( ecode )
					{
						throw std::system_error{ ecode };
					}

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
		return !m_log.empty() && index >= front().index() && index <= back().index();
	}
	
	bool
	integrity_check() const noexcept
	{
		return m_log.empty() || (m_log.size() == ((m_log.back()->index() + 1) - m_log.front()->index()));
	}

	replicant_id_type							m_self;
	replicant_state								m_state;
	std::string									m_log_pathname;
	std::string									m_log_temp_pathname;
	bstream::ofbstream							m_os;
	std::deque< std::unique_ptr< entry > > 		m_log;
	bstream::ombstream							m_frame_writer;
};

} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAF_LOG_H
