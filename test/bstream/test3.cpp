#include <nodeoze/test.h>
#include <nodeoze/bstream.h>
#include <nodeoze/bstream/ombstream.h>
#include <nodeoze/bstream/imbstream.h>
#include <thread>
#include <chrono>
// #include <bstream/msgpack.h>

using namespace nodeoze;

namespace test_types_3
{

class far : BSTRM_BASE(far)
{
public:
	far() {}
	far( const std::string& name ) : name_{name} {}
	virtual ~far() {}
	
	BSTRM_POLY_CLASS(far, , (name_))
	
    const std::string& name() const
    {
        return name_;
    }

	virtual std::string id() const
	{
		return std::string( "I AM FAR: " + name_ );
	}
	
protected:
	std::string name_;
};


class foo : BSTRM_BASE(foo), public far
{
public:
	BSTRM_FRIEND_BASE(foo)
	BSTRM_CTOR(foo, ( far ), )
	BSTRM_ITEM_COUNT( ( far ), )
	BSTRM_POLY_SERIALIZE(foo, ( far ), )
	
	foo(std::string const& name ) : far{ name } {}
			
	virtual std::string id() const override
	{
		return std::string( "I AM FOO: " + name() );
	}
};

using poly_factory_func = std::function< std::shared_ptr< void > ( std::string const& ) >;

poly_factory_func foo_far = [] ( std::string const& s )
{
	std::shared_ptr< foo > foop = std::make_shared< foo >( s );
	std::shared_ptr< far > farp_dcast =  std::dynamic_pointer_cast< far >( foop );
	std::shared_ptr< void > vfarp = farp_dcast;
	return vfarp;
};


} // namespace test_types_3

using namespace test_types_3;

#if 0

TEST_CASE("nodeoze/smoke/bstream/poly_shared_ptrs_no_erasure")
{
//	std::this_thread::sleep_for (std::chrono::seconds(10));

	auto foop_test = std::make_shared< foo >( "france is pork" );

	std::shared_ptr< far > farp_test = foop_test;

	std::shared_ptr< foo > foop_cast = std::dynamic_pointer_cast< foo >( farp_test );

	CHECK( foop_cast );
}

TEST_CASE("nodeoze/smoke/bstream/poly_shared_ptrs_erasure")
{
//	std::this_thread::sleep_for (std::chrono::seconds(10));

	auto foop = std::make_shared< foo >( "france is pork" );

	std::shared_ptr< void > vfoop = foop;

	std::shared_ptr< far > farp_scast =  std::static_pointer_cast< far >( vfoop );

	std::cout << "static cast to std::shared_ptr< far > from std::shared_ptr< void >: " << typeid( *farp_scast ).name() << std::endl;

	std::shared_ptr< foo > foop_dcast = std::dynamic_pointer_cast< foo >( farp_scast );

	CHECK( foop_dcast ); 
}

TEST_CASE("nodeoze/smoke/bstream/poly_shared_ptrs_half_erasure")
{
//	std::this_thread::sleep_for (std::chrono::seconds(10));

	auto foop = std::make_shared< foo >( "france is pork" );

	std::shared_ptr< far > farp_dcast =  std::dynamic_pointer_cast< far >( foop );

	std::shared_ptr< void > vfarp = farp_dcast;

	std::shared_ptr< far > farp_recast = std::static_pointer_cast< far >( vfarp );

	std::cout << "dynamic cast to std::shared_ptr< far > from std::shared_ptr< foo >: " << typeid( *farp_recast ).name() << std::endl;

	std::shared_ptr< foo > foop_dcast = std::dynamic_pointer_cast< foo >( farp_recast );

	CHECK( foop_dcast );

	if ( foop_dcast )
	{
		std::cout << foop->id() << std::endl;
	}
}

TEST_CASE("nodeoze/smoke/bstream/poly_shared_ptrs_factory_func")
{
//	std::this_thread::sleep_for (std::chrono::seconds(10));

	std::shared_ptr< far > farp_recast = std::static_pointer_cast< far >( foo_far( "france is ham" ) );

	std::cout << "dynamic cast to std::shared_ptr< far > from std::shared_ptr< foo >: " << typeid( *farp_recast ).name() << std::endl;

	std::shared_ptr< foo > foop_dcast = std::dynamic_pointer_cast< foo >( farp_recast );

	CHECK( foop_dcast );

	if ( foop_dcast )
	{
		std::cout << foop_dcast->id() << std::endl;
	}
}



TEST_CASE("nodeoze/smoke/bstream/poly_raw_ptrs_no_erasure")
{
//	std::this_thread::sleep_for (std::chrono::seconds(10));

	auto raw_foop = new foo( "france is pork" );

	far* raw_farp =  raw_foop;

	std::cout << "implicit cast to far*: " << typeid( *raw_farp ).name() << std::endl;

	foo* raw_foop_dcast = dynamic_cast< foo* >( raw_farp );

	CHECK( raw_foop_dcast ); 
}

TEST_CASE("nodeoze/smoke/bstream/poly_raw_ptrs_erasure")
{
//	std::this_thread::sleep_for (std::chrono::seconds(10));

	auto raw_foop = new foo( "france is pork" );

	void* vfoop = raw_foop;

	far* raw_farp_scast =  static_cast< far* >( vfoop );

	std::cout << "static cast to far* from void*: " << typeid( *raw_farp_scast ).name() << std::endl;

	foo* raw_foop_dcast = dynamic_cast< foo* >( raw_farp_scast );

	CHECK( raw_foop_dcast ); 
}

#endif

TEST_CASE("nodeoze/smoke/bstream/poly_raw_ptrs_half_erasure")
{
//	std::this_thread::sleep_for (std::chrono::seconds(10));

	auto raw_foop = new foo( "france is pork" );

	far* raw_farp_dcast = dynamic_cast< far* >( raw_foop );

	void* vfarp = raw_farp_dcast;

	far* raw_farp_recast = static_cast< far* >( vfarp );

//	std::cout << "static recast to far* from foo*: " << typeid( *raw_farp_recast ).name() << std::endl;

	auto raw_foop_dcast = dynamic_cast< foo* >( raw_farp_recast );

	CHECK( raw_foop_dcast );

	// if ( raw_foop_dcast )
	// {
	// 	std::cout << raw_foop_dcast->id() << std::endl;
	// }
}


TEST_CASE("nodeoze/smoke/bstream/poly_shared_ptrs")
{
	bstream::context< foo, far > cntxt;
	
	bstream::ombstream os{ 1024, cntxt };
	auto foop0 = std::make_shared< foo >( "france is bacon" );
	os << foop0;
	
	bstream::imbstream is{ os.get_buffer(), cntxt };

//	is.get_buffer().dump( std::cout );

	std::shared_ptr< far > farp0;

	is >> farp0;

//	far f1 = is.read_as<far>();

	CHECK( farp0 );

	far* raw_farp = farp0.get();

	foo* raw_foop = dynamic_cast< foo* >( raw_farp );

	CHECK( raw_foop != nullptr );

	auto foop1 = std::dynamic_pointer_cast< foo >( farp0 );

	CHECK( foop1 );

	// CHECK(foop0->name() == foop1->name());
	// CHECK(foop0->number() == foop1->number());
	// CHECK(foop0->real() == foop1->real());
}

TEST_CASE("nodeoze/smoke/bstream/poly_unique_ptrs")
{
	bstream::context< foo, far > cntxt;
	
	bstream::ombstream os{ 1024, cntxt };
	auto foop0 = std::make_unique< foo >( "france is bacon" );
	os << foop0;
	
	bstream::imbstream is{ os.get_buffer(), cntxt };

//	is.get_buffer().dump( std::cout );

	std::unique_ptr< far > farp0;

	is >> farp0;

//	far f1 = is.read_as<far>();

	CHECK( farp0 );

	far* raw_farp = farp0.get();

	foo* raw_foop = dynamic_cast< foo* >( raw_farp );

	CHECK( raw_foop != nullptr );

//	std::cout << farp0->id() << std::endl;


	// CHECK(foop0->name() == foop1->name());
	// CHECK(foop0->number() == foop1->number());
	// CHECK(foop0->real() == foop1->real());
}
