#include <nodeoze/bstream/error_category_context.h>
#include <nodeoze/bstream/error.h>

using namespace nodeoze;
using namespace bstream;

const error_category_context::category_init_list error_category_context::m_default_categories =
{
    &std::generic_category(),
    &bstream::bstream_category()
};

error_category_context::error_category_context( category_init_list init_list )
{
	m_category_vector.reserve( init_list.size() + m_default_categories.size() );
	m_category_vector.insert(m_category_vector.end(), m_default_categories.begin(), m_default_categories.end() );
	m_category_vector.insert(m_category_vector.end(), init_list.begin(), init_list.end() );

	for ( auto i = 0u; i < m_category_vector.size(); ++i )
	{
		m_category_map.emplace( m_category_vector[ i ], i );
	}
}

error_category_context::error_category_context()
{
	m_category_vector.reserve( m_default_categories.size() );
	m_category_vector.insert(m_category_vector.end(), m_default_categories.begin(), m_default_categories.end() );

	for ( auto i = 0u; i < m_category_vector.size(); ++i )
	{
		m_category_map.emplace( m_category_vector[ i ], i );
	}
}

std::error_category const&
error_category_context::category_from_index( index_type index ) const
{
	if ( index < 0  || static_cast< std::size_t >( index ) >= m_category_vector.size() )
	{
		throw std::system_error( make_error_code( bstream::errc::invalid_err_category ) );
	}
	else
	{
		return *( m_category_vector[ index ] );
	}
}

error_category_context::index_type
error_category_context::index_of_category( std::error_category const& category ) const
{
	auto it = m_category_map.find( &category );
	if ( it != m_category_map.end() )
	{
		return it->second;
	}
	else
	{
		throw std::system_error( make_error_code( bstream::errc::invalid_err_category ) );
	}
}
