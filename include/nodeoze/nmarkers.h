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

#ifndef	_nodeoze_markers_h
#define	_nodeoze_markers_h

#include <nodeoze/nlog.h>

namespace nodeoze {
 
namespace marker {
 
extern const nodeoze::log::marker &root;
extern const nodeoze::log::marker &database;
extern const nodeoze::log::marker &connection;
extern const nodeoze::log::marker &http;
extern const nodeoze::log::marker &rpc;
extern const nodeoze::log::marker &rpc_performance;
extern const nodeoze::log::marker &json;
extern const nodeoze::log::marker &msgpack;
extern const nodeoze::log::marker &process;
extern const nodeoze::log::marker &promise;
extern const nodeoze::log::marker &shell;
extern const nodeoze::log::marker &socket;
extern const nodeoze::log::marker &socket_filter;
extern const nodeoze::log::marker &socket_ip;
extern const nodeoze::log::marker &socket_tcp;
extern const nodeoze::log::marker &socket_tcp_accept;
extern const nodeoze::log::marker &socket_tcp_connect;
extern const nodeoze::log::marker &socket_tcp_send;
extern const nodeoze::log::marker &socket_tcp_recv;
extern const nodeoze::log::marker &socket_tcp_close;
extern const nodeoze::log::marker &socket_udp;
extern const nodeoze::log::marker &socket_udp_bind;
extern const nodeoze::log::marker &socket_udp_send;
extern const nodeoze::log::marker &socket_udp_recv;
extern const nodeoze::log::marker &socket_udp_close;
extern const nodeoze::log::marker &state_machine;
extern const nodeoze::log::marker &websocket;
extern const nodeoze::log::marker &proxy;
extern const nodeoze::log::marker &proxy_http;
extern const nodeoze::log::marker &proxy_socks;
extern const nodeoze::log::marker &async_iter;
extern const nodeoze::log::marker &machine;
extern const nodeoze::log::marker &buffer;
extern const nodeoze::log::marker &logclk;
extern const nodeoze::log::marker &dns_sd;
extern const nodeoze::log::marker &arp;

}

}

#endif
