#ifndef NODEOZE_RAFT_LOG_FRAMES_H
#define NODEOZE_RAFT_LOG_FRAMES_H

#include <memory>
#include <cstdint>
#include <system_error>
#include <unistd.h>
#include <cerrno>
#include <nodeoze/bstream.h>
#include <nodeoze/bstream/ombstream.h>
#include <nodeoze/bstream/imbstream.h>
#include <nodeoze/bstream/ofbstream.h>
#include <nodeoze/bstream/ifbstream.h>
#include <nodeoze/buffer.h>
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

enum class frame_type
{
	invalid,
	replicant_state_frame,
	state_machine_update_frame
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

    using ptr = std::shared_ptr< frame >;

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
	BSTRM_POLY_SERIALIZE( frame, , ( m_pos ) )

	// virtual bstream::obstream&
	// serialize( nodeoze::bstream::obstream& os ) const
	// {
	// 	base_type::_serialize( os );
	// 	os << m_pos;
	// 	return os;
	// }
	
	virtual frame_type
	get_type() const noexcept = 0;

	template< class T >
	typename std::enable_if_t< std::is_base_of< frame, T >::value, T& >
	as()
	{
		if ( T::type() != get_type() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_frame_type_error ) };
		}
		return reinterpret_cast< T& >( *this );
	}

	template< class T >
	typename std::enable_if_t< std::is_base_of< frame, T >::value, const T& >
	as() const
	{
		if ( T::type() != get_type() )
		{
			throw std::system_error{ make_error_code( raft::errc::log_frame_type_error ) };
		}
		return reinterpret_cast< const T& >( *this );
	}

	file_position_type
	file_position() const noexcept
	{
		return m_pos;
	}

	void
	file_position( file_position_type pos )
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
	BSTRM_POLY_SERIALIZE( replicant_state, ( frame ) , ( m_self, m_term, m_vote ) )

    using ptr = std::shared_ptr< replicant_state >;

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

	replicant_state()
	:
	m_dirty{ false },
	m_self{ 0 },
	m_term{ 0 },
	m_vote{ 0 }
	{}

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

	// virtual bstream::obstream&
	// serialize( nodeoze::bstream::obstream& os ) const override
	// {
	// 	base_type::_serialize( os );
	// 	frame::serialize( os );
	// 	os << m_self << m_term << m_vote;
	// 	return os;
	// }

	void
	update( term_type t, replicant_id_type v )
	{
		term( t );
		vote( v );
	}

	void
	update( replicant_state const& rhs )
	{
		if ( rhs.m_self != m_self )
		{
			throw std::system_error{ make_error_code( raft::errc::log_server_id_error ) };
		}
		term( rhs.term() );
		vote( rhs.vote() );
	}

	void
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

	void
	clean()
	{
		m_dirty = false;
	}

	void
	clear( replicant_id_type self )
	{
		m_self = self;
		m_dirty = false;
		m_term = 1;
		m_vote = 0;
	}

	replicant_id_type
	self() const noexcept
	{
		return m_self;
	}

	term_type
	term() const noexcept
	{
		return m_term;
	}

	replicant_id_type
	vote() const noexcept
	{
		return m_vote;
	}

	void
	term( term_type trm )
	{
		assert( trm >= m_term );
		if ( trm != m_term )
		{
			m_dirty = true;
			m_term = trm;
		}
	}

	void
	vote( replicant_id_type id )
	{
		if ( id != m_vote )
		{
			m_dirty = true;
			m_vote = id;
		}
	}

	bool
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

	using ptr = std::shared_ptr< entry >;
	
	entry( term_type term, index_type index )
	:
	frame{},
	m_term{ term },
	m_index{ index }
	{}

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
	BSTRM_POLY_SERIALIZE( entry, ( frame ), ( m_term, m_index ) )

	// virtual bstream::obstream&
	// serialize( nodeoze::bstream::obstream& os ) const
	// {
	// 	base_type::_serialize( os );
	// 	frame::serialize( os );
	// 	os << m_term << m_index;
	// 	return os;
	// }

	index_type
	index() const noexcept
	{
		return m_index;
	}
	
	term_type
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
	BSTRM_CTOR( state_machine_update, ( entry ), ( m_payload ) )
	BSTRM_POLY_SERIALIZE( state_machine_update, ( entry ), ( m_payload ) )

    using ptr = std::shared_ptr< state_machine_update >;


	// state_machine_update( nodeoze::bstream::ibstream& is )
	// :
	// base_type{ is },
	// entry{ is },
	// m_payload{ nodeoze::bstream::ibstream_initializer< decltype( m_payload ) >::get( is ) }
	// {}

	// virtual bstream::obstream&
	// serialize( nodeoze::bstream::obstream& os ) const override
	// {
	// 	base_type::_serialize( os );
	// 	entry::serialize( os );
	// 	os << m_payload;
	// 	return os;
	// }

	state_machine_update( term_type term, index_type index, buffer&& payload )
	:
	entry{ term, index },
	m_payload{ std::move( payload ) }
	{}

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

	buffer
	payload() const
	{
		return m_payload;
	}

private:
	buffer	m_payload;
};


inline bstream::context_base const& get_log_context()
{
//    static const bstream::context< frame, replicant_state, entry, state_machine_update > log_context{ { &raft_category(), } };
    static const bstream::context< frame, replicant_state, entry, state_machine_update > log_context{ &raft_category() };
    return log_context;
}


} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAFT_LOG_FRAMES_H
