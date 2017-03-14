// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_TCP_COMMON_HPP
#define NET_TCP_COMMON_HPP

#include <net/ip4/addr.hpp>
#include <net/packet.hpp>
#include <chrono>

namespace net {
  namespace tcp {

    // Constants
    // default size of TCP window - how much data can be "in flight" (unacknowledged)
    static constexpr uint16_t default_window_size {0xffff};
    // window scaling + window size
    static constexpr uint8_t  default_window_scaling {5};
    static constexpr uint32_t default_ws_window_size {8192 << default_window_scaling};
    // use of timestamps option
    static constexpr bool     default_timestamps {true};
    // maximum size of a TCP segment - later set based on MTU or peer
    static constexpr uint16_t default_mss {536};
    // the maximum amount of half-open connections per port (listener)
    static constexpr size_t   default_max_syn_backlog {64};
    // clock granularity of the timestamp value clock
    static constexpr float   clock_granularity {0.0001};

    static const std::chrono::seconds       default_msl {30};
    static const std::chrono::milliseconds  default_dack_timeout {40};

    using Address = ip4::Addr;

    /** A port */
    using port_t = uint16_t;

    /** A Sequence number (SYN/ACK) (32 bits) */
    using seq_t = uint32_t;

    /** A shared buffer pointer */
    using buffer_t = std::shared_ptr<uint8_t>;

    /**
     * @brief Creates a shared buffer with a given length
     *
     * @param length buffer length
     * @return a newly created buffer_t
     */
    inline buffer_t new_shared_buffer(uint64_t length)
    { return buffer_t(new uint8_t[length], std::default_delete<uint8_t[]>()); }

    class Packet;
    using Packet_ptr = std::unique_ptr<Packet>;

    class Connection;
    using Connection_ptr = std::shared_ptr<Connection>;

  } // < namespace tcp
} // < namespace net

#endif // < NET_TCP_COMMON_HPP
