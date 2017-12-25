#ifndef _nodeoze_win32_user_h
#define _nodeoze_win32_user_h

#include <nodeoze/nstring.h>
#include <WinSock2.h>
#include <UserEnv.h>
#include <memory>

namespace nodeoze {

namespace windows {

namespace user {

class token
{
public:

	typedef std::shared_ptr< token > ptr;

	token( HANDLE native );

	virtual ~token();

	inline bool
	is_valid()
	{
		return ( m_native ) ? true : false;
	}

	token::ptr
	duplicate( DWORD desired_access, LPSECURITY_ATTRIBUTES attributes, SECURITY_IMPERSONATION_LEVEL level, TOKEN_TYPE type );

	HANDLE
	native() const
	{
		return m_native;
	}

	const std::string&
	domain() const
	{
		return m_domain;
	}

	const std::string&
	username() const
	{
		return m_username;
	}

	const std::string&
	sid() const
	{
		return m_sid;
	}

private:

	HANDLE		m_native;
	std::string	m_domain;
	std::string	m_username;
	std::string	m_sid;
};


class profile
{
public:

	typedef std::shared_ptr< profile > ptr;

	profile( const token::ptr &token );

	profile( const token::ptr &token, const std::string &username );

	~profile();

	inline bool
	is_valid()
	{
		return ( m_info.hProfile ) ? true : false;
	}

private:

	void
	load();

	PROFILEINFO	m_info;
	token::ptr	m_token;
	std::string	m_username;
};

}

}

}

#endif
