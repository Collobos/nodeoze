#include <nodeoze/ntest.h>
#include <nodeoze/bstream.h>
#include <nodeoze/bstream/ombstream.h>
#include <nodeoze/bstream/imbstream.h>
#include <nodeoze/bstream/msgpack.h>

using namespace nodeoze;

class fee
{
public:
    fee() {}
    
    fee(msgpack::object const& o)
    {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 2) throw msgpack::type_error();
        n = o.via.array.ptr[0].as<int>();
        s = o.via.array.ptr[1].as<std::string>();
    }
    
    fee(int num, std::string const& str) : n{num}, s{str} {}
    int n;
    std::string s;
    
    MSGPACK_DEFINE(n, s);
};

class fie
{
public:
    fie() {}
        
    fie(int num, std::string const& str) : n{num}, s{str} {}
    int n;
    std::string s;
    
    MSGPACK_DEFINE(n, s);
};

class foe
{
public:
    foe() {}
        
    foe(int num, std::string const& str) : n{num}, s{str} {}
    int n;
    std::string s;    
};


class fum
{
public:
    fum() {}
        
    fum(int num, std::string const& str) : n{num}, s{str} {}
    int n;
    std::string s;    
};


namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor 
{
template <>
struct as<foe> 
{
    foe operator()(msgpack::object const& o) const {
        if (o.type != msgpack::type::ARRAY) throw msgpack::type_error();
        if (o.via.array.size != 2) throw msgpack::type_error();
        return foe(o.via.array.ptr[0].as<int>(), o.via.array.ptr[1].as<std::string>());
    }
};

template<>
struct pack<foe> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, foe const& v) const {
        // packing member variables as an array.
        o.pack_array(2);
        o.pack(v.n);
        o.pack(v.s);
        return o;
    }
};

template<>
struct pack<fum> {
    template <typename Stream>
    packer<Stream>& operator()(msgpack::packer<Stream>& o, fum const& v) const {
        // packing member variables as an array.
        o.pack_array(2);
        o.pack(v.n);
        o.pack(v.s);
        return o;
    }
};

	
// Place class template specialization here
template<>
struct convert<fum> 
{
	msgpack::object const& operator()(msgpack::object const& o, fum& v) const 
	{
		try
		{
			if (o.type != msgpack::type::ARRAY)
			{
				throw msgpack::type_error();
			}
			if (o.via.array.size != 2)
			{
				throw msgpack::type_error();
			}
				
			v.n = o.via.array.ptr[0].as<int>();
			v.s = o.via.array.ptr[1].as<std::string>();
		}
		catch (msgpack::type_error const& e)
		{
			std::rethrow_exception(std::current_exception());
		}
		return o;
	}
};
} // adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // msgpack


BSTRM_HAS_MSGPACK_CVT_ADAPTOR(fum);
BSTRM_HAS_MSGPACK_PACK_ADAPTOR(fum);
BSTRM_HAS_MSGPACK_PACK_ADAPTOR(foe);
BSTRM_HAS_MSGPACK_AS_ADAPTOR(foe);


TEST_CASE("nodeoze/smoke/bstream/msgpack_integration")
{
	CHECK(bstream::is_msgpack_object_constructible<fee>::value);
	CHECK(bstream::has_msgpack_unpack_method<fee>::value);
	CHECK(bstream::has_msgpack_pack_method<fee>::value);
	CHECK(!bstream::has_msgpack_as_adaptor<fee>::value);
	
	CHECK(bstream::has_value_deserializer<fee>::value);
	CHECK(bstream::has_ref_deserializer<fee>::value);

	CHECK(!bstream::is_msgpack_object_constructible<fie>::value);
	CHECK(bstream::has_msgpack_unpack_method<fie>::value);
	CHECK(bstream::has_msgpack_pack_method<fie>::value);
	CHECK(!bstream::has_msgpack_as_adaptor<fie>::value);

	CHECK(!bstream::is_msgpack_object_constructible<foe>::value);
	CHECK(!bstream::has_msgpack_unpack_method<foe>::value);
	CHECK(!bstream::has_msgpack_pack_method<foe>::value);
	CHECK(!bstream::has_msgpack_cvt_adaptor<foe>::value);
	CHECK(bstream::has_msgpack_pack_adaptor<foe>::value);
	CHECK(bstream::has_msgpack_as_adaptor<foe>::value);
	
	CHECK(!bstream::is_msgpack_object_constructible<fum>::value);
	CHECK(!bstream::has_msgpack_unpack_method<fum>::value);
	CHECK(!bstream::has_msgpack_pack_method<fum>::value);
	CHECK(bstream::has_msgpack_cvt_adaptor<fum>::value);
	CHECK(bstream::has_msgpack_pack_adaptor<fum>::value);
	CHECK(!bstream::has_msgpack_as_adaptor<fum>::value);

	{	
		bstream::ombstream os{1024};

		fee mpc0;
		mpc0.s = "zoot";
		mpc0.n = 27;

		os << mpc0;
		bstream::imbstream is{ os.get_buffer() };
		fee mpc1 = is.read_as<fee>();

		CHECK(mpc1.s == mpc0.s);
		CHECK(mpc1.n == mpc0.n);
		
		is.rewind();
		fee mpc2;
		is >> mpc2;
		
		CHECK(mpc2.s == mpc0.s);
		CHECK(mpc2.n == mpc0.n);
	}

	{	
		bstream::ombstream os{1024};

		fie mpc0;
		mpc0.s = "zoot";
		mpc0.n = 27;

		os << mpc0;
		bstream::imbstream is{ os.get_buffer() };
		fie mpc1 = is.read_as<fie>();

		CHECK(mpc1.s == mpc0.s);
		CHECK(mpc1.n == mpc0.n);
		
		is.rewind();
		fie mpc2;
		is >> mpc2;
		
		CHECK(mpc2.s == mpc0.s);
		CHECK(mpc2.n == mpc0.n);
	}

	{	
		bstream::ombstream os{1024};

		foe mpc0;
		mpc0.s = "zoot";
		mpc0.n = 27;

		os << mpc0;
		bstream::imbstream is{ os.get_buffer() };
		foe mpc1 = is.read_as<foe>();

		CHECK(mpc1.s == mpc0.s);
		CHECK(mpc1.n == mpc0.n);
		
		is.rewind();
		foe mpc2;
		is >> mpc2;
		
		CHECK(mpc2.s == mpc0.s);
		CHECK(mpc2.n == mpc0.n);
	}

	{	
		bstream::ombstream os{1024};

		fum mpc0;
		mpc0.s = "zoot";
		mpc0.n = 27;

		os << mpc0;
		bstream::imbstream is{ os.get_buffer() };
		fum mpc1 = is.read_as<fum>();

		CHECK(mpc1.s == mpc0.s);
		CHECK(mpc1.n == mpc0.n);
		
		is.rewind();
		fum mpc2;
		is >> mpc2;
		
		CHECK(mpc2.s == mpc0.s);
		CHECK(mpc2.n == mpc0.n);
	}

}


