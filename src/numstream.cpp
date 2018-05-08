#include <nodeoze/bstream/numstream.h>
#include <nodeoze/dump.h>

using namespace nodeoze;

void 
bstream::inumstream::dump( std::ostream& os, position_type pos, size_type length ) 
{
	if ( m_is_buffer )
	{
		buffer bslice = get_buffer().slice( pos, length );
		dumpster{}.dump( os, bslice.const_data(), bslice.size() );
	}
	else
	{
		os << "dump not implemented for non-buffer streambufs" << std::endl;
	}
}

std::string 
bstream::inumstream::strdump( position_type pos, size_type nbytes ) 
{
	std::ostringstream oss;
	dump( oss, pos, nbytes );
	return oss.str(); 
}

/*
void 
bstream::inumstream::dump_state( std::ostream& os ) 
{
	m_bufptr.dump_state( os );
	os << "inumstream - m_data: " << (void*) m_data << ", m_size: " << m_size << ", m_pos: " << m_pos << std::endl;
	os.flush();
}
*/

void 
bstream::onumstream::dump( std::ostream& os, position_type pos, size_type length ) 
{
	if ( m_is_buffer )
	{
		buffer bslice = get_buffer().slice( pos, length );
		dumpster{}.dump( os, bslice.const_data(), bslice.size() );
	}
	else
	{
		os << "dump not implemented for non-buffer streambufs" << std::endl;
	}
}

std::string 
bstream::onumstream::strdump( position_type pos, size_type length ) 
{
	std::ostringstream oss;
	dump( oss, pos, length );
	return oss.str(); 
}
/*
void 
bstream::onumstream::dump_state( std::ostream& os ) 
{
	
	m_bufptr.dump_state( os );
	os << "inumstream - m_data: " << (void*) m_data << ", m_size: " << m_size << ", m_pos: " << m_pos << std::endl;
	os.flush();
}
*/
