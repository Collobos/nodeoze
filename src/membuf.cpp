#include <nodeoze/membuf.h>
#include <nodeoze/ntest.h>
#include <vector>

using namespace nodeoze;
//namespace io = boost::iostreams;


constexpr std::streamsize omembuf::min_alloc_size;

TEST_CASE( "nodeoze/smoke/buffer_streambuf/basic" )
{

	omembuf s{ 8 };
	s.sputc( '0' );
	s.sputc( '1' );
	s.sputc( '2' );
	s.sputc( '3' );
	s.sputc( '4' );
	s.sputc( '5' );

	std::vector< char > stuff = { '6', '7', '8', '9', 'A', 'B', 'C' };

	s.sputn( stuff.data(), stuff.size() );

	s.pubseekpos( 24, std::ios_base::out );

	s.sputc( '?' );

//	s.get_buffer().dump( std::cout );

	imembuf isb{ s.get_buffer() };

	auto ch = isb.sbumpc();
	CHECK( ch == '0' );
	ch = isb.sbumpc();
	CHECK( ch == '1' );
	ch = isb.sbumpc();
	CHECK( ch == '2' );
	ch = isb.sbumpc();
	CHECK( ch == '3' );
	ch = isb.sbumpc();
	CHECK( ch == '4' );
	ch = isb.sbumpc();
	CHECK( ch == '5' );
	ch = isb.sbumpc();
	CHECK( ch == '6' );
	ch = isb.sbumpc();
	CHECK( ch == '7' );
	ch = isb.sbumpc();
	CHECK( ch == '8' );
	ch = isb.sbumpc();
	CHECK( ch == '9' );
	ch = isb.sbumpc();
	CHECK( ch == 'A' );
	ch = isb.sbumpc();
	CHECK( ch == 'B' );
	ch = isb.sbumpc();
	CHECK( ch == 'C' );

	buffer tmp{ 11 };

	auto n = isb.sgetn( reinterpret_cast< char* >( tmp.mutable_data() ), 11 );
	CHECK( n == 11 );
	for ( auto i = 0; i < 11; ++i )
	{
		CHECK( tmp[ i ] == 0 );
	}

	CHECK( isb.in_avail() == 1 );

	ch = isb.sbumpc();
	CHECK( ch == '?' );

	CHECK( isb.in_avail() == 0 );

	ch = isb.sbumpc();

	CHECK( ch == imembuf::traits_type::eof() );

	auto sgetc_result = s.sgetc();

	CHECK( sgetc_result == -1 );

//	buffer empty_buf;

//	omembuf esb{ std::move( empty_buf ) };

//	auto result = esb.sputc( 'X' );

//	esb.print_state();

//	buffer buf = esb.get_buffer();

//	buf.debug_state();

/*
	s.sputc( '6' );
	s.sputc( '7' );
	s.sputc( '8' );
	s.sputc( '9' );
	s.sputc( 'A' );
	s.sputc( 'B' );
	s.sputc( 'C' );
*/

}

