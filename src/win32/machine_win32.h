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

 #ifndef _nodeoze_machine_win32_h
 #define _nodeoze_machine_win32_h

#include <nodeoze/nmachine.h>
#include <nodeoze/nrunloop.h>

 namespace nodeoze {

class machine_win32 : public machine
{
public:

	class nif_win32 : public nif
	{
		friend machine_win32;
	};

	machine_win32();
	
	virtual ~machine_win32();
	
	virtual void
	refresh();
	
protected:

	std::string
	get_hostname();

	std::string
	get_mdnsname();
	
	std::string
	get_description();

	std::string
	get_uuid();

	std::string
	get_legacy_uuid();

	nodeoze::mac::address
	get_mac_address( const std::string &name );

	std::error_code
	get_index_and_mask( sockaddr *addr, std::uint32_t &index, nodeoze::ip::address &mask );

	void
	setup_printer_notifications();

	void
	schedule_printer_notifications();

	void
	teardown_printer_notifications();

	HANDLE						m_local_printer_spooler			= nullptr;
	HANDLE						m_local_printer_change_handle	= INVALID_HANDLE_VALUE;
	nodeoze::runloop::event		m_local_printer_change_event;
};
 
}

#endif
