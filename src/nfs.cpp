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

#include <nodeoze/nfs.h>
#include <nodeoze/nunicode.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nlog.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined( WIN32 )
#	include <io.h>
#endif


using namespace nodeoze;

fs::~fs()
{
}


fs::marker
fs::open( const path &p, const char *mode )
{
	FILE *fp;
	
	fp = fopen( p.to_string().c_str(), mode );
	
	if ( !fp )
	{
		goto exit;
	}
	
exit:

	return std::shared_ptr< void >( fp, []( void *data )
	{
		FILE *fp = reinterpret_cast< FILE* >( data );

		if ( fp )
		{
			fclose( fp );
		}
	} );
}


fs::marker
fs::create_only( const path &p )
{
	FILE	*fp = nullptr;
	int		fd;

	fd = ::open( p.to_string().c_str(), O_CREAT|O_WRONLY|O_TRUNC|O_EXCL, 0660 );

	ncheck_error( fd != -1, exit, "exclusive open failed" );

#if defined( WIN32 )
	fp = ::_fdopen( fd, "wb" );
#else
	fp = ::fdopen( fd, "wb" );
#endif

exit:
	
	return std::shared_ptr< void >( fp, []( void *data )
	{
		FILE *fp = reinterpret_cast<FILE*>(data);

		if (fp)
		{
			fclose( fp );
		}
	} );
}
	


void
fs::write( fs::marker marker, const std::uint8_t *buf, std::size_t len )
{
	assert( marker );
	
	FILE *fp = reinterpret_cast< FILE* >( marker.get() );
	
	if ( fp )
	{
		fwrite( buf, 1, len, fp );
	}
}


void
fs::read( fs::marker marker, read_f func )
{
	const std::size_t	buf_len	= 32767;
	std::uint8_t		buf[ buf_len ];
	std::streamsize		bytes;
	FILE				*fp = reinterpret_cast< FILE* >( marker.get() );
	
	while ( ( bytes = fread( buf, 1, buf_len, fp ) ) > 0 )
	{
		func( buf, bytes );
	}
}
