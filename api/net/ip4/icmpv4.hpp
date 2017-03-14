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

#ifndef NET_IP4_ICMPv4_HPP
#define NET_IP4_ICMPv4_HPP

#include "packet_icmp4.hpp"

namespace net {

  struct ICMPv4 {

    using Stack = IP4::Stack;

    // Initialize
    ICMPv4(Stack&);

    // Input from network layer
    void receive(Packet_ptr);

    // Delegate output to network layer
    inline void set_network_out(downstream s)
    { network_layer_out_ = s;  };

  private:

    Stack& inet_;
    downstream network_layer_out_ = nullptr;

    void ping_reply(icmp4::Packet&);

  }; //< class ICMPv4
} //< namespace net

#endif //< NET_IP4_ICMPv4_HPP
