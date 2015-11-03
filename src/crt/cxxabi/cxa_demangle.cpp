//===-------------------------- cxa_demangle.cpp --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//#define DEBUG
#ifdef DEBUG

#include "cxa_demangle_ORIGINAL.inc"

#else


#include <stddef.h>
#include <stdint.h>

namespace __cxxabiv1
{

namespace
{


extern "C"

char*
__cxa_demangle(const char* mangled_name, char* buf, size_t* n, int* status)
{

return (char*)"We don't think 248 KB is worth it for demangling.";

}

}

}  // __cxxabiv1

#endif
