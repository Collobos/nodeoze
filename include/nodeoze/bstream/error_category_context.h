#ifndef NODEOZE_BSTREAM_ERROR_CATEGORY_CONTEXT_H
#define NODEOZE_BSTREAM_ERROR_CATEGORY_CONTEXT_H

#include <vector>
#include <unordered_map>
#include <deque>
#include <initializer_list>
#include <system_error>
#include <nodeoze/bstream/error.h>

namespace nodeoze
{
namespace bstream
{

class error_category_context
{
public:
    using index_type = int;
    using category_vector = std::vector< const std::error_category* >;
    using category_map = std::unordered_map< const std::error_category*, index_type >;
	using category_init_list = std::initializer_list< const std::error_category* >;


    error_category_context();

    error_category_context( category_init_list init_list );

    std::error_category const&
    category_from_index( index_type index ) const;

    index_type
    index_of_category( std::error_category const& category ) const;

private:
    category_vector                     m_category_vector;
    category_map                        m_category_map;
    static const category_init_list     m_default_categories;
};

} // namespace bstream
} // namespace nodeoze

#endif // NODEOZE_BSTREAM_ERROR_CATEGORY_MAP_H