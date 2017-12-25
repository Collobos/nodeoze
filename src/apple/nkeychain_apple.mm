#include "nkeychain_apple.h"
#include <Foundation/Foundation.h>

using namespace nodeoze;

#if defined( __APPLE__ )
#	pragma mark keychain_mac implementation
#endif

struct ObjCDeleter
{
	void operator()( id obj )
	{
		if ( obj )
		{
			[ obj release ];
		}
	}
};

NODEOZE_DEFINE_SINGLETON( keychain )

keychain*
keychain::create()
{
	return new keychain_mac;
}

keychain_mac::keychain_mac()
{
}


keychain_mac::~keychain_mac()
{
}


/*
 * Following code is thanks to this stackoverflow answer:
 * http://stackoverflow.com/questions/190963/can-i-access-the-keychain-on-the-iphone
 */

std::error_code
keychain_mac::save( const uri &key, const std::string &value )
{
	std::unique_ptr< NSArray, ObjCDeleter >			keys;
	std::unique_ptr< NSArray, ObjCDeleter >			objects;
	std::unique_ptr< NSDictionary, ObjCDeleter >	query;
	std::string										existing;
    OSStatus										status;
	
    if ( !find( key, existing ) )
	{
		keys.reset( [ [ NSArray alloc] initWithObjects: (NSString *) kSecClass, kSecAttrAccount, nil ] );
		objects.reset( [ [ NSArray alloc] initWithObjects: (NSString *) kSecClassGenericPassword, [ NSString stringWithUTF8String:key.to_string().c_str() ], nil ] );
		query.reset( [ [ NSDictionary alloc ] initWithObjects:objects.get() forKeys:keys.get() ] );
		
		status = SecItemUpdate( ( CFDictionaryRef ) query.get(), (CFDictionaryRef) [NSDictionary dictionaryWithObject:[ NSString stringWithUTF8String:value.c_str() ] forKey: (NSString *) kSecAttrGeneric ] );
    }
	else
	{
		keys.reset( [ [ NSArray alloc ] initWithObjects: (NSString *) kSecClass, kSecAttrAccount, kSecAttrGeneric, nil ] );
		objects.reset( [ [ NSArray alloc] initWithObjects: (NSString *) kSecClassGenericPassword, [ NSString stringWithUTF8String:key.to_string().c_str() ], [ NSString stringWithUTF8String:value.c_str() ], nil ] );
		query.reset( [ [ NSDictionary alloc] initWithObjects:objects.get() forKeys:keys.get() ] );
		
		status = SecItemAdd( ( CFDictionaryRef ) query.get(), nullptr );
	}
	
	return ( status == noErr ) ? std::error_code() : std::make_error_code( err_t::internal_error );
}


std::error_code
keychain_mac::find( const uri &key, std::string &value )
{
	std::unique_ptr< NSArray, ObjCDeleter >			keys( [ [ NSArray alloc ] initWithObjects:static_cast< const NSString* >( kSecClass ), kSecAttrAccount, kSecReturnAttributes, nil ] );
	std::unique_ptr< NSArray, ObjCDeleter >			objects( [ [ NSArray alloc ] initWithObjects:static_cast< const NSString* >( kSecClassGenericPassword ), [ NSString stringWithUTF8String:key.to_string().c_str() ], kCFBooleanTrue, nil ] );
	std::unique_ptr< NSDictionary, ObjCDeleter >	query( [ [ NSDictionary alloc ] initWithObjects:objects.get() forKeys:keys.get() ] );
	NSDictionary									*result = nullptr;
	OSStatus										status		= SecItemCopyMatching( ( CFDictionaryRef ) query.get(), ( CFTypeRef* ) &result );
	
	if ( status == noErr )
	{
		NSData *inserted = ( NSData* ) [ result objectForKey:( NSString* ) kSecAttrGeneric ];
		NSString *string = [ [ NSString alloc ] initWithData:inserted encoding:NSUTF8StringEncoding ];
		value = [ string UTF8String ];
		[ string release ];
		[ result release ];
	}
	
	return ( status == noErr ) ? std::error_code() : std::make_error_code( err_t::not_exist );
}

