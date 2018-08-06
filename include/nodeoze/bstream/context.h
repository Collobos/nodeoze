#ifndef NODEOZE_BSTREAM_CONTEXT_H
#define NODEOZE_BSTREAM_CONTEXT_H

#include <nodeoze/bstream/error_category_context.h>
#include <nodeoze/bstream/types.h>
#include <boost/endian/conversion.hpp>
#include <nodeoze/bstream/error.h>
#include <nodeoze/bstream/utils/traits.h>
#include <nodeoze/bstream/ibstream_traits.h>
#include <typeindex>

namespace nodeoze
{
namespace bstream
{

class ibstream;

class context_impl_base : public error_category_context
{
public:
    static constexpr bool is_valid_tag( poly_tag_type tag ) { return tag != invalid_tag; };

    context_impl_base( bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    error_category_context{},
    m_dedup_shared_ptrs{ dedup_shared_ptrs },
    m_byte_order{ byte_order }
    {}

    context_impl_base( error_category_context::category_init_list categories, bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    error_category_context{ categories },
    m_dedup_shared_ptrs{ dedup_shared_ptrs },
    m_byte_order{ byte_order }
    {}

    virtual ~context_impl_base() {}

    virtual poly_tag_type
    get_type_tag( std::type_index index ) const = 0;

    template< class T >
    poly_tag_type
    get_type_tag() const
    {
        return get_type_tag( typeid( T ) );
    }

    virtual bool
    can_downcast_ptr( poly_tag_type from, poly_tag_type to ) const = 0;

    virtual void*
    create_raw_from_tag( poly_tag_type tag, ibstream& is ) const = 0;

    virtual std::shared_ptr< void >
    create_shared_from_tag( poly_tag_type tag, ibstream& is ) const = 0;

    template< class T >
    T*
    create_raw( poly_tag_type tag, ibstream& is ) const
    {
        if ( can_downcast_ptr( tag, get_type_tag< T >() ) )
        {
            return static_cast< T* >( create_raw_from_tag( tag, is ) );
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::invalid_ptr_downcast ) };
        }
    }

    template< class T >
    std::shared_ptr< T >
    create_shared( poly_tag_type tag, ibstream& is ) const
    {
        if ( can_downcast_ptr( tag, get_type_tag< T >() ) )
        {
            return std::static_pointer_cast< T >( create_shared_from_tag( tag, is ) );
        }
        else
        {
            throw std::system_error{ make_error_code( bstream::errc::invalid_ptr_downcast ) };
        }
    }

    bool
    dedup_shared_ptrs() const
    {
        return m_dedup_shared_ptrs;
    }

    boost::endian::order
    byte_order() const
    {
        return m_byte_order;
    }

private:
    bool                        m_dedup_shared_ptrs;
    boost::endian::order        m_byte_order;
};

using poly_raw_factory_func = std::function< void* ( ibstream& ) >;

using poly_shared_factory_func = std::function< std::shared_ptr< void > ( ibstream& ) >;

template< class T1, class T2, class Enable = void >
struct can_downcast_ptr : public std::false_type {};

template< class T1, class T2 >
struct can_downcast_ptr< T1, T2, std::enable_if_t< std::is_base_of< T2, T1 >::value > > : public std::true_type {};

template< class T, class... Args >
struct can_downcast_ptr_row
{
    static const std::vector< bool > row_values;
};

template< class T, class... Args >
const std::vector< bool > can_downcast_ptr_row< T, Args... >::row_values = { can_downcast_ptr< T, Args >::value... };

template< class... Args >
struct can_downcast_ptr_table
{
    static const std::vector< std::vector< bool > > values;
};

template< class... Args >
const std::vector< std::vector< bool > > can_downcast_ptr_table< Args... >::values = 
{ 
    { can_downcast_ptr_row< Args, Args... >::row_values }... 
};

template< class... Args >
class context_impl : public context_impl_base
{
public:

    context_impl( bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    context_impl_base{ dedup_shared_ptrs, byte_order }
    {}

    context_impl( error_category_context::category_init_list categories, bool dedup_shared_ptrs, boost::endian::order byte_order )
    :
    context_impl_base{ categories, dedup_shared_ptrs, byte_order }
    {}

    virtual poly_tag_type 
    get_type_tag( std::type_index index ) const override
    {
        auto it = m_type_tag_map.find( index );
        if ( it != m_type_tag_map.end() )
        {
            return it->second;
        }
        else
        {
            return invalid_tag;
        }
    }

    virtual bool 
    can_downcast_ptr( poly_tag_type from, poly_tag_type to ) const override
    {
        return m_downcast_ptr_table.values[ from ][ to ];
    }

    virtual void*
    create_raw_from_tag( poly_tag_type tag, ibstream& is ) const override
    {
        return m_factories[ tag ]( is );
    }

    virtual std::shared_ptr< void >
    create_shared_from_tag( poly_tag_type tag, ibstream& is ) const override
    {
        return m_shared_factories[ tag ]( is );
    }

protected:
    static const std::unordered_map< std::type_index, poly_tag_type >   m_type_tag_map;
    static const can_downcast_ptr_table< Args... >                      m_downcast_ptr_table;
    static const std::vector< poly_raw_factory_func >                   m_factories;
    static const std::vector< poly_shared_factory_func >                m_shared_factories;
};

template< class... Args >
const std::unordered_map< std::type_index, poly_tag_type > context_impl< Args... >::m_type_tag_map = 
{ 
    { typeid( Args ), utils::index< Args, Args... >::value }... 
};

template< class... Args >
const can_downcast_ptr_table< Args... > context_impl< Args... >::m_downcast_ptr_table;

template< class T, class Enable = void >
struct poly_factory;

template< class T >
struct poly_factory< T, std::enable_if_t< std::is_abstract< T >::value || ! has_ptr_deserializer< T >::value > >
{
    void* 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static void*
    get( ibstream& )
    {
        return nullptr;
    }
};

template< class T >
struct poly_factory< T, std::enable_if_t< ! std::is_abstract< T >::value && has_ptr_deserializer< T >::value > >
{
    void* 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static void*
    get( ibstream& is )
    {
        return ptr_deserializer< T >::get( is );
    }
};

template< class... Args >
const std::vector< poly_raw_factory_func > context_impl< Args... >::m_factories = 
{
    []( ibstream& is ) 
    {
        return poly_factory< Args >::get( is );
    }...
};

template< class T, class Enable = void >
struct poly_shared_factory;

template< class T >
struct poly_shared_factory< T, std::enable_if_t< std::is_abstract< T >::value || ! has_shared_ptr_deserializer< T >::value > >
{
    std::shared_ptr< void > 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static std::shared_ptr< void >
    get( ibstream& )
    {
        return nullptr;
    }
};

template< class T >
struct poly_shared_factory< T, std::enable_if_t< ! std::is_abstract< T >::value && has_shared_ptr_deserializer< T >::value > >
{
    std::shared_ptr< void > 
    operator()( ibstream& is ) const
    {
        return get( is );
    }

    static std::shared_ptr< void >
    get( ibstream& is )
    {
        return shared_ptr_deserializer< T >::get( is );
    }
};

template< class... Args >
const std::vector< poly_shared_factory_func > context_impl< Args... >::m_shared_factories =
{
    [] ( ibstream& is )
    {
        return poly_shared_factory< Args >::get( is );
    }...
};

class context_base 
{
public:

    virtual ~context_base() {}

    virtual std::shared_ptr< const context_impl_base >
    get_context_impl() const = 0;

};

template< class... Args >
class context : public context_base
{
public:

    context( bool dedup_shared_ptrs = true, boost::endian::order byte_order = boost::endian::order::big )
    :
    m_context_impl{ std::make_shared< const context_impl< Args... > >( dedup_shared_ptrs, byte_order ) }
    {}

    context( error_category_context::category_init_list categories, bool dedup_shared_ptrs = true, boost::endian::order byte_order = boost::endian::order::big )
    :
    m_context_impl{ std::make_shared< const context_impl< Args... > >( categories, dedup_shared_ptrs, byte_order ) }
    {}
    
    virtual std::shared_ptr< const context_impl_base >
    get_context_impl() const override
    {
        return m_context_impl;
    }

private:
    std::shared_ptr< const context_impl_base >   m_context_impl;
};


inline context_base const& get_default_context()
{
    static const context<> default_context{ {} };
    return default_context;
}

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_CONTEXT_H
