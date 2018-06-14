#ifndef NODEOZE_RAFT_REPLICANT_H
#define NODEOZE_RAFT_REPLICANT_H

#include <nodeoze/raft/log.h>
#include <set>

namespace nodeoze 
{
namespace raft 
{

class replicant
{
public:

    using ptr = std::shared_ptr< replicant >;

    using log_entries = std::vector< entry::ptr >;

    enum class role 
    {
        follower,
        candidate,
        leader
    };

    enum class message_type
    {
        request_vote,
        request_vote_reply,
        append_entries,
        append_entries_reply
    };

    /*
     * State transitions
     */

    void
    initialize( replicant_id_type self, std::set< replicant_id_type > const& active_replicants, std::error_code& err )
    {
        m_self = self;

        m_active_replicants = active_replicants;

        for ( auto id : m_active_replicants )
        {
            m_next_index[ id ] = 1;
            m_match_index[ id ] = 0;
        }

        m_role = role::follower;
        m_current_term = 1;
        m_voted_for = 0;

        m_votes_responded.clear();
        m_votes_granted.clear();

        m_log.initialize( self, m_current_term, m_voted_for, err );
        if ( err ) goto exit;

        m_commit_index = 0;

    exit:
        return;
    }

    void
    restart( replicant_id_type self, std::set< replicant_id_type > const& active_replicants, std::error_code& err )
    {
        m_self = self;
        m_active_replicants = active_replicants;

        for ( auto id : m_active_replicants )
        {
            m_next_index[ id ] = 1;
            m_match_index[ id ] = 0;
        }

        m_role = role::follower;

        m_votes_responded.clear();
        m_votes_granted.clear();

        m_log.restart( self, err );
        if ( err ) goto exit;

        m_current_term = m_log.current_replicant_state().term();
        m_voted_for = m_log.current_replicant_state().vote();

        m_commit_index = 0;
    }

    void
    on_election_timeout()
    {
        if ( m_role != role::leader )
        {
            ++m_current_term;
            m_role = role::candidate;
            m_voted_for = m_self;
            m_votes_granted.clear();
            m_votes_responded.clear();
            m_votes_granted.insert( m_self );
            m_votes_responded.insert( m_self );
            m_log.update_replicant_state( m_self, m_current_term, m_voted_for );
            start_election();
        }
    }

    void
    start_election()
    {
        start_election_timer();
        schedule_request_vote_messages();
    }

    void
    schedule_request_vote_messages()
    {
        for ( replicant_id : m_active_replicants )
        {
            if ( replicant_id != m_self_id )
            {
                
            }
        }
    }

    void
    append_entries()
    {

    }

    void
    become_leader()
    {
        m_role = role::leader;

        for ( auto id : m_active_replicants )
        {
            m_next_index[ id ] = m_log.back()->index() + 1;
            m_match_index[ id ] = 0;
        }
        start_heartbeat();
    }

    void
    advance_commit_index()
    {

    }

    void
    client_request()
    {

    }

    static void 
    send_request_vote_messages( replicant_id_type self_id )
    {
        auto self = get_replicant( self_id );
        
        if ( self && self->election_in_progress( current_term ) )
        {
            auto current_term = self->m_current_term;
            auto last_log_index = self->m_log.back()->index();
            auto last_log_term = self->m_log.back()->term();

            for ( replicant : self->m_active_replicants && replicant != self_id && m_votes_responded.count( replicant ) == 0 )
            {
                send_vote_request_message( self_id, replicant, current_term, last_log_index, last_log_term );
            }
        }
    }

    static void
    receive_request_vote_message( replicant_id_type sender_id, 
                                  replicant_id_type self_id, 
                                  term_type term, 
                                  index_type last_log_index, 
                                  term_type last_log_term )
    {
        auto self = get_replicant( self_id );
        if ( self )
        {
            self->update_term( term );
            self->handle_request_vote_message( sender_id, term, last_log_index, last_log_term );
        }
    }

    static void
    receive_request_vote_reply( replicant_id_type sender_id, replicant_id_type self_id, term_type term, bool granted )
    {
        auto self = get_replicant( self_id );
        if ( self )
        {
            self->update_term( term );
            if ( term >= self->m_current_term )
            {
                self->handle_request_vote_reply( sender_id, term, granted );
            }
        }
    }

    static void 
    receive_append_entries_message( replicant_id_type sender_id, 
                                    replicant_id_type self_id, 
                                    term_type term, 
                                    index_type prev_log_index, 
                                    term_type prev_log_term, 
                                    index_type commit_index, 
                                    log_entries& entries )
    {
        auto self = get_replicant( self_id );
        if ( self )
        {
            self->update_term( term );
            self->handle_append_entries_message( sender_id, term, prev_log_index, prev_log_term, commit_index, entries)
        }
    }

    static void
    receive_append_entries_reply( replicant_id_type sender_id, 
                                  replicant_id_type self_id,
                                  term_type term,
                                  index_type prev_log_index,
                                  term_type prev_log_term,
                                  index_type commit_index,
                                  log_entries& entries )
    {
        auto self = get_replicant( term );
        if ( self )
        {
            self->update_term( term );
            if ( term >= self->m_current_term )
            {
                self->handle_append_entries_reply( sender_id, term, term, prev_log_index, prev_log_term, commit_index, entries );
            }
        }
    }

	void handle_request_vote_message(replicant_id_type candidate_id, term_type candidate_term, term_type last_term, index_type last_index)
	{
		bool log_ok = last_term > m_log.back()->term() ||
			( last_term == m_log.back()->term() && last_index >= m_log.back()->index() );
		bool grant = candidate_term == m_current_term && log_ok &&
			( m_voted_for == 0 || m_voted_for == candidate_id );
		if (grant)
		{
			m_voted_for = candidate_id;
		}
		send_request_vote_reply( candidate_id, m_current_term, grant );
	}
	
    void
    handle_request_vote_reply( replicant_id_type voter_id, term_type voter_term, bool granted )
    {
        if ( election_in_prgress( voter_term ) )
        {
            m_votes_responded.insert( voter_id );
            if ( granted )
            {
                m_votes_granted.insert( voter_id );
            }
            if ( quorum() )
            {
                become_leader();
            }
        }
    }

	void update_term(std::size_t incoming_term)
	{
		if (incoming_term > m_current_term)
		{
			m_current_term = incoming_term;
			m_role = role::follower;
			m_voted_for = 0;
		}
	}

    bool
    quorum()
    {
        return m_votes_granted.size() * 2 > m_active_replicants.size();
    }

    bool
    election_in_progress( term_type term )
    {
        return m_role == role::candidate && term == m_current_term;
    }

	inline std::chrono::milliseconds
	randomize_timeout(std::chrono::milliseconds timer_base)
	{
		static std::mt19937_64 mt{entropy()};
		std::uniform_int_distribution<std::uint64_t> dist(0, timer_base.count());
		return timer_base + std::chrono::milliseconds{dist(mt)};
	}

    static replicant::ptr
    install_replicant( replicant_id_type id, replicant::ptr rep )
    {
        replicant::ptr result = nullptr;

        auto emplaced = m_replicant_map.emplace( id, rep );
        if ( ! emplaced.second )
        {
            result emplaced.first->second;
        }

        return result;
    }

    static replicant::ptr
    uninstall_replicant( replicant_id_type id )
    {
        replicant::ptr result = nullptr;
        auto found = m_replicant_map.find( id );
        if ( found != m_replicant_map.end() )
        {
            result = found->second;
            m_replicant_map.erase( found );
        }
        return result;
    }

    static replicant::ptr
    get_replicant( replicant_id_type id )
    {
        replicant::ptr result = nullptr;
        auto found = m_replicant_map.find( id );
        if ( found != m_replicant_map.end() )
        {
            result = found->second;
        }
        return result;
    }

    using index_map = std::map< replicant_id_type, index_type >;

    replicant_id_type                   m_self_id;
    term_type                           m_current_term;
    role                                m_role;
    replicant_id_type                   m_voted_for;
    log                                 m_log;
    index_type                          m_commit_index;

    // candidate member variables

    std::set< replicant_id_type >       m_votes_responded;
    std::set< replicant_id_type >       m_votes_granted;

    // leader member variables

    index_map                           m_next_index;
    index_map                           m_match_index;

    std::set< replicant_id_type >       m_active_replicants;

    std::chrono::milliseconds           m_election_timeout;
    std::chrono::milliseconds           m_election_timeout_random;
    std::chrono::milliseconds           m_heartbeat_timeout;

    static std::map< replicant_id_type, replicant::ptr > m_replicant_map;

};

} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAFT_REPLICANT_H