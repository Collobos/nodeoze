#include "nkeychain_win32.h"
#include <nodeoze/nunicode.h>
#include <nodeoze/nbase64.h>
#include <nodeoze/nmacros.h>
#include <nodeoze/nlog.h>
#include <WinSock2.h>
#include <Windows.h>
#include <WinCrypt.h>
#include <string>
#include <NTSecAPI.h>
#include <lm.h>

using namespace nodeoze;

static std::error_code
MakeLsaStringFromUTF8String( PLSA_UNICODE_STRING output, const std::string &value );

static std::error_code
MakeUTF8StringFromLsaString( PLSA_UNICODE_STRING input, std::string &value );

NODEOZE_DEFINE_SINGLETON( keychain )

keychain*
keychain::create()
{
	return new keychain_win32;
}


std::error_code
keychain_win32::save( const uri &resource, const std::string &value )
{
	LSA_OBJECT_ATTRIBUTES	attrs;
	LSA_HANDLE				handle = NULL;
	NTSTATUS				res;
	LSA_UNICODE_STRING		lucResourceName;
	LSA_UNICODE_STRING		lucSecretName;
	BOOL					ok = TRUE;
	std::error_code			err;

	// attrs are reserved, so initialize to zeroes.

	ZeroMemory( &attrs, sizeof( attrs ) );

	// Get a handle to the Policy object on the local system

	res = LsaOpenPolicy( NULL, &attrs, POLICY_ALL_ACCESS, &handle );
	ncheck_error_action_quiet( res == ERROR_SUCCESS, err = make_error_code( err_t::internal_error ), exit );

	// Intializing PLSA_UNICODE_STRING structures

	err = MakeLsaStringFromUTF8String( &lucResourceName, resource.to_string() );
	ncheck_error_quiet( !err, exit );

	err = MakeLsaStringFromUTF8String( &lucSecretName, value );
	ncheck_error_quiet( !err, exit );

	// Store the private data.

	res = LsaStorePrivateData( handle, &lucResourceName, &lucSecretName );
	ncheck_error_action_quiet( res == ERROR_SUCCESS, err = make_error_code( err_t::internal_error ), exit );

exit:

	if ( handle )
	{
		LsaClose( handle );
		handle = NULL;
	}

	return err;
}


std::error_code
keychain_win32::find( const uri &resource, std::string &value )
{
	PLSA_UNICODE_STRING		resourceLSA	= nullptr;
	PLSA_UNICODE_STRING		secretLSA	= nullptr;
	LSA_OBJECT_ATTRIBUTES	attrs;
	LSA_HANDLE				handle = NULL;
	NTSTATUS				res;
	auto					err			= std::error_code();

	// attrs are reserved, so initialize to zeroes.

	ZeroMemory( &attrs, sizeof( attrs ) );

	// Get a handle to the Policy object on the local system

	res = LsaOpenPolicy( NULL, &attrs, POLICY_GET_PRIVATE_INFORMATION, &handle );
	ncheck_error_action_quiet( res == ERROR_SUCCESS, err = make_error_code( err_t::internal_error ), exit );

	// Get the encrypted data

	resourceLSA = ( PLSA_UNICODE_STRING ) malloc( sizeof( LSA_UNICODE_STRING ) );
	ncheck_error_action_quiet( resourceLSA != NULL, err = make_error_code( err_t::no_memory ), exit );
	err = MakeLsaStringFromUTF8String( resourceLSA, resource.to_string().c_str() );
	ncheck_error_quiet( !err, exit );

	// Retrieve the key

	res = LsaRetrievePrivateData( handle, resourceLSA, &secretLSA );
	ncheck_error_action_quiet( res == ERROR_SUCCESS, err = make_error_code( err_t::not_exist ), exit );

	err = MakeUTF8StringFromLsaString( secretLSA, value );
	ncheck_error_quiet( !err, exit );

exit:

	if ( resourceLSA != NULL )
	{
		if ( resourceLSA->Buffer != NULL )
		{
			free( resourceLSA->Buffer );
		}

		free( resourceLSA );
	}

	if ( secretLSA != NULL )
	{
		LsaFreeMemory( secretLSA );
	}

	if ( handle )
	{
		LsaClose( handle );
		handle = NULL;
	}

	return err;
}


static std::error_code
MakeLsaStringFromUTF8String( PLSA_UNICODE_STRING output, const std::string &input )
{
	int		size;
	auto	err = std::error_code();
	
	output->Buffer = nullptr;
	size = MultiByteToWideChar( CP_UTF8, 0, input.c_str(), -1, NULL, 0 );
	ncheck_error_action_quiet( size > 0, err = make_error_code( err_t::internal_error ), exit );
	output->Length = (USHORT)( size * sizeof( wchar_t ) );
	output->Buffer = (PWCHAR) malloc( output->Length );
	ncheck_error_action_quiet( output->Buffer, err = make_error_code( err_t::no_memory ), exit );
	size = MultiByteToWideChar( CP_UTF8, 0, input.c_str(), -1, output->Buffer, size );
	ncheck_error_action_quiet( size > 0, err = make_error_code( err_t::internal_error ), exit );

	// We're going to subtrace one wchar_t from the size, because we didn't
	// include it when we encoded the string

	output->MaximumLength = output->Length;
	output->Length		-= sizeof( wchar_t );
	
exit:

	if ( err && output->Buffer )
	{
		free( output->Buffer );
		output->Buffer = NULL;
	}

	return err;
}


static std::error_code
MakeUTF8StringFromLsaString( PLSA_UNICODE_STRING input, std::string &output )
{
	output = narrow( std::wstring( input->Buffer, input->Buffer + input->Length ) );
	return std::error_code();
}

