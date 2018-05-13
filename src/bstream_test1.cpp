#include <nodeoze/test.h>
#include <nodeoze/bstream.h>
#include <nodeoze/bstream/ombstream.h>
#include <nodeoze/bstream/imbstream.h>
// #include <bstream/msgpack.h>

using namespace nodeoze;

class far : BSTRM_MAP_BASE(far)
{
public:
	far() {}
	far(const std::string& name, int num) : name_{name}, number_{num} {}
	
	BSTRM_MAP_CLASS(far, , (name_, number_))
	
    const std::string& name() const
    {
        return name_;
    }

   	int number() const
	{
		return number_;
	}
	
private:
	std::string name_;
    int number_;
};


class foo : BSTRM_BASE(foo)
{
public:
	BSTRM_FRIEND_BASE(foo)
	BSTRM_CTOR(foo, , (name_, number_))
	BSTRM_ITEM_COUNT( , (name_, number_))
	BSTRM_SERIALIZE(foo, , (name_, number_))
	
	foo(std::string const& name, int number) : name_{name}, number_{number} {}
	
	bstream::ibstream& deserialize(bstream::ibstream& is)
	{
		is.check_array_header(2);
		name_ = is.read_as<std::string>();
		number_ = is.read_as<int>();
		return is;
	}

	std::string const& name() const
	{
		return name_;
	}
	
	int number() const
	{
		return number_;
	}
	
private:
	std::string name_;
	int number_;
};


TEST_CASE("nodeoze/smoke/bstream/array_base_macro")
{
	bstream::ombstream os{1024};
	foo f0("france is bacon", 27);
	os << f0;
	
	bstream::imbstream is{ os.get_buffer() };
	
	foo f1 = is.read_as<foo>();
	CHECK(f1.name() == f0.name());
	CHECK(f1.number() == f0.number());
}

TEST_CASE("nodeoze/smoke/bstream/map_base_macro")
{
	bstream::ombstream os{1024};
	far f0("france is bacon", 27);
	os << f0;
	
	bstream::imbstream is{ os.get_buffer() };
	
	far f1 = is.read_as<far>();
	CHECK(f1.name() == f0.name());
	CHECK(f1.number() == f0.number());

}
