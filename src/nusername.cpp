#include <nodeoze/nusername.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nlog.h>
#if defined( WIN32 )
#	include <WinSock2.h>
#	include <DSRole.h>
#endif

using namespace nodeoze;

void
username::assign( const std::string &name, const std::string &domain )
{
	auto pos = name.find( '@' );

	if ( pos != std::string::npos )
	{
		m_name	= name.substr( 0, pos );
		m_domains.emplace_back(	name.substr( pos + 1, name.size() - pos + 1 ) );
	}
	else if ( ( pos = name.find ( '\\' ) ) != std::string::npos )
	{
		m_name	= name.substr( pos + 1, name.size() - pos + 1 );
		m_domains.emplace_back(	name.substr( 0, pos ) );
	}
	else if ( domain.size() > 0 )
	{
		m_name	= name;
		m_domains.emplace_back(	domain );
	}
	else
	{
		m_name = name;
		
#if defined( WIN32 )

		static std::string	network_domain;
		static bool			init = false;
		
		if ( !init )
		{
			init = true;
			
			DSROLE_PRIMARY_DOMAIN_INFO_BASIC	*info;
			DWORD								dw;

			dw = DsRoleGetPrimaryDomainInformation( NULL, DsRolePrimaryDomainInfoBasic, ( PBYTE* ) &info );
			ncheck_error( dw == ERROR_SUCCESS, exit, "DsRoleGetPrimaryDomainInformation() failed (%)", ::GetLastError() );

			if ( info->DomainNameDns )
			{
				network_domain = narrow( info->DomainNameDns );
			}
		}
		
		if ( network_domain.size() > 0 )
		{
			m_domains.emplace_back( network_domain );
		}
#endif

		m_domains.emplace_back( "." );
	}

#if defined( WIN32 )

exit:

	return;
#endif
}

