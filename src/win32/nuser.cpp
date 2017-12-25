#include <nodeoze/win32/nuser.h>
#include <nodeoze/nlog.h>
#include <LMCons.h>
#include <Sddl.h>

using namespace nodeoze;

#pragma comment( lib, "userenv.lib" )

windows::user::token::token( HANDLE native )
:
	m_native( nullptr )
{
	DWORD				len				= 0;
	std::vector<BYTE>	buffer;
    PTOKEN_USER			user			= nullptr;
	DWORD				username_len	= 0;
	DWORD				domain_len		= 0;
	LPTSTR				str				= nullptr;
	SID_NAME_USE		snu;

	if ( !GetTokenInformation( native, TokenUser, NULL, 0, &len ) && ( GetLastError() != ERROR_INSUFFICIENT_BUFFER ) )
    {
		nlog( log::level_t::error, "GetTokenInformation() failed: %", ::GetLastError() );
		goto exit;
    }

	buffer.resize( len );

	if ( !GetTokenInformation( native, TokenUser, buffer.data(), len, &len ) ) 
    {
		nlog( log::level_t::error, "GetTokenInformation() failed: %", ::GetLastError() );
		goto exit;
	}

	user = reinterpret_cast< PTOKEN_USER >( buffer.data() );

	if ( !IsValidSid( user->User.Sid ) ) 
	{
		nlog( log::level_t::error, "IsValidSid() failed: %", ::GetLastError() );
		goto exit;
	}

	if ( !ConvertSidToStringSid( user->User.Sid, &str ) )
	{
		nlog( log::level_t::error, "ConvertSidToStringSid() failed: %", ::GetLastError() );
		goto exit;
	}

	m_sid = narrow( str );

	username_len	= 0;
	domain_len		= 0;

	if ( !LookupAccountSid( nullptr, user->User.Sid, nullptr, &username_len, nullptr, &domain_len, &snu ) && ( ::GetLastError() != ERROR_INSUFFICIENT_BUFFER ) )
	{
		nlog( log::level_t::error, "LookupAccountSid() failed: %", ::GetLastError() );
		goto exit;
	}

	m_username.resize( username_len, 0 );
	m_domain.resize( domain_len, 0 );

	if ( !LookupAccountSid( nullptr, user->User.Sid, ( LPTSTR ) m_username.data(), &username_len, ( LPTSTR ) m_domain.data(), &domain_len, &snu ) )
	{
		nlog( log::level_t::error, "LookupAccountSid() failed: %", ::GetLastError() );
		goto exit;
	}

	m_native	= native;
	native		= nullptr;

	nlog( log::level_t::verbose, "handle %", m_native );

exit:

	if ( str )
	{
		LocalFree( str );
	}

	if ( native )
	{
		CloseHandle( native );
	}
}


windows::user::token::~token()
{
	if ( m_native )
	{
		nlog( log::level_t::verbose, "closing handle %", m_native );
		CloseHandle( m_native );
	}
}


windows::user::token::ptr
windows::user::token::duplicate( DWORD desired_access, LPSECURITY_ATTRIBUTES attributes, SECURITY_IMPERSONATION_LEVEL level, TOKEN_TYPE type )
{
	HANDLE			native;
	token::ptr dup;

	if ( !DuplicateTokenEx( m_native, desired_access, attributes, level, type, &native ) )
	{
		nlog( log::level_t::error, "DuplicateTokenEx() failed: %", ::GetLastError() );
		goto exit;
	}

	dup = std::make_shared< token >( native );

	if ( !dup->is_valid() )
	{
		nlog( log::level_t::error, "dup->is_valid() failed" );
		dup = nullptr;
	}

exit:

	return dup;
}


windows::user::profile::profile( const windows::user::token::ptr &token )
:
	m_token( token ),
	m_username( m_token->username() )
{
	assert( m_token );

	load();
}


windows::user::profile::profile( const windows::user::token::ptr &token, const std::string &username )
:
	m_token( token ),
	m_username( username )
{
	assert( m_token );

	load();
}


void
windows::user::profile::load()
{
	ZeroMemory( &m_info, sizeof( PROFILEINFO ) );

	m_info.dwSize		= sizeof( PROFILEINFO );
	m_info.dwFlags		= PI_NOUI;
	m_info.lpUserName	= ( LPWSTR ) m_username.c_str();

	if ( !LoadUserProfile( m_token->native(), &m_info ) )
	{
		nlog( log::level_t::error, "LoadUserProfile() failed: %", ::GetLastError() );
		ZeroMemory( &m_info, sizeof( PROFILEINFO ) );
	}

	nlog( log::level_t::verbose, "profile for user % is loaded", m_token->username() );
}


windows::user::profile::~profile()
{
	if ( is_valid() )
	{
		if ( UnloadUserProfile( m_token->native(), m_info.hProfile ) )
		{
			nlog( log::level_t::verbose, "profile for user % is unloaded", m_username );
		}
		else
		{
			nlog( log::level_t::warning, "UnloadUserProfile(%) failed: %", m_username, ::GetLastError() );
		}
	}
}
