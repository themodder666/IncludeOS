; This file is a part of the IncludeOS unikernel - www.includeos.org
;
; Copyright 2015 Oslo and Akershus University College of Applied Sciences
; and Alfred Bratterud
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
USE32
global unused_interrupt_handler:function
extern current_eoi_mechanism

extern register_modern_interrupt
global modern_interrupt_handler:function

extern cpu_sampling_irq_handler
global cpu_sampling_irq_entry:function

unused_interrupt_handler:
  cli
  pusha
  call DWORD [current_eoi_mechanism]
  popa
  sti
  iret

modern_interrupt_handler:
  cli
  pusha
  call register_modern_interrupt
  call DWORD [current_eoi_mechanism]
  popa
  sti
  iret

cpu_sampling_irq_entry:
  cli
  pusha
  call cpu_sampling_irq_handler
  call DWORD [current_eoi_mechanism]
  popa
  sti
  iret
