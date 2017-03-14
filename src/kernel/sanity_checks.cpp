// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <cassert>
#include <cstdint>
#include <common>
#include <kprint>
#include <util/crc32.hpp>
#include <kernel/elf.hpp>
#include <kernel/syscalls.hpp>

// NOTE: crc_to MUST NOT be initialized to zero
static uint32_t crc_ro = CRC32_BEGIN();
extern char _TEXT_START_;
extern char _TEXT_END_;
extern char _RODATA_START_;
extern char _RODATA_END_;
const auto* LOW_CHECK_SIZE = (volatile int*) 0x200;

static uint32_t generate_ro_crc() noexcept
{
  uint32_t crc = CRC32_BEGIN();
  crc = crc32(crc, &_TEXT_START_, &_RODATA_END_ - &_TEXT_START_);
  return CRC32_VALUE(crc);
}

extern "C"
void __init_sanity_checks() noexcept
{
  // zero low memory
  for (volatile int* lowmem = NULL; lowmem < LOW_CHECK_SIZE; lowmem++)
      *lowmem = 0;
  // generate checksum for read-only portions of kernel
  crc_ro = generate_ro_crc();
}

extern "C"
void kernel_sanity_checks()
{
  // verify checksum of read-only portions of kernel
  uint32_t new_ro = generate_ro_crc();
  if (crc_ro != new_ro) {
    kprintf("CRC mismatch %#x vs %#x\n", crc_ro, new_ro);
    panic("Sanity checks: CRC of kernel read-only area failed");
  }
  // verify that first page is zeroes only
  for (volatile int* lowmem = NULL; lowmem < LOW_CHECK_SIZE; lowmem++)
  if (UNLIKELY(*lowmem != 0)) {
    kprintf("Memory at %p was not zeroed: %#x\n", lowmem, *lowmem);
    panic("Sanity checks: Low-memory zero test");
  }
  // verify that Elf symbols were not overwritten
  bool symbols_verified = Elf::verify_symbols();
  if (!symbols_verified)
    panic("Sanity checks: Consistency of Elf symbols and string areas");
}
