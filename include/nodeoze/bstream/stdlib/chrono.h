#ifndef NODEOZE_BSTREAM_STDLIB_CHRONO_H
#define NODEOZE_BSTREAM_STDLIB_CHRONO_H

#include <nodeoze/bstream/ibstream.h>
#include <nodeoze/bstream/obstream.h>
#include <chrono>

namespace nodeoze
{
namespace bstream
{

template< class Rep, class Ratio >
struct value_deserializer< std::chrono::duration< Rep,Ratio > >
{
    using duration_type = std::chrono::duration< Rep,Ratio >;

    duration_type 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static duration_type
    get( ibstream&is )
    {
        auto count = is.read_as< duration_type::rep >();
        return duration_type{ count };
    }
};

template< class Clock, class Duration >
struct value_deserializer< std::chrono::time_point< Clock,Duration > >
{
    using time_point_type = std::chrono::time_point< Clock,Duration >;

    time_point_type 
    operator()( ibstream& is )  const
    {
        return get( is );
    }

    static time_point_type
    get( ibstream& is )
    {
        auto ticks = is.read_as< typename time_point_type::rep >();
        return time_point_type( typename time_point_type::duration( ticks ) );
    }
};

template< class Rep, class Ratio >
struct serializer< std::chrono::duration< Rep,Ratio > >
{
	using duration_type = std::chrono::duration< Rep,Ratio >;
	static obstream& put( obstream& os, duration_type val )
	{
		os << val.count();
		return os;
	}
};

template< class Clock, class Duration >
struct serializer< std::chrono::time_point< Clock,Duration > >
{
	using time_point_type = std::chrono::time_point< Clock,Duration >;
	static obstream& put( obstream& os, time_point_type val )
	{
		os << val.time_since_epoch().count();
		return os;
	}
};

} // bstream
} // nodeoze

#endif // NODEOZE_BSTREAM_STDLIB_CHRONO_H