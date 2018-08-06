#ifndef NODEOZE_BSTREAM_FWD_DECLS_H
#define NODEOZE_BSTREAM_FWD_DECLS_H

namespace nodeoze
{
namespace bstream
{

template< class T, class Enable = void >
struct value_deserializer;

template< class T, class Enable = void >
struct ref_deserializer;

template< class T, class Enable = void >
struct ptr_deserializer;

template< class T, class Enable = void >
struct shared_ptr_deserializer;

template< class T, class Enable = void >
struct unique_ptr_deserializer;

template< class T, class Enable = void >
struct serializer;

template< class Derived, class Base, class Enable = void >
struct base_serializer;

template< class T, class Enable = void > 
struct ibstream_initializer;

class ibstream;

class obstream;

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_FWD_DECLS_H