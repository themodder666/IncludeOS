// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once
#ifndef NET_DHCP_DH4CLIENT_HPP
#define NET_DHCP_DH4CLIENT_HPP

#include <timers>
#include "../packet.hpp"
#include <net/ip4/ip4.hpp>
#include <net/inet.hpp>
#include "dhcp4.hpp"

namespace net
{
  class UDPSocket;

  class DHClient
  {
  public:
    using Stack = IP4::Stack;
    using config_func = delegate<void(bool)>;

    DHClient() = delete;
    DHClient(DHClient&) = delete;
    DHClient(Stack& inet);

    // negotiate with local DHCP server
    void negotiate(uint32_t timeout_secs);

    // Signal indicating the result of DHCP negotation
    // timeout is true if the negotiation timed out
    void on_config(config_func handler);

    // disable or enable console spam
    void set_silent(bool sil) noexcept
    { this->console_spam = !sil; }

  private:
    void offer(UDPSocket&, const char* data, size_t len);
    void request(UDPSocket&, const dhcp_option_t* server_id);   // --> acknowledge
    void acknowledge(const char* data, size_t len);

    Stack& stack;
    uint32_t     xid;
    IP4::addr    ipaddr, netmask, router, dns_server;
    uint32_t     lease_time;
    std::vector<config_func> config_handlers_;
    Timers::id_t timeout;
    bool         console_spam;
    bool         in_progress;
  };
}

#endif
