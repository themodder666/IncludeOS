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

#include "acpi.hpp"
#include <kernel/syscalls.hpp>
#include <hw/ioport.hpp>
#include <debug>
#include <info>

extern "C" void reboot_os();

namespace x86 {
  
  struct RSDPDescriptor {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
  } __attribute__ ((packed));
  
  struct RSDPDescriptor20 {
    RSDPDescriptor rdsp10;
    
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t  ExtendedChecksum;
    uint8_t  reserved[3];
  } __attribute__ ((packed));
  
  struct SDTHeader {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
    
    uint32_t sigint() const {
      return *(uint32_t*) Signature;
    }
  };
  
  struct MADTRecord {
    uint8_t type;
    uint8_t length;
    uint8_t data[0];
  };
  
  struct MADTHeader {
    
    SDTHeader  hdr;
    uintptr_t  lapic_addr;
    uint32_t   flags; // 1 = dual 8259 PICs
    MADTRecord record[0];
  };
  
  struct FACPHeader
  {
    SDTHeader sdt;
    uint32_t  unneded1;
    uint32_t* DSDT;
    uint8_t unneded2[48 - 44];
    uint32_t* SMI_CMD;
    uint8_t ACPI_ENABLE;
    uint8_t ACPI_DISABLE;
    uint8_t unneded3[64 - 54];
    uint32_t* PM1a_CNT_BLK;
    uint32_t* PM1b_CNT_BLK;
    uint8_t unneded4[89 - 72];
    uint8_t PM1_CNT_LEN;
  };
  
  struct AddressStructure
  {
    uint8_t  address_space_id; // 0 - system memory, 1 - system I/O
    uint8_t  register_bit_width;
    uint8_t  register_bit_offset;
    uint8_t  reserved;
    uint64_t address;
  };
  
  struct pci_vendor_t
  {
    uint16_t    ven_id;
    const char* ven_name;
  };
  
  struct HPET
  {
    uint8_t hardware_rev_id;
    uint8_t comparator_count :5;
    uint8_t counter_size     :1;
    uint8_t reserved         :1;
    uint8_t legacy_replacem  :1;
    pci_vendor_t pci_vendor_id;
    AddressStructure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
  } __attribute__((packed));
  
  uint64_t ACPI::time() {
    return 0;
  }
  
  void ACPI::begin(const void* addr) {
    
    auto* rdsp = (RSDPDescriptor20*) addr;
    INFO("ACPI", "Reading headers");
    INFO2("OEM: %.*s Rev. %u", 
        6, rdsp->rdsp10.OEMID, rdsp->rdsp10.Revision);
    
    auto* rsdt = (SDTHeader*) rdsp->rdsp10.RsdtAddress;
    // verify Root SDT
    if (!checksum((char*) rsdt, rsdt->Length)) {
      printf("ACPI: SDT failed checksum!");
      panic("SDT checksum failed");
    }
    
    // walk through system description table headers
    // remember the interesting ones, and count CPUs
    walk_sdts((char*) rsdt);
  }
  
  constexpr uint32_t bake(char a, char b , char c, char d) {
    return a | (b << 8) | (c << 16) | (d << 24);
  }
  
  void ACPI::walk_sdts(const char* addr)
  {
    // find total number of SDTs
    auto* rsdt = (SDTHeader*) addr;
    int  total = (rsdt->Length - sizeof(SDTHeader)) / 4;
    // go past rsdt
    addr += sizeof(SDTHeader);
    
    // parse all tables
    constexpr uint32_t APIC_t = bake('A', 'P', 'I', 'C');
    constexpr uint32_t HPET_t = bake('H', 'P', 'E', 'T');
    constexpr uint32_t FACP_t = bake('F', 'A', 'C', 'P');
    
    while (total) {
      // convert *addr to SDT-address
      auto sdt_ptr = *(intptr_t*) addr;
      // create SDT pointer
      auto* sdt = (SDTHeader*) sdt_ptr;
      // find out which SDT it is
      switch (sdt->sigint()) {
      case APIC_t:
        debug("APIC found: P=%p L=%u\n", sdt, sdt->Length);
        walk_madt((char*) sdt);
        break;
      case HPET_t:
        debug("HPET found: P=%p L=%u\n", sdt, sdt->Length);
        this->hpet_base = sdt_ptr + sizeof(SDTHeader);
        break;
      case FACP_t:
        debug("FACP found: P=%p L=%u\n", sdt, sdt->Length);
        walk_facp((char*) sdt);
        break;
      default:
        debug("Signature: %.*s (u=%u)\n", 4, sdt->Signature, sdt->sigint());
      }
      
      addr += 4; total--;
    }
    debug("Finished walking SDTs\n");
  }
  
  void ACPI::walk_madt(const char* addr)
  {
    auto* hdr = (MADTHeader*) addr;
    INFO("ACPI", "Reading APIC information");
    // the base address for APIC registers
    INFO2("LAPIC base: 0x%x  (flags: 0x%x)", 
        hdr->lapic_addr, hdr->flags);
    this->apic_base = hdr->lapic_addr;
    
    // the length remaining after MADT header
    int len = hdr->hdr.Length - sizeof(MADTHeader);
    // start walking
    const char* ptr = (char*) hdr->record;
    
    while (len) {
      auto* rec = (MADTRecord*) ptr;
      switch (rec->type) {
      case 0:
        {
          auto& lapic = *(LAPIC*) rec;
          lapics.push_back(lapic);
          //INFO2("-> CPU %u ID %u  (flags=0x%x)", 
          //    lapic.cpu, lapic.id, lapic.flags);
        }
        break;
      case 1:
        {
          auto& ioapic = *(IOAPIC*) rec;
          ioapics.push_back(ioapic);
          INFO2("I/O APIC %u   ADDR 0x%x  INTR 0x%x", 
              ioapic.id, ioapic.addr_base, ioapic.intr_base);
        }
        break;
      case 2:
        {
          auto& redirect = *(override_t*) rec;
          overrides.push_back(redirect);
          INFO2("IRQ redirect for bus %u from IRQ %u to VEC %u", 
              redirect.bus_source, redirect.irq_source, redirect.global_intr);
        }
        break;
      default:
        debug("Unrecognized ACPI MADT type: %u\n", rec->type);
      }
      // decrease length as we go
      len -= rec->length;
      // go to next entry
      ptr += rec->length;
    }
    INFO("SMP", "Found %u APs", lapics.size());
  }
  
  void ACPI::walk_facp(const char* addr)
  {
    auto* facp = (FACPHeader*) addr;
    // verify DSDT
    constexpr uint32_t DSDT_t = bake('D', 'S', 'D', 'T');
    assert(*facp->DSDT == DSDT_t);
    
    /// big thanks to kaworu from OSdev.org forums for algo
    /// http://forum.osdev.org/viewtopic.php?t=16990
    char* S5Addr = (char*) facp->DSDT + 36; // skip header
    int dsdtLength = ((SDTHeader*) facp->DSDT)->Length;
    // some ting wong
    dsdtLength *= 2;
    while (dsdtLength-- > 0)
    {
      if (memcmp(S5Addr, "_S5_", 4) == 0)
           break;
      S5Addr++;
    }
    // check if \_S5 was found
    if (dsdtLength <= 0) {
      printf("WARNING: _S5 not present in ACPI\n");
      return;
    }
    // check for valid AML structure
    if ( ( *(S5Addr-1) == 0x08 || ( *(S5Addr-2) == 0x08 && *(S5Addr-1) == '\\') ) && *(S5Addr+4) == 0x12 )
    {
       S5Addr += 5;
       S5Addr += ((*S5Addr &0xC0)>>6) + 2;   // calculate PkgLength size

       if (*S5Addr == 0x0A)
          S5Addr++;   // skip byteprefix
       SLP_TYPa = *(S5Addr) << 10;
       S5Addr++;

       if (*S5Addr == 0x0A)
          S5Addr++;   // skip byteprefix
       SLP_TYPb = *(S5Addr)<<10;

       SMI_CMD = facp->SMI_CMD;

       ACPI_ENABLE = facp->ACPI_ENABLE;
       ACPI_DISABLE = facp->ACPI_DISABLE;

       PM1a_CNT = facp->PM1a_CNT_BLK;
       PM1b_CNT = facp->PM1b_CNT_BLK;
       
       PM1_CNT_LEN = facp->PM1_CNT_LEN;

       SLP_EN = 1<<13;
       SCI_EN = 1;

       debug("ACPI: Found shutdown information\n");
       return;
       
    } else {
       printf("WARNING: Failed to parse _S5 in ACPI\n");
    }
    // disable ACPI shutdown
    SCI_EN = 0;
  }
  
  bool ACPI::checksum(const char* addr, size_t size) const {
    
    const char* end = addr + size;
    uint8_t sum = 0;
    while (addr < end) {
      sum += *addr; addr++;
    }
    return sum == 0;
  }
  
  void ACPI::discover() {
    // "RSD PTR "
    const uint64_t sign = 0x2052545020445352;
    
    // guess at QEMU location of RDSP
    const auto* guess = (char*) 0xf6450;
    if (*(uint64_t*) guess == sign) {
      if (checksum(guess, sizeof(RSDPDescriptor))) {
        debug("Found ACPI located at QEMU-guess (%p)\n", guess);
        begin(guess);
        return;
      }
    }
    
    // search in BIOS area (below 1mb)
    const auto* addr = (char*) 0x000e0000;
    const auto* end  = (char*) 0x000fffff;
    debug("Looking for ACPI at %p\n", addr);
    
    while (addr < end) {
      
      if (*(uint64_t*) addr == sign) {
        // verify checksum of potential RDSP
        if (checksum(addr, sizeof(RSDPDescriptor))) {
          debug("Found ACPI located at %p\n", addr);
          begin(addr);
          return;
        }
      }
      addr++;
    }
    
    panic("ACPI RDST-search failed\n");
  }
  
  void ACPI::reboot()
  {
    ::reboot_os();
  }
  
  void ACPI::acpi_shutdown() {
    // check if shutdown enabled
    if (SCI_EN == 1) {
      // write shutdown commands
      hw::outw((uint32_t) PM1a_CNT, SLP_TYPa | SLP_EN);
      if (PM1b_CNT != 0)
        hw::outw((uint32_t) PM1b_CNT, SLP_TYPb | SLP_EN);
      printf("*** ACPI shutdown failed\n");
    }
  }
  
  __attribute__((noreturn))
  void ACPI::shutdown()
  {
    asm volatile("cli");
    
    // ACPI shutdown
    get().acpi_shutdown();
    
    // http://forum.osdev.org/viewtopic.php?t=16990
    hw::outw (0xB004, 0x2000);
    
    const char s[] = "Shutdown";
    const char *p;
    for (p = s; *p; p++)
      // magic code for bochs and qemu
      hw::outb (0x8900, *s);

    // VMWare poweroff when "gui.exitOnCLIHLT" is true
    printf("Shutdown failed :(\n");
    while (true) {
      asm volatile("cli; hlt" : : : "memory");
    }
  }
}
