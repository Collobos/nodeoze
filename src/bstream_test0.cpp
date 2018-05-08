#include <nodeoze/ntest.h>
#include <nodeoze/bstream.h>
#include <nodeoze/bstream/msgpack.h>

using namespace nodeoze;

namespace test_types
{
	struct simple_state
	{
		std::string name;
		int value;
		
		
		simple_state() : name{}, value{0} {}
		simple_state(std::string const& nstr, int val) : name{nstr}, value{val} {}
		
		simple_state(simple_state const& other) : name{other.name}, value{other.value} {}
		simple_state(simple_state&& other) : name{std::move(other.name)}, value{other.value} {}
		
		bool check_state(std::string const& nstr, int val)
		{
			return nstr == name && val == value;
		}
		
		bool check_state(simple_state const& other) const
		{
			return name == other.name && value == other.value;
		}
		
		bool operator==(simple_state const& other) const
		{
			return name == other.name && value == other.value;
		}
		
		bool operator!=(simple_state const& other) const
		{
			return !(*this == other);
		}
		
		simple_state& operator=(simple_state const& other)
		{
			name = other.name;
			value = other.value;
			return *this;
		}

		simple_state& operator=(simple_state&& other)
		{
			name = std::move(other.name);
			value = other.value;
			return *this;
		}
	};

	struct fee : public simple_state // serialize method/deserialize method/default ctor/copy ctor
	{
		fee() = default;
		fee(std::string const& nstr, int val) : simple_state{nstr, val} {}
		fee(fee const& other) = default;
		fee(fee&&) = default;
		
		bstream::obstream& serialize(bstream::obstream& os) const
		{
			os.write_array_header(2);
			os << name << value;
			return os;
		}
		
		bstream::ibstream& deserialize(bstream::ibstream& is)
		{
			is.check_array_header(2) >> name >> value;
			return is;
		}
		fee& operator=(fee&& other) = default;
		fee& operator=(fee const& other) = default;
	};
}

namespace std 
{
	template<> struct hash<test_types::fee>  
	{
		typedef test_types::fee argument_type;
		typedef std::hash<std::string>::result_type result_type;
		result_type operator()( const argument_type& arg ) const  {
			std::hash<std::string> hasher;
			return hasher(arg.name);
		}  
	};

	template<> struct less<test_types::fee>
	{

		typedef test_types::fee first_argument_type;
		typedef test_types::fee second_argument_type;
		typedef bool result_type;
		result_type operator()(const first_argument_type& x, const second_argument_type& y) const
		{
			return x.name < y.name;
		}
	};
}

namespace test_types
{
	struct fie : public simple_state // serializer/value_deserializer/move ctor
	{
		fie() = default;
		fie(std::string const& nstr, int val) : simple_state{nstr, val} {}
		fie(fie const&) = default;
		fie(fie&&) = default;
		fie& operator=(fie&& other) = default;
		fie& operator=(fie const& other) = default;
	};
}

namespace std 
{
	template<> struct hash<test_types::fie>  
	{
		typedef test_types::fie argument_type;
		typedef std::hash<std::string>::result_type result_type;
		result_type operator()( const argument_type& arg ) const  {
			std::hash<std::string> hasher;
			return hasher(arg.name);
		}  
	}; 
	
	template<> struct less<test_types::fie>
	{

		typedef test_types::fie first_argument_type;
		typedef test_types::fie second_argument_type;
		typedef bool result_type;
		result_type operator()(const first_argument_type& x, const second_argument_type& y) const
		{
			return x.name < y.name;
		}
	};
}

template<>
struct bstream::serializer<test_types::fie>
{
	static inline bstream::obstream& put(bstream::obstream& os, test_types::fie const& obj)
	{
		os.write_array_header(2);
		os << obj.name << obj.value;
		return os;
	}
};

template<>
struct bstream::value_deserializer<test_types::fie>
{
	static inline test_types::fie get(bstream::ibstream& is)
	{
		is.check_array_header(2);
		auto s = is.read_as<std::string>();
		auto v = is.read_as<int>();
		return test_types::fie(s, v);
	}
};

namespace test_types
{
	struct foe : public simple_state // serializer/ref_deserializer/default ctor/copy ctor
	{
		foe() {}
		foe(std::string const& nstr, int val) : simple_state{nstr, val} {}
		foe(foe const&) = default;
		foe(foe&&) = default;
		foe& operator=(foe&& other) = default;
		foe& operator=(foe const& other) = default;		
	};	
}

namespace std 
{
	template<> struct hash<test_types::foe>  
	{
		typedef test_types::foe argument_type;
		typedef std::hash<std::string>::result_type result_type;
		result_type operator()( const argument_type& arg ) const  {
			std::hash<std::string> hasher;
			return hasher(arg.name);
		}  
	};
	
	template<> struct less<test_types::foe>
	{

		typedef test_types::foe first_argument_type;
		typedef test_types::foe second_argument_type;
		typedef bool result_type;
		result_type operator()(const first_argument_type& x, const second_argument_type& y) const
		{
			return x.name < y.name;
		}
	};
}

template<>
struct bstream::serializer<test_types::foe>
{
	static inline bstream::obstream& put(bstream::obstream& os, test_types::foe const& obj)
	{
		os.write_array_header(2);
		os << obj.name << obj.value;
		return os;
	}
};
template<>
struct bstream::ref_deserializer<test_types::foe>
{
	static inline bstream::ibstream& get(bstream::ibstream& is, test_types::foe& obj)
	{
		is.check_array_header(2);
		is >> obj.name >> obj.value;
		return is;
	}
};

namespace test_types
{
	struct fum : public simple_state // serialize/istream ctor/move ctor
	{
		fum() = default;
		fum(bstream::ibstream& is)
		{
			is.check_array_header(2);
			is >> name >> value;
		}
		fum(std::string const& nstr, int val) : simple_state{nstr, val} {}
		fum(fum const&) = default;
		fum(fum&&) = default;
		fum& operator=(fum&& other) = default;
		fum& operator=(fum const& other) = default;
		
		bstream::obstream& serialize(bstream::obstream& os) const
		{
			os.write_array_header(2);
			os << name << value;
			return os;
		}
	};	
}

namespace std 
{
	template<> struct hash<test_types::fum>  
	{
		typedef test_types::fum argument_type;
		typedef std::hash<std::string>::result_type result_type;
		result_type operator()( const argument_type& arg ) const  {
			std::hash<std::string> hasher;
			return hasher(arg.name);
		}  
	};
	
	template<> struct less<test_types::fum>
	{

		typedef test_types::fum first_argument_type;
		typedef test_types::fum second_argument_type;
		typedef bool result_type;
		result_type operator()(const first_argument_type& x, const second_argument_type& y) const
		{
			return x.name < y.name;
		}
	};
}

namespace test_types
{
	class foo : BSTRM_MAP_BASE(foo), 
			public fee, public fie, public foe, public fum
	{
	public:
		foo() {}
		foo(fee const& a, fie const& b, foe const& c, fum const& d, std::string const& l) :
		fee{a}, fie{b}, foe{c}, fum{d}, label{l} {}

		BSTRM_MAP_CLASS(foo, (fee, fie, foe, fum) , (label))

		bool operator==(foo const& other) const
		{
			return static_cast<fee>(*this) == static_cast<fee>(other) &&
					static_cast<fie>(*this) == static_cast<fie>(other) &&
					static_cast<foe>(*this) == static_cast<foe>(other) &&
					static_cast<fum>(*this) == static_cast<fum>(other) && 
					label == other.label;
		}

		bool operator!=(foo const& other) const
		{
			return !(*this == other);
		}

	private:
		std::string label;
	};
}

/*
 * Macros to verify buffer contents at 
 * representational boundaries for integer 
 * numeric types
 */

#define CHECK_0(ostrm)							\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0);							\
	ostrm.clear();								\
}												\
/**/

#define CHECK_neg_fixint_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_neg_fixint_min(ostrm)				\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0xe0);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_neg_fixint_min_minus_1(ostrm)		\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd0);						\
	byte = is.get();							\
	CHECK(byte == 0xdf);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_pos_fixint_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 1);						\
	auto byte = is.get();						\
	CHECK(byte == 0x7f);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_pos_fixint_max_plus_1(ostrm)		\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcc);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_8_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd0);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_8_min_minus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd1);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0x7f);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_8_max(ostrm)					\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 2);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcc);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_8_max_plus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcd);						\
	byte = is.get();							\
	CHECK(byte == 0x01);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_16_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd1);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_16_min_minus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd2);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0x7f);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_16_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 3);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcd);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_16_max_plus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xce);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x01);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_32_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd2);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_32_min_minus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd3);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0x7f);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_32_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 5);						\
	auto byte = is.get();						\
	CHECK(byte == 0xce);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_32_max_plus_1(ostrm)			\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcf);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x01);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_int_64_min(ostrm)					\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xd3);						\
	byte = is.get();							\
	CHECK(byte == 0x80);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	byte = is.get();							\
	CHECK(byte == 0x00);						\
	ostrm.clear();								\
}												\
/**/

#define CHECK_uint_64_max(ostrm)				\
{												\
	bstream::ibstream is(ostrm.get_buffer());	\
	CHECK(is.size() == 9);						\
	auto byte = is.get();						\
	CHECK(byte == 0xcf);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	byte = is.get();							\
	CHECK(byte == 0xff);						\
	ostrm.clear();								\
}												\
/**/

TEST_CASE("nodeoze/smoke/bstream/numeric_representation")
{
	bstream::obstream os{1024};
	
	os << (std::uint8_t)0;
	CHECK_0(os);
	os << (std::int8_t)0;
	CHECK_0(os);
	os << (std::uint16_t)0;
	CHECK_0(os);
	os << (std::int16_t)0;
	CHECK_0(os);
	os << (std::uint32_t)0;
	CHECK_0(os);
	os << (std::int32_t)0;
	CHECK_0(os);
	os << (std::uint64_t)0;
	CHECK_0(os);
	os << (std::int64_t)0;
	CHECK_0(os);
	
	os << (std::int8_t)-1;
	CHECK_neg_fixint_max(os);
	os << (std::int16_t)-1;
	CHECK_neg_fixint_max(os);
	os << (std::int32_t)-1;
	CHECK_neg_fixint_max(os);
	os << (std::int64_t)-1;
	CHECK_neg_fixint_max(os);
	
	os << (std::int8_t)-32;
	CHECK_neg_fixint_min(os);
	os << (std::int16_t)-32;
	CHECK_neg_fixint_min(os);
	os << (std::int32_t)-32;
	CHECK_neg_fixint_min(os);
	os << (std::int64_t)-32;
	CHECK_neg_fixint_min(os);
	
	os << (std::int8_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	os << (std::int16_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	os << (std::int32_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	os << (std::int64_t)-33;
	CHECK_neg_fixint_min_minus_1(os);
	
	os.clear();
	os << (std::uint8_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int8_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::uint16_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int16_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::uint32_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int32_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::uint64_t)127;
	CHECK_pos_fixint_max(os);
	os << (std::int64_t)127;
	CHECK_pos_fixint_max(os);

	os << (std::uint8_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::uint16_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::int16_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::uint32_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::int32_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::uint64_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	os << (std::int64_t)128;
	CHECK_pos_fixint_max_plus_1(os);	
	
	os << (std::int8_t)-128;
	CHECK_int_8_min(os);
	os << (std::int16_t)-128;
	CHECK_int_8_min(os);
	os << (std::int32_t)-128;
	CHECK_int_8_min(os);
	os << (std::int64_t)-128;
	CHECK_int_8_min(os);
	
	os << (std::int16_t)-129;
	CHECK_int_8_min_minus_1(os);
	os << (std::int32_t)-129;
	CHECK_int_8_min_minus_1(os);
	os << (std::int64_t)-129;
	CHECK_int_8_min_minus_1(os);
	
	os << (std::uint8_t)255;
	CHECK_uint_8_max(os);	
	os << (std::uint16_t)255;
	CHECK_uint_8_max(os);	
	os << (std::int16_t)255;
	CHECK_uint_8_max(os);	
	os << (std::uint32_t)255;
	CHECK_uint_8_max(os);	
	os << (std::int32_t)255;
	CHECK_uint_8_max(os);	
	os << (std::uint64_t)255;
	CHECK_uint_8_max(os);	
	os << (std::int64_t)255;
	CHECK_uint_8_max(os);	
	
	os << (std::uint16_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::int16_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::uint32_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::int32_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::uint64_t)256;
	CHECK_uint_8_max_plus_1(os);	
	os << (std::int64_t)256;
	CHECK_uint_8_max_plus_1(os);
	
	os << (std::int16_t)-32768;
	CHECK_int_16_min(os);
	os << (std::int32_t)-32768;
	CHECK_int_16_min(os);
	os << (std::int64_t)-32768;
	CHECK_int_16_min(os);
	
	os << (std::int32_t)-32769;
	CHECK_int_16_min_minus_1(os);
	os << (std::int64_t)-32769;
	CHECK_int_16_min_minus_1(os);
	
	os << (std::uint16_t)65535;
	CHECK_uint_16_max(os);
	os << (std::uint32_t)65535;
	CHECK_uint_16_max(os);
	os << (std::int32_t)65535;
	CHECK_uint_16_max(os);
	os << (std::uint64_t)65535;
	CHECK_uint_16_max(os);
	os << (std::int64_t)65535;
	CHECK_uint_16_max(os);
	
	os << (std::uint32_t)65536;
	CHECK_uint_16_max_plus_1(os);	
	os << (std::int32_t)65536;
	CHECK_uint_16_max_plus_1(os);	
	os << (std::uint64_t)65536;
	CHECK_uint_16_max_plus_1(os);	
	os << (std::int64_t)65536;
	CHECK_uint_16_max_plus_1(os);	

	os << (std::int32_t)-2147483648;
	CHECK_int_32_min(os);
	os << (std::int64_t)-2147483648;
	CHECK_int_32_min(os);
	
	os << (std::int64_t)-2147483649;
	CHECK_int_32_min_minus_1(os);
	
	os << (std::uint32_t)4294967295;
	CHECK_uint_32_max(os);
	os << (std::uint64_t)4294967295;
	CHECK_uint_32_max(os);
	os << (std::int64_t)4294967295;
	CHECK_uint_32_max(os);

	os << (std::uint64_t)4294967296;
	CHECK_uint_32_max_plus_1(os);
	os << (std::int64_t)4294967296;
	CHECK_uint_32_max_plus_1(os);

	os << (std::int64_t)std::numeric_limits<std::int64_t>::min();
	CHECK_int_64_min(os);

	os << (std::uint64_t)18446744073709551615ull;
	CHECK_uint_64_max(os);
}

#define WRITE_READ_TEST(type, value)			\
{												\
	bstream::obstream os{1024};					\
	type v0 = value;							\
	os << v0;									\
	bstream::ibstream is{ os.get_buffer() };	\
	type v1;									\
	is >> v1;									\
	CHECK(v1 == v0);							\
	is.rewind();								\
	auto v2 = is.read_as<type>();				\
	CHECK(v2 == v0);							\
}												\

#define READ_TYPE_ERROR_TEST(type, value)		\
{												\
	bstream::obstream os{1024};					\
	os << value;								\
	bstream::ibstream is{ os.get_buffer() };	\
	try {										\
		type v;									\
		is >> v;								\
		CHECK(false);							\
	} catch (std::system_error const& e) {		\
		auto ec = make_error_code(				\
			bstream::errc::type_error );		\
		CHECK( e.code() == ec );				\
	}											\
}												\
/**/

TEST_CASE("nodeoze/smoke/bstream/numeric_write_read")
{
	WRITE_READ_TEST(std::int8_t, 0);
	WRITE_READ_TEST(std::uint8_t, 0);
	WRITE_READ_TEST(std::int16_t, 0);
	WRITE_READ_TEST(std::uint16_t, 0);
	WRITE_READ_TEST(std::int32_t, 0);
	WRITE_READ_TEST(std::uint32_t, 0);
	WRITE_READ_TEST(std::int64_t, 0);
	WRITE_READ_TEST(std::uint64_t, 0);

	WRITE_READ_TEST(std::int8_t, 127);
	WRITE_READ_TEST(std::uint8_t, 127);
	WRITE_READ_TEST(std::int16_t, 127);
	WRITE_READ_TEST(std::uint16_t, 127);
	WRITE_READ_TEST(std::int32_t, 127);
	WRITE_READ_TEST(std::uint32_t, 127);
	WRITE_READ_TEST(std::int64_t, 127);
	WRITE_READ_TEST(std::uint64_t, 127);
	
	READ_TYPE_ERROR_TEST(std::int8_t, 128);
	
	WRITE_READ_TEST(std::uint8_t, 128);
	WRITE_READ_TEST(std::int16_t, 128);
	WRITE_READ_TEST(std::uint16_t, 128);
	WRITE_READ_TEST(std::int32_t, 128);
	WRITE_READ_TEST(std::uint32_t, 128);
	WRITE_READ_TEST(std::int64_t, 128);
	WRITE_READ_TEST(std::uint64_t, 128);

	WRITE_READ_TEST(std::uint8_t, 255);
	WRITE_READ_TEST(std::int16_t, 255);
	WRITE_READ_TEST(std::uint16_t, 255);
	WRITE_READ_TEST(std::int32_t, 255);
	WRITE_READ_TEST(std::uint32_t, 255);
	WRITE_READ_TEST(std::int64_t, 255);
	WRITE_READ_TEST(std::uint64_t, 255);

	READ_TYPE_ERROR_TEST(std::uint8_t, 256);
	
	WRITE_READ_TEST(std::int16_t, 256);
	WRITE_READ_TEST(std::uint16_t, 256);
	WRITE_READ_TEST(std::int32_t, 256);
	WRITE_READ_TEST(std::uint32_t, 256);
	WRITE_READ_TEST(std::int64_t, 256);
	WRITE_READ_TEST(std::uint64_t, 256);

	WRITE_READ_TEST(std::int16_t, 32767);
	WRITE_READ_TEST(std::uint16_t, 32767);
	WRITE_READ_TEST(std::int32_t, 32767);
	WRITE_READ_TEST(std::uint32_t, 32767);
	WRITE_READ_TEST(std::int64_t, 32767);
	WRITE_READ_TEST(std::uint64_t, 32767);

	READ_TYPE_ERROR_TEST(std::int16_t, 32768);
	
	WRITE_READ_TEST(std::uint16_t, 32768);
	WRITE_READ_TEST(std::int32_t, 32768);
	WRITE_READ_TEST(std::uint32_t, 32768);
	WRITE_READ_TEST(std::int64_t, 32768);
	WRITE_READ_TEST(std::uint64_t, 32768);

	WRITE_READ_TEST(std::uint16_t, 65535);
	WRITE_READ_TEST(std::int32_t, 65535);
	WRITE_READ_TEST(std::uint32_t, 65535);
	WRITE_READ_TEST(std::int64_t, 65535);
	WRITE_READ_TEST(std::uint64_t, 65535);

	READ_TYPE_ERROR_TEST(std::uint16_t, 65536);
	
	WRITE_READ_TEST(std::int32_t, 65536);
	WRITE_READ_TEST(std::uint32_t, 65536);
	WRITE_READ_TEST(std::int64_t, 65536);
	WRITE_READ_TEST(std::uint64_t, 65536);

	WRITE_READ_TEST(std::int32_t, 2147483647);
	WRITE_READ_TEST(std::uint32_t, 2147483647);
	WRITE_READ_TEST(std::int64_t, 2147483647);
	WRITE_READ_TEST(std::uint64_t, 2147483647);

	READ_TYPE_ERROR_TEST(std::int32_t, 2147483648);
	
	WRITE_READ_TEST(std::uint32_t, 2147483648);
	WRITE_READ_TEST(std::int64_t, 2147483648);
	WRITE_READ_TEST(std::uint64_t, 2147483648);

	WRITE_READ_TEST(std::uint32_t, 4294967295);
	WRITE_READ_TEST(std::int64_t, 4294967295);
	WRITE_READ_TEST(std::uint64_t, 4294967295);

	READ_TYPE_ERROR_TEST(std::uint32_t, 4294967296LL);
	
	WRITE_READ_TEST(std::int64_t, 4294967296LL);
	WRITE_READ_TEST(std::uint64_t, 4294967296ULL);

	WRITE_READ_TEST(std::int64_t, 9223372036854775807ULL);
	WRITE_READ_TEST(std::uint64_t, 9223372036854775807ULL);

	READ_TYPE_ERROR_TEST(std::int64_t, 9223372036854775808ULL);
	
	WRITE_READ_TEST(std::uint64_t, 9223372036854775808ULL);
	
	WRITE_READ_TEST(std::uint64_t, 18446744073709551615ULL);
	
	WRITE_READ_TEST(std::int8_t, -1);
	WRITE_READ_TEST(std::int16_t, -1);
	WRITE_READ_TEST(std::int32_t, -1);
	WRITE_READ_TEST(std::int64_t, -1);

	WRITE_READ_TEST(std::int8_t, -32);
	WRITE_READ_TEST(std::int16_t, -32);
	WRITE_READ_TEST(std::int32_t, -32);
	WRITE_READ_TEST(std::int64_t, -32);

	WRITE_READ_TEST(std::int8_t, -33);
	WRITE_READ_TEST(std::int16_t, -33);
	WRITE_READ_TEST(std::int32_t, -33);
	WRITE_READ_TEST(std::int64_t, -33);

	WRITE_READ_TEST(std::int8_t, -128);
	WRITE_READ_TEST(std::int16_t, -128);
	WRITE_READ_TEST(std::int32_t, -128);
	WRITE_READ_TEST(std::int64_t, -128);

	READ_TYPE_ERROR_TEST(std::int8_t, -129);
	
	WRITE_READ_TEST(std::int16_t, -129);
	WRITE_READ_TEST(std::int32_t, -129);
	WRITE_READ_TEST(std::int64_t, -129);

	WRITE_READ_TEST(std::int16_t, -32768);
	WRITE_READ_TEST(std::int32_t, -32768);
	WRITE_READ_TEST(std::int64_t, -32768);

	READ_TYPE_ERROR_TEST(std::int16_t, -32769);
	
	WRITE_READ_TEST(std::int32_t, -32769);
	WRITE_READ_TEST(std::int64_t, -32769);

	WRITE_READ_TEST(std::int32_t, -2147483648);
	WRITE_READ_TEST(std::int64_t, -2147483648);

	READ_TYPE_ERROR_TEST(std::int32_t, -2147483649LL);
	
	WRITE_READ_TEST(std::int64_t, -2147483649LL);

	WRITE_READ_TEST(std::int64_t, std::numeric_limits<std::int64_t>::min());
}

template<class K, class V>
bool same_contents(std::unordered_map<K, V> const& a, std::unordered_map<K, V> const& b)
{
	for (auto ait = a.begin(); ait != a.end(); ++ait)
	{
		auto bit = b.find(ait->first);
		if (bit == b.end())
		{
			return false;
		}
		else
		{
			if (bit->second != ait->second)
			{
				return false;
			}
		}
	}
	return true;
}

TEST_CASE("nodeoze/smoke/bstream/unordered_map")
{
	{
		std::unordered_map<test_types::fee, test_types::fee> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fee, test_types::fee> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fee, test_types::fie> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fee, test_types::fie> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fee, test_types::foe> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fee, test_types::foe> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fee, test_types::fum> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fee, test_types::fum> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fie, test_types::fee> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fie, test_types::fee> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fie, test_types::fie> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fie, test_types::fie> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fie, test_types::foe> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fie, test_types::foe> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fie, test_types::fum> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fie, test_types::fum> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}


	{
		std::unordered_map<test_types::foe, test_types::fee> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::foe, test_types::fee> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::foe, test_types::fie> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::foe, test_types::fie> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::foe, test_types::foe> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::foe, test_types::foe> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::foe, test_types::fum> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::foe, test_types::fum> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}



	{
		std::unordered_map<test_types::fum, test_types::fee> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fum, test_types::fee> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fum, test_types::fie> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fum, test_types::fie> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fum, test_types::foe> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fum, test_types::foe> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::unordered_map<test_types::fum, test_types::fum> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_map<test_types::fum, test_types::fum> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
}

template<class K, class V>
bool same_contents(std::map<K, V> const& a, std::map<K, V> const& b)
{
	for (auto ait = a.begin(); ait != a.end(); ++ait)
	{
		auto bit = b.find(ait->first);
		if (bit == b.end())
		{
			return false;
		}
		else
		{
			if (bit->second != ait->second)
			{
				return false;
			}
		}
	}
	return true;
}

TEST_CASE("nodeoze/smoke/bstream/map")
{
	{
		std::map<test_types::fee, test_types::fee> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fee, test_types::fee> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fee, test_types::fie> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fee, test_types::fie> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fee, test_types::foe> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fee, test_types::foe> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fee, test_types::fum> map0;
		map0.emplace(test_types::fee{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fee, test_types::fum> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fie, test_types::fee> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fie, test_types::fee> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fie, test_types::fie> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fie, test_types::fie> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fie, test_types::foe> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fie, test_types::foe> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fie, test_types::fum> map0;
		map0.emplace(test_types::fie{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fie, test_types::fum> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}


	{
		std::map<test_types::foe, test_types::fee> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::foe, test_types::fee> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::foe, test_types::fie> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::foe, test_types::fie> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::foe, test_types::foe> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::foe, test_types::foe> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::foe, test_types::fum> map0;
		map0.emplace(test_types::foe{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::foe, test_types::fum> map1;
		is >> map1;
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}



	{
		std::map<test_types::fum, test_types::fee> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::fee{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fum, test_types::fee> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fum, test_types::fie> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::fie{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fum, test_types::fie> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fum, test_types::foe> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::foe{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fum, test_types::foe> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
	{
		std::map<test_types::fum, test_types::fum> map0;
		map0.emplace(test_types::fum{"zoot", 0}, test_types::fum{"arble", 100});
		bstream::obstream os{1024};
		os << map0;
		bstream::ibstream is{ os.get_buffer() };
		std::map<test_types::fum, test_types::fum> map1;
		is >> map1;
		CHECK(map0.size() == 1);
		CHECK(map1.size() == 1);
		CHECK(same_contents(map1, map0));
	}
}

TEST_CASE("nodeoze/smoke/bstream/vector")
{
	{
		std::vector<test_types::fee> vec0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << vec0;
		bstream::ibstream is{ os.get_buffer() };
		std::vector<test_types::fee> vec1;
		is >> vec1;
		CHECK(vec0 == vec1);
		is.rewind();
		std::vector<test_types::fee> vec2{is.read_as<std::vector<test_types::fee>>()};
		CHECK(vec0 == vec2);
	}
	{
		std::vector<test_types::fie> vec0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << vec0;
		bstream::ibstream is{ os.get_buffer() };
		std::vector<test_types::fie> vec1;
		is >> vec1;
		CHECK(vec0 == vec1);
		is.rewind();
		std::vector<test_types::fie> vec2{is.read_as<std::vector<test_types::fie>>()};
		CHECK(vec0 == vec2);
	}
	{
		std::vector<test_types::foe> vec0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << vec0;
		bstream::ibstream is{ os.get_buffer() };
		std::vector<test_types::foe> vec1;
		is >> vec1;
		CHECK(vec0 == vec1);
		is.rewind();
		std::vector<test_types::foe> vec2{is.read_as<std::vector<test_types::foe>>()};
		CHECK(vec0 == vec2);
	}
	{
		std::vector<test_types::fum> vec0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << vec0;
		bstream::ibstream is{ os.get_buffer() };
		std::vector<test_types::fum> vec1;
		is >> vec1;
		CHECK(vec0 == vec1);
		is.rewind();
		std::vector<test_types::fum> vec2{is.read_as<std::vector<test_types::fum>>()};
		CHECK(vec0 == vec2);
	}
}

TEST_CASE("nodeoze/smoke/bstream/deque")
{
	{
		std::deque<test_types::fee> deq0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << deq0;
		bstream::ibstream is{ os.get_buffer() };
		std::deque<test_types::fee> deq1;
		is >> deq1;
		CHECK(deq0 == deq1);
		is.rewind();
		std::deque<test_types::fee> deq2{is.read_as<std::deque<test_types::fee>>()};
		CHECK(deq0 == deq2);
	}
	{
		std::deque<test_types::fie> deq0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << deq0;
		bstream::ibstream is{ os.get_buffer() };
		std::deque<test_types::fie> deq1;
		is >> deq1;
		CHECK(deq0 == deq1);
		is.rewind();
		std::deque<test_types::fie> deq2{is.read_as<std::deque<test_types::fie>>()};
		CHECK(deq0 == deq2);
	}
	{
		std::deque<test_types::foe> deq0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << deq0;
		bstream::ibstream is{ os.get_buffer() };
		std::deque<test_types::foe> deq1;
		is >> deq1;
		CHECK(deq0 == deq1);
		is.rewind();
		std::deque<test_types::foe> deq2{is.read_as<std::deque<test_types::foe>>()};
		CHECK(deq0 == deq2);
	}
	{
		std::deque<test_types::fum> deq0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << deq0;
		bstream::ibstream is{ os.get_buffer() };
		std::deque<test_types::fum> deq1;
		is >> deq1;
		CHECK(deq0 == deq1);
		is.rewind();
		std::deque<test_types::fum> deq2{is.read_as<std::deque<test_types::fum>>()};
		CHECK(deq0 == deq2);
	}
}

TEST_CASE("nodeoze/smoke/bstream/forward_list")
{
	{
		std::forward_list<test_types::fee> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::forward_list<test_types::fee> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::forward_list<test_types::fee> obj2{is.read_as<std::forward_list<test_types::fee>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::forward_list<test_types::fie> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::forward_list<test_types::fie> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::forward_list<test_types::fie> obj2{is.read_as<std::forward_list<test_types::fie>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::forward_list<test_types::foe> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::forward_list<test_types::foe> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::forward_list<test_types::foe> obj2{is.read_as<std::forward_list<test_types::foe>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::forward_list<test_types::fum> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::forward_list<test_types::fum> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::forward_list<test_types::fum> obj2{is.read_as<std::forward_list<test_types::fum>>()};
		CHECK(obj0 == obj2);
	}
}

TEST_CASE("nodeoze/smoke/bstream/list")
{
	{
		std::list<test_types::fee> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::list<test_types::fee> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::list<test_types::fee> obj2{is.read_as<std::list<test_types::fee>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::list<test_types::fie> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::list<test_types::fie> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::list<test_types::fie> obj2{is.read_as<std::list<test_types::fie>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::list<test_types::foe> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::list<test_types::foe> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::list<test_types::foe> obj2{is.read_as<std::list<test_types::foe>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::list<test_types::fum> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::list<test_types::fum> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::list<test_types::fum> obj2{is.read_as<std::list<test_types::fum>>()};
		CHECK(obj0 == obj2);
	}
}

TEST_CASE("nodeoze/smoke/bstream/set")
{
	{
		std::set<test_types::fee> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::set<test_types::fee> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::set<test_types::fee> obj2{is.read_as<std::set<test_types::fee>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::set<test_types::fie> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::set<test_types::fie> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::set<test_types::fie> obj2{is.read_as<std::set<test_types::fie>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::set<test_types::foe> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::set<test_types::foe> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::set<test_types::foe> obj2{is.read_as<std::set<test_types::foe>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::set<test_types::fum> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::set<test_types::fum> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::set<test_types::fum> obj2{is.read_as<std::set<test_types::fum>>()};
		CHECK(obj0 == obj2);
	}
}

TEST_CASE("nodeoze/smoke/bstream/unordered_set")
{
	{
		std::unordered_set<test_types::fee> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_set<test_types::fee> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::unordered_set<test_types::fee> obj2{is.read_as<std::unordered_set<test_types::fee>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::unordered_set<test_types::fie> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_set<test_types::fie> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::unordered_set<test_types::fie> obj2{is.read_as<std::unordered_set<test_types::fie>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::unordered_set<test_types::foe> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_set<test_types::foe> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::unordered_set<test_types::foe> obj2{is.read_as<std::unordered_set<test_types::foe>>()};
		CHECK(obj0 == obj2);
	}
	{
		std::unordered_set<test_types::fum> obj0 = {{"silly", 0}, {"sully", 1}, {"sally", 2}, {"solly", 3}};
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		std::unordered_set<test_types::fum> obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		std::unordered_set<test_types::fum> obj2{is.read_as<std::unordered_set<test_types::fum>>()};
		CHECK(obj0 == obj2);
	}
}

TEST_CASE("nodeoze/smoke/bstream/tuple")
{
	{
		using tup_type = std::tuple<test_types::fee, test_types::fie, test_types::foe, test_types::fum>;

		CHECK(bstream::has_value_deserializer<tup_type>::value);
		CHECK(bstream::is_value_deserializable<tup_type>::value);
		CHECK(bstream::is_ref_deserializable<tup_type>::value);
		CHECK(std::is_move_assignable<tup_type>::value);
		CHECK(std::is_copy_assignable<tup_type>::value);
		tup_type obj0(test_types::fee{"silly", 0}, test_types::fie{"sully", 1}, test_types::foe{"sally", 2}, test_types::fum{"solly", 3});
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		tup_type obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		tup_type obj2{is.read_as<tup_type>()};
		CHECK(obj0 == obj2);
		
	}
}

TEST_CASE("nodeoze/smoke/bstream/class_write_read")
{
	{
		CHECK(!bstream::is_ibstream_constructible<test_types::fee>::value);
		CHECK(bstream::has_serialize_method<test_types::fee>::value);
		CHECK(bstream::has_deserialize_method<test_types::fee>::value);
		CHECK(bstream::has_serializer<test_types::fee>::value);
		CHECK(!bstream::has_value_deserializer<test_types::fee>::value);
		CHECK(bstream::has_ref_deserializer<test_types::fee>::value);
		CHECK(bstream::has_ibstream_extraction_operator<test_types::fee>::value);
		CHECK(bstream::has_obstream_insertion_operator<test_types::fee>::value);
		
		CHECK(std::is_move_assignable<test_types::fee>::value);
		test_types::fee obj0{"the answer is", 42}; 
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		test_types::fee obj1;
		is >> obj1;
		CHECK(obj1 == obj0);
		is.rewind();
		test_types::foe obj2(is.read_as<test_types::foe>());
		CHECK(obj2 == obj0);
	}
	{
		CHECK(!bstream::is_ibstream_constructible<test_types::fie>::value);
		CHECK(!bstream::has_serialize_method<test_types::fie>::value);
		CHECK(!bstream::has_deserialize_method<test_types::fie>::value);
		CHECK(bstream::has_serializer<test_types::fie>::value);
		CHECK(bstream::has_value_deserializer<test_types::fie>::value);
		CHECK(!bstream::has_ref_deserializer<test_types::fie>::value);
		CHECK(bstream::has_ibstream_extraction_operator<test_types::fie>::value);
		CHECK(bstream::has_obstream_insertion_operator<test_types::fie>::value);
		
		test_types::fie obj0{"the answer is", 42}; 
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
//		test_types::fie obj1{bstream::ibstream_initializer<test_types::fie>::get(is)};
		test_types::fie obj1;
		is >> obj1;
		CHECK(obj1 == obj0);
		is.rewind();
		test_types::foe obj2(is.read_as<test_types::foe>());
		CHECK(obj0 == obj2);
	}
	{
		CHECK(!bstream::is_ibstream_constructible<test_types::foe>::value);
		CHECK(!bstream::has_serialize_method<test_types::foe>::value);
		CHECK(!bstream::has_deserialize_method<test_types::foe>::value);
		CHECK(bstream::has_serializer<test_types::foe>::value);
		CHECK(!bstream::has_value_deserializer<test_types::foe>::value);
		CHECK(bstream::has_ref_deserializer<test_types::foe>::value);
		CHECK(bstream::has_ibstream_extraction_operator<test_types::foe>::value);
		CHECK(bstream::has_obstream_insertion_operator<test_types::foe>::value);
		
		test_types::foe obj0{"the answer is", 42}; 
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		test_types::foe obj1;
		is >> obj1;
		CHECK(obj1 == obj0);
		is.rewind();
		test_types::foe obj2(is.read_as<test_types::foe>());
		CHECK(obj0 == obj2);
	}
	{
		CHECK(bstream::is_ibstream_constructible<test_types::fum>::value);
		CHECK(bstream::has_serialize_method<test_types::fum>::value);
		CHECK(!bstream::has_deserialize_method<test_types::fum>::value);
		CHECK(bstream::has_serializer<test_types::fum>::value);
		CHECK(bstream::has_value_deserializer<test_types::fum>::value);
		CHECK(!bstream::has_ref_deserializer<test_types::fum>::value);
		CHECK(bstream::has_ibstream_extraction_operator<test_types::fum>::value);
		CHECK(bstream::has_obstream_insertion_operator<test_types::fum>::value);

		test_types::fum obj0{"the answer is", 42}; 
		bstream::obstream os{1024};
		os << obj0;
		bstream::ibstream is{ os.get_buffer() };
		test_types::fum obj1;
		is >> obj1;
		CHECK(obj1 == obj0);
		is.rewind();
		test_types::fum obj2(is.read_as<test_types::fum>());
		CHECK(obj0 == obj2);
	}
	{
		CHECK(bstream::is_ibstream_constructible<test_types::fum>::value);
		CHECK(bstream::has_serialize_method<test_types::fum>::value);
		CHECK(!bstream::has_deserialize_method<test_types::fum>::value);
		CHECK(bstream::has_serializer<test_types::fum>::value);
		CHECK(bstream::has_value_deserializer<test_types::fum>::value);
		CHECK(!bstream::has_ref_deserializer<test_types::fum>::value);
		CHECK(bstream::has_ibstream_extraction_operator<test_types::fum>::value);
		CHECK(bstream::has_obstream_insertion_operator<test_types::fum>::value);

		test_types::foo obj0{test_types::fee{"shamma", 0}, test_types::fie{"lamma", 1}, 
				test_types::foe{"ding", 2}, test_types::fum{"dong", 3}, "ooo mau mau" }; 
		bstream::obstream os{1024};
		os << obj0;
//		bstream::dump_json(std::cout, os);
		bstream::ibstream is{ os.get_buffer() };
		test_types::foo obj1;
		is >> obj1;
		CHECK(obj0 == obj1);
		is.rewind();
		test_types::foo obj2(is.read_as<test_types::foo>());
		CHECK(obj0 == obj2);
	}
}

namespace test_types
{
	struct fou : public simple_state // serializer/value_deserializer/move ctor
	{
		fou() = default;
		fou(std::string const& nstr, int val) : simple_state{nstr, val} {}
		fou(fou const&) = delete;
		fou(fou&&) = default;
		fou& operator=(fou&& other) = default;
		fou& operator=(fou const& other) = default;
	};
}

namespace std 
{
	template<> struct hash<test_types::fou>  
	{
		typedef test_types::fou argument_type;
		typedef std::hash<std::string>::result_type result_type;
		result_type operator()( const argument_type& arg ) const  {
			std::hash<std::string> hasher;
			return hasher(arg.name);
		}  
	}; 
	
	template<> struct less<test_types::fou>
	{

		typedef test_types::fou first_argument_type;
		typedef test_types::fou second_argument_type;
		typedef bool result_type;
		result_type operator()(const first_argument_type& x, const second_argument_type& y) const
		{
			return x.name < y.name;
		}
	};
}

template<>
struct bstream::serializer<test_types::fou>
{
	static inline bstream::obstream& put(bstream::obstream& os, test_types::fou const& obj)
	{
		os.write_array_header(2);
		os << obj.name << obj.value;
		return os;
	}
};

template<>
struct bstream::value_deserializer<test_types::fou>
{
	static inline test_types::fou get(bstream::ibstream& is)
	{
		is.check_array_header(2);
		auto s = is.read_as<std::string>();
		auto v = is.read_as<int>();
		return test_types::fou(s, v);
	}
};

namespace test_types
{
	struct flu : public simple_state // serializer/ref_deserializer/default ctor/copy ctor
	{
		flu() {}
		flu(std::string const& nstr, int val) : simple_state{nstr, val} {}
		flu(flu const&) = default;
		flu(flu&&) = default;
		flu& operator=(flu&& other) = default;
		flu& operator=(flu const& other) = default;		
	};	
}

namespace std 
{
	template<> struct hash<test_types::flu>  
	{
		typedef test_types::flu argument_type;
		typedef std::hash<std::string>::result_type result_type;
		result_type operator()( const argument_type& arg ) const  {
			std::hash<std::string> hasher;
			return hasher(arg.name);
		}  
	};
	
	template<> struct less<test_types::flu>
	{

		typedef test_types::flu first_argument_type;
		typedef test_types::flu second_argument_type;
		typedef bool result_type;
		result_type operator()(const first_argument_type& x, const second_argument_type& y) const
		{
			return x.name < y.name;
		}
	};
}

template<>
struct bstream::serializer<test_types::flu>
{
	static inline bstream::obstream& put(bstream::obstream& os, test_types::flu const& obj)
	{
		os.write_array_header(2);
		os << obj.name << obj.value;
		return os;
	}
};
template<>
struct bstream::ref_deserializer<test_types::flu>
{
	static inline bstream::ibstream& get(bstream::ibstream& is, test_types::flu& obj)
	{
		is.check_array_header(2);
		is >> obj.name >> obj.value;
		return is;
	}
};

namespace test_types
{
	struct fox : public simple_state // serializer/ref_deserializer/default ctor/move ctor
	{
		fox() {}
		fox(std::string const& nstr, int val) : simple_state{nstr, val} {}
		fox(fox const&) = delete;
		fox(fox&&) = default;
		fox& operator=(fox&& other) = default;
		fox& operator=(fox const& other) = default;		
	};	
}

namespace std 
{
	template<> struct hash<test_types::fox>  
	{
		typedef test_types::fox argument_type;
		typedef std::hash<std::string>::result_type result_type;
		result_type operator()( const argument_type& arg ) const  {
			std::hash<std::string> hasher;
			return hasher(arg.name);
		}  
	};
	
	template<> struct less<test_types::fox>
	{

		typedef test_types::fox first_argument_type;
		typedef test_types::fox second_argument_type;
		typedef bool result_type;
		result_type operator()(const first_argument_type& x, const second_argument_type& y) const
		{
			return x.name < y.name;
		}
	};
}

template<>
struct bstream::serializer<test_types::fox>
{
	static inline bstream::obstream& put(bstream::obstream& os, test_types::fox const& obj)
	{
		os.write_array_header(2);
		os << obj.name << obj.value;
		return os;
	}
};
template<>
struct bstream::ref_deserializer<test_types::fox>
{
	static inline bstream::ibstream& get(bstream::ibstream& is, test_types::fox& obj)
	{
		is.check_array_header(2);
		is >> obj.name >> obj.value;
		return is;
	}
};

TEST_CASE("nodeoze/smoke/bstream/ptr")
{
	{
		bstream::obstream os{1024};

		std::shared_ptr<test_types::fee> objp0 = std::make_shared<test_types::fee>("shamma", 0);

		os << objp0;
		
		bstream::ibstream is{ os.get_buffer() };

		std::shared_ptr<test_types::fee> objp1;

		is >> objp1;

		CHECK(*objp0 == *objp1);
		
		is.rewind();
		
		std::shared_ptr<test_types::fee> objp2 = is.read_as<std::shared_ptr<test_types::fee>>();
		
		CHECK(*objp0 == *objp2);
	}
	{
		bstream::obstream os{1024};

		std::unique_ptr<test_types::fee> objp0 = std::make_unique<test_types::fee>("shamma", 0);

		os << objp0;

		bstream::ibstream is{ os.get_buffer() };

		std::unique_ptr<test_types::fee> objp1;

		is >> objp1;

		CHECK(*objp0 == *objp1);
		
		is.rewind();
		
		std::unique_ptr<test_types::fee> objp2 = is.read_as<std::unique_ptr<test_types::fee>>();
		
		CHECK(*objp0 == *objp2);
	}
	{
		bstream::obstream os{1024};

		std::unique_ptr<test_types::fou> objp0 = std::make_unique<test_types::fou>("shamma", 0);

		os << objp0;

		bstream::ibstream is{ os.get_buffer() };

		std::unique_ptr<test_types::fou> objp1;

		is >> objp1;

		CHECK(*objp0 == *objp1);
		
		is.rewind();
		
		std::unique_ptr<test_types::fou> objp2 = is.read_as<std::unique_ptr<test_types::fou>>();
		
		CHECK(*objp0 == *objp2);
	}
	{
		bstream::obstream os{1024};

		std::unique_ptr<test_types::flu> objp0 = std::make_unique<test_types::flu>("shamma", 0);

		os << objp0;

		bstream::ibstream is{ os.get_buffer() };

		std::unique_ptr<test_types::flu> objp1;

		is >> objp1;

		CHECK(*objp0 == *objp1);
		
		is.rewind();
		
		std::unique_ptr<test_types::flu> objp2 = is.read_as<std::unique_ptr<test_types::flu>>();
		
		CHECK(*objp0 == *objp2);
	}
	{
		bstream::obstream_cntxt os{1024};
		std::shared_ptr<test_types::fee> objp0 = std::make_shared<test_types::fee>("shamma", 0);
		auto objp0_copy = objp0;
		os << objp0 << objp0_copy;
		bstream::ibstream_cntxt is{ os.get_buffer() };
		std::shared_ptr<test_types::fee> objp1;
		is >> objp1;
		CHECK(*objp0 == *objp1);
		std::shared_ptr<test_types::fee> objp2 = is.read_as<std::shared_ptr<test_types::fee>>();
		CHECK(objp1 == objp2);
	}
	{
		bstream::obstream_cntxt os{1024};
		std::shared_ptr<test_types::fee> objp0 = std::make_shared<test_types::fee>("shamma", 0);
		os << objp0;
		bstream::ibstream_cntxt is{ os.get_buffer() };
		std::shared_ptr<test_types::fee> objp1;
		is >> objp1;
		CHECK(*objp0 == *objp1);
		is.rewind();
		std::shared_ptr<test_types::fee> objp2 = is.read_as<std::shared_ptr<test_types::fee>>();
		CHECK(objp1 != objp2);
	}
}

TEST_CASE("nodeoze/smoke/bstream/initializer")
{
	{
		/*
		 *	fou has value_deserializer, move ctor
		 */
		bstream::obstream_cntxt os{1024};
		test_types::fou objp0{"shamma", 0};
		os << objp0;
		bstream::ibstream_cntxt is{ os.get_buffer() };
		test_types::fou objp1{bstream::ibstream_initializer<test_types::fou>::get(is)};
		CHECK(objp0 == objp1);
	}

	{
		/*
		 *	flu has ref_deserializer, copy ctor
		 */
		bstream::obstream_cntxt os{1024};
		test_types::flu objp0{"shamma", 0};
		os << objp0;
		bstream::ibstream_cntxt is{ os.get_buffer() };
		test_types::flu objp1{bstream::ibstream_initializer<test_types::flu>::get(is)};
		CHECK(objp0 == objp1);
	}
	{
		/*
		 *	fox has ref_deserializer, move ctor
		 */
		bstream::obstream_cntxt os{1024};
		test_types::fox objp0{"shamma", 0};
		os << objp0;
		bstream::ibstream_cntxt is{ os.get_buffer() };
		test_types::fox objp1{bstream::ibstream_initializer<test_types::fox>::get(is)};
		CHECK(objp0 == objp1);
	}
}

TEST_CASE("nodeoze/smoke/bstream/pair")
{
	{
		bstream::obstream os{1024};
		using pair_type = std::pair<test_types::fee, test_types::fie>;
		test_types::fee fee1{"shamma", 0};
		test_types::fie fie1{"lamma", 1};
		pair_type p0{fee1, fie1};
		
		os << p0;
		
		bstream::ibstream is{ os.get_buffer() };
		
		pair_type p1;
		
		is >> p1;
		CHECK(p0 == p1);
		is.rewind();
		pair_type p2{is.read_as<pair_type>()};
		CHECK(p0 == p2);
	}
}

