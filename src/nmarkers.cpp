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

#include <nodeoze/nmarkers.h>

using namespace nodeoze;

const log::marker &marker::root						= log::shared().get_marker( "nodeoze", false );
const log::marker &marker::database					= log::shared().get_marker( marker::root, "database", false );
const log::marker &marker::connection				= log::shared().get_marker( marker::root, "connection", false );
const log::marker &marker::http						= log::shared().get_marker( marker::connection, "http", false );
const log::marker &marker::rpc						= log::shared().get_marker( marker::connection, "rpc", false );
const log::marker &marker::rpc_performance			= log::shared().get_marker( marker::rpc, "performance", false );
const log::marker &marker::json						= log::shared().get_marker( marker::rpc, "json", false );
const log::marker &marker::msgpack					= log::shared().get_marker( marker::rpc, "msgpack", false );
const log::marker &marker::process					= log::shared().get_marker( marker::root, "process", false );
const log::marker &marker::promise					= log::shared().get_marker( marker::root, "promise", false );
const log::marker &marker::shell					= log::shared().get_marker( marker::root, "shell", false );
const log::marker &marker::socket					= log::shared().get_marker( marker::root, "socket", false );
const log::marker &marker::socket_filter			= log::shared().get_marker( marker::socket, "filter", false );
const log::marker &marker::socket_ip				= log::shared().get_marker( marker::socket, "ip", false );
const log::marker &marker::socket_tcp				= log::shared().get_marker( marker::socket_ip, "tcp", false );
const log::marker &marker::socket_tcp_accept		= log::shared().get_marker( marker::socket_tcp, "accept", false );
const log::marker &marker::socket_tcp_connect		= log::shared().get_marker( marker::socket_tcp, "connect", false );
const log::marker &marker::socket_tcp_send			= log::shared().get_marker( marker::socket_tcp, "send", false );
const log::marker &marker::socket_tcp_recv			= log::shared().get_marker( marker::socket_tcp, "recv", false );
const log::marker &marker::socket_tcp_close			= log::shared().get_marker( marker::socket_tcp, "close", false );
const log::marker &marker::socket_udp				= log::shared().get_marker( marker::socket_ip, "udp", false );
const log::marker &marker::socket_udp_bind			= log::shared().get_marker( marker::socket_udp, "bind", false );
const log::marker &marker::socket_udp_send			= log::shared().get_marker( marker::socket_udp, "send", false );
const log::marker &marker::socket_udp_recv			= log::shared().get_marker( marker::socket_udp, "recv", false );
const log::marker &marker::socket_udp_close			= log::shared().get_marker( marker::socket_udp, "close", false );
const log::marker &marker::websocket				= log::shared().get_marker( marker::root, "websocket", false );
const log::marker &marker::proxy					= log::shared().get_marker( marker::root, "proxy", false );
const log::marker &marker::proxy_http				= log::shared().get_marker( marker::proxy, "http", false );
const log::marker &marker::proxy_socks				= log::shared().get_marker( marker::proxy, "socks", false );
const log::marker &marker::async_iter				= log::shared().get_marker( marker::root, "async_iter", false );
const log::marker &marker::machine					= log::shared().get_marker( marker::root, "machine", false );
const log::marker &marker::buffer					= log::shared().get_marker( marker::root, "buffer", false );
const log::marker &marker::logclk					= log::shared().get_marker( marker::root, "logclk", false );
const log::marker &marker::arp						= log::shared().get_marker( marker::root, "arp", false );
const log::marker &marker::state_machine			= log::shared().get_marker( marker::root, "state_machine", false );



