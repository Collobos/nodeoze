/*
 * Copyright (c) 2013-2017, Collobos Software Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
 
#ifndef _nodeoze_location_h
#define _nodeoze_location_h

#include <nodeoze/promise.h>
#include <nodeoze/any.h>
#include <nodeoze/json.h>

namespace nodeoze {

class location
{
public:

	static promise< location >
	from_ip_address();

	inline location()
	{
	}

	inline location( double latitude, double longitude, double altitude, double size, double horizontal_accuracy, double vertical_accuracy )
	:
		m_latitude( latitude ),
		m_longitude( longitude ),
		m_altitude( altitude ),
		m_size( size ),
		m_horizontal_accuracy( horizontal_accuracy ),
		m_vertical_accuracy( vertical_accuracy )
	{
	}
	
	inline location( const nodeoze::any &root )
	{
		operator=( root );
	}
	
	inline location( const location &rhs )
	:
		m_latitude( rhs.m_latitude ),
		m_longitude( rhs.m_longitude ),
		m_altitude( rhs.m_altitude ),
		m_size( rhs.m_size ),
		m_horizontal_accuracy( rhs.m_horizontal_accuracy ),
		m_vertical_accuracy( rhs.m_vertical_accuracy )
	{
	}
	
	inline ~location()
	{
	}
	
	inline location&
	operator=( const nodeoze::any &root )
	{
		m_latitude				= root[ "latitude" ].to_floating();
		m_longitude				= root[ "longitude" ].to_floating();
		m_altitude				= root[ "altitude" ].to_floating();
		m_size					= root[ "size" ].to_floating();
		m_horizontal_accuracy	= root[ "horizontal_accuracy" ].to_floating();
		m_vertical_accuracy		= root[ "vertical_accuracy" ].to_floating();
		
		return *this;
	}
	
	inline location&
	operator=( const location &rhs )
	{
		m_latitude				= rhs.m_latitude;
		m_longitude				= rhs.m_longitude;
		m_altitude				= rhs.m_altitude;
		m_size					= rhs.m_size;
		m_horizontal_accuracy	= rhs.m_horizontal_accuracy;
		m_vertical_accuracy		= rhs.m_vertical_accuracy;
		
		return *this;
	}
	
	inline bool
	operator==( const location &rhs ) const
	{
		return ( ( m_latitude == rhs.m_latitude ) && ( m_longitude == rhs.m_longitude ) && ( m_altitude == rhs.m_altitude ) );
	}
	
	inline bool
	operator!=( const location &rhs ) const
	{
		return ( ( m_latitude != rhs.m_latitude ) || ( m_longitude != rhs.m_longitude ) || ( m_altitude != rhs.m_altitude ) );
	}
	
	inline double
	latitude_degrees() const
	{
		return m_latitude;
	}
	
	inline void
	latitude_degrees( std::int32_t &degrees, std::int32_t &minutes, double &seconds )
	{
		to_integer( m_latitude, degrees, minutes, seconds );
	}
	
	inline void
	set_latitude_degrees( double val )
	{
		m_latitude = val;
	}
	
	inline double
	longitude_degrees() const
	{
		return m_longitude;
	}
	
	inline void
	longitude_degrees( std::int32_t &degrees, std::int32_t &minutes, double &seconds )
	{
		to_integer( m_longitude, degrees, minutes, seconds );
	}
	
	inline void
	set_longitude_degrees( double val )
	{
		m_longitude = val;
	}
	
	inline double
	altitude_meters() const
	{
		return m_altitude;
	}
	
	inline void
	set_altitude_meters( double val )
	{
		m_altitude = val;
	}
	
	inline double
	size_meters() const
	{
		return m_size;
	}
	
	inline void
	set_size_meters( double val )
	{
		m_size = val;
	}
	
	inline double
	horizontal_accuracy_meters() const
	{
		return m_horizontal_accuracy;
	}
	
	inline void
	set_horizontal_accuracy_meters( double val )
	{
		m_horizontal_accuracy = val;
	}
	
	inline double
	vertical_accuracy_meters() const
	{
		return m_vertical_accuracy;
	}
	
	inline void
	set_vertical_accuracy_meters( double val )
	{
		m_vertical_accuracy = val;
	}
	
	inline std::string
	to_string() const
	{
		return nodeoze::json::deflate_to_string( to_any() );
	}
	
	inline nodeoze::any
	to_any() const
	{
		nodeoze::any root;
		
		root[ "latitude" ]				= m_latitude;
		root[ "longitude" ]				= m_longitude;
		root[ "altitude" ]				= m_altitude;
		root[ "size" ]					= m_size;
		root[ "horizontal_accuracy" ]	= m_horizontal_accuracy;
		root[ "vertical_accuracy" ]		= m_vertical_accuracy;
		
		return root;
	}
	
	explicit operator bool () const
	{
		return ( m_latitude || m_longitude || m_altitude ) ? true : false;
	}
	
	inline static bool
	integrity_check( const any& root )
	{
		return
			root.is_object()
			&& root.is_member( "latitude" )				&& root[ "latitude" ].is_floating()
			&& root.is_member( "longitude" )			&& root[ "longitude" ].is_floating()
			&& root.is_member( "altitude" )				&& root[ "altitude" ].is_floating()
			&& root.is_member( "size" )					&& root[ "size" ].is_floating()
			&& root.is_member( "horizontal_accuracy" )	&& root[ "horizontal_accuracy" ].is_floating()
			&& root.is_member( "vertical_accuracy" )	&& root[ "vertical_accuracy" ].is_floating();
	}
	
private:

	inline void
	safe_bool_func()
	{
	}

	void
	to_integer( double ang, std::int32_t &degrees, std::int32_t &minutes, double &seconds )
	{
		bool neg = false;
		
		if ( ang < 0.0 )
		{
			neg = true ;
			ang = -ang ;
		}
  
		degrees = static_cast< std::int32_t >( ang );
		double frac = ang - static_cast< double >( degrees );

		frac *= 60.0;

		minutes = static_cast< std::int32_t >( frac );

		frac = frac - static_cast< double >( minutes );

		seconds = std::nearbyint( frac * 600000.0 );
		seconds /= 10000.0 ;

		if ( seconds >= 60.0 )
		{
			minutes++;
			seconds -= 60.0;
		}
	}

	double m_latitude				= 0.0;
	double m_longitude				= 0.0;
	double m_altitude				= 0.0;
	double m_size					= 0.0;
	double m_horizontal_accuracy	= 0.0;
	double m_vertical_accuracy		= 0.0;
};

}

#endif

