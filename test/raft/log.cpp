#include <nodeoze/raft/log.h>
#include <nodeoze/test.h>
#include <experimental/type_traits>
#include <thread>
#include <chrono>
#include <iostream>

using namespace nodeoze;
using namespace raft;

TEST_CASE( "nodeoze/smoke/raft/basic" )
{
	{
//		std::this_thread::sleep_for ( std::chrono::seconds( 10 ) );
		std::error_code ec;
		raft::log oak( 1, "logfile.log", "logfile.tmp" );
		oak.initialize( 1, 1, 0, ec );
		CHECK( ! ec );
		oak.update_replicant_state( raft::replicant_state{ 1, 2, 3 }, ec );
		CHECK( ! ec );
		oak.close( ec );
		CHECK( ! ec );
	}
	{
		std::error_code ec;
		raft::log oak( 1, "logfile.log", "logfile.tmp" );
		oak.restart( 1, ec );
		CHECK( ! ec );
		CHECK( oak.current_replicant_state().term() == 2 );
		CHECK( oak.current_replicant_state().vote() == 3 );
		oak.close( ec );
		CHECK( ! ec );
	}
}

TEST_CASE( "nodeoze/smoke/raft/payload" )
{
//	std::this_thread::sleep_for ( std::chrono::seconds( 10 ) );
	{
		std::error_code ec;
		raft::log oak( 1, "logfile.log", "logfile.tmp" );
		oak.initialize( 1, 1, 0, ec );
		CHECK( ! ec );
		oak.update_replicant_state( raft::replicant_state{ 1, 1, 0 }, ec );
		CHECK( ! ec );

		auto p1 = std::make_shared< raft::state_machine_update >( 1, 1, buffer{ "some update payload" } );
		oak.append( p1, ec );
		CHECK( ! ec );

		auto p2 = std::make_shared< raft::state_machine_update >( 1, 2, buffer{ "some more update payload" } );
		oak.append( p2, ec );
		CHECK( ! ec );
		
		auto p3 = std::make_shared< raft::state_machine_update >( 1, 3, buffer{ "even more update payload" } );
		oak.append( p3, ec );
		CHECK( ! ec );

		oak.close( ec );
		CHECK( ! ec );
	}
	{
		std::error_code ec;
		raft::log oak( 1, "logfile.log", "logfile.tmp" );
		oak.restart( 1, ec );
		if ( ec ) { std::cout << "error code is " << ec.message() << std::endl; }
		CHECK( ! ec );
		CHECK( oak.current_replicant_state().term() == 1 );
		CHECK( oak.current_replicant_state().vote() == 0 );
		CHECK( oak.size() == 3 );
		CHECK( oak.back()->get_type() == raft::frame_type::state_machine_update_frame );
		auto smup = std::dynamic_pointer_cast< state_machine_update >( oak.back() );
		auto index = smup->index();
		CHECK( index == 3 );
		buffer load = smup->payload();
		CHECK( load.to_string() == "even more update payload" );
//		std::cout << "back index is " << index << ", payload:" << std::endl;
//		load.dump( std::cout ); std::cout.flush();
		oak.close( ec );
		CHECK( ! ec );
	}

	{
		std::error_code ec;
		raft::log oak( 1, "logfile.log", "logfile.tmp" );
		oak.restart( 1, ec );
		CHECK( ! ec );

		oak.prune_front( 2, ec );
		CHECK( ! ec );

		auto fidx = oak.front()->index();
		CHECK( fidx == 2 );

		CHECK( oak.size() == 2 );

		oak.prune_back( 2, ec );
		CHECK( ! ec );

		CHECK( oak.back()->index() == 2 );
		CHECK( oak.size() == 1 );

		oak.close( ec );
		CHECK( ! ec );
	}
	
}

// TEST_CASE( "nodeoze/smoke/raft/basic" )

