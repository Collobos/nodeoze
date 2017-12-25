#include <nodeoze/nzip.h>
#include <nodeoze/nmacros.h>
#include "miniz.c"
#include <string>
#include <unordered_map>

using namespace nodeoze;

class miniz_zip : public zip
{
public:

	miniz_zip( const std::uint8_t *data, std::size_t len );

	virtual ~miniz_zip();

	virtual bool
	lookup( const std::string &name, std::uint32_t &index );

	virtual bool
	extract( std::uint32_t index, extract_f func );
	
private:

	static size_t
	callback( void *pOpaque, std::uint64_t ofs, const void *pBuf, size_t n )
	{
		nunused( ofs );
	
		miniz_zip *self = static_cast< miniz_zip* >( pOpaque );
		self->m_func( static_cast< const std::uint8_t* >( pBuf ), n );
		return n;
	}

	mz_zip_archive										m_archive;
	extract_f											m_func;
	std::unordered_map< std::string, std::uint32_t >	m_map;
};


zip::ptr
zip::create( const std::uint8_t *data, std::size_t len )
{
	return std::make_shared< miniz_zip >( data, len );
}


miniz_zip::miniz_zip( const std::uint8_t *data, std::size_t len )
{
	memset( &m_archive, 0, sizeof( m_archive ) );
	
	auto ok = mz_zip_reader_init_mem( &m_archive, data, len, 0 );
	
	if ( !ok )
	{
	//	nlog( log::level_t::error, "mz_zip_reader_init_mem() failed" );
		goto exit;
	}

	for ( auto i = 0u; i < mz_zip_reader_get_num_files( &m_archive ); i++ )
	{
		mz_zip_archive_file_stat file_stat;

		if ( mz_zip_reader_file_stat( &m_archive, i, &file_stat ) )
		{
			if ( !mz_zip_reader_is_file_a_directory( &m_archive, i ) )
			{
				m_map[ std::string( "/" ) + file_stat.m_filename ] = file_stat.m_file_index;
			}
		}
	}

exit:

	return;
}


miniz_zip::~miniz_zip()
{
	mz_zip_reader_end( &m_archive );
}


bool
miniz_zip::lookup( const std::string &name, std::uint32_t &index )
{
	auto it = m_map.find( name );
	bool ok = false;

	if ( it != m_map.end() )
	{
		index	= it->second;
		ok		= true;
	}

	return ok;
}


bool
miniz_zip::extract( std::uint32_t index, extract_f func )
{
	m_func = func;

	return mz_zip_reader_extract_to_callback( &m_archive, index, reinterpret_cast< mz_file_write_func >( callback ), this, 0 ) ? true : false;
}
