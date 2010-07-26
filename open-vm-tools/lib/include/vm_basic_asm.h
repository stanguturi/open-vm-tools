/*********************************************************
 * Copyright (C) 2003 VMware, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation version 2.1 and no later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the Lesser GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 *********************************************************/

/*********************************************************
 * The contents of this file are subject to the terms of the Common
 * Development and Distribution License (the "License") version 1.0
 * and no later version.  You may not use this file except in
 * compliance with the License.
 *
 * You can obtain a copy of the License at
 *         http://www.opensource.org/licenses/cddl1.php
 *
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *********************************************************/

/*
 * vm_basic_asm.h
 *
 *	Basic asm macros
 *
 *        ARM not implemented.
 */

#ifndef _VM_BASIC_ASM_H_
#define _VM_BASIC_ASM_H_

#define INCLUDE_ALLOW_USERLEVEL

#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_VMMON
#define INCLUDE_ALLOW_VMK_MODULE
#define INCLUDE_ALLOW_VMKERNEL
#define INCLUDE_ALLOW_DISTRIBUTE
#define INCLUDE_ALLOW_VMCORE
#define INCLUDE_ALLOW_VMIROM
#include "includeCheck.h"

#include "vm_basic_types.h"

#if defined VM_X86_64
#include "vm_basic_asm_x86_64.h"
#elif defined __i386__
#include "vm_basic_asm_x86.h"
#endif

/*
 * x86-64 windows doesn't support inline asm so we have to use these
 * intrinsic functions defined in the compiler.  Not all of these are well
 * documented.  There is an array in the compiler dll (c1.dll) which has
 * an array of the names of all the intrinsics minus the leading
 * underscore.  Searching around in the ntddk.h file can also be helpful.
 *
 * The declarations for the intrinsic functions were taken from the DDK. 
 * Our declarations must match the ddk's otherwise the 64-bit c++ compiler
 * will complain about second linkage of the intrinsic functions.
 * We define the intrinsic using the basic types corresponding to the 
 * Windows typedefs. This avoids having to include windows header files
 * to get to the windows types.
 */
#ifdef _MSC_VER
#ifdef __cplusplus
extern "C" {
#endif
/*
 * It seems x86 & x86-64 windows still implements these intrinsic
 * functions.  The documentation for the x86-64 suggest the
 * __inbyte/__outbyte intrinsics eventhough the _in/_out work fine and
 * __inbyte/__outbyte aren't supported on x86.
 */
int            _inp(unsigned short);
unsigned short _inpw(unsigned short);
unsigned long  _inpd(unsigned short);

int            _outp(unsigned short, int);
unsigned short _outpw(unsigned short, unsigned short);
unsigned long  _outpd(uint16, unsigned long);
#pragma intrinsic(_inp, _inpw, _inpd, _outp, _outpw, _outpw, _outpd)

/*
 * Prevents compiler from re-ordering reads, writes and reads&writes.
 * These functions do not add any instructions thus only affect
 * the compiler ordering.
 *
 * See:
 * `Lockless Programming Considerations for Xbox 360 and Microsoft Windows'
 * http://msdn.microsoft.com/en-us/library/bb310595(VS.85).aspx
 */
void _ReadBarrier(void);
void _WriteBarrier(void);
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadBarrier, _WriteBarrier, _ReadWriteBarrier)

#ifdef VM_X86_64
/*
 * intrinsic functions only supported by x86-64 windows as of 2k3sp1
 */
unsigned __int64 __rdtsc(void);
void             __stosw(unsigned short *, unsigned short, size_t);
void             __stosd(unsigned long *, unsigned long, size_t);
void             _mm_pause(void);
#pragma intrinsic(__rdtsc, __stosw, __stosd, _mm_pause)

unsigned char  _BitScanForward64(unsigned long *, unsigned __int64);
unsigned char  _BitScanReverse64(unsigned long *, unsigned __int64);
#pragma intrinsic(_BitScanForward64, _BitScanReverse64)
#endif /* VM_X86_64 */

unsigned char  _BitScanForward(unsigned long *, unsigned long);
unsigned char  _BitScanReverse(unsigned long *, unsigned long);
#pragma intrinsic(_BitScanForward, _BitScanReverse)

#ifdef __cplusplus
}
#endif
#endif /* _MSC_VER */


#ifdef __GNUC__ // {
#if defined(__i386__) || defined(__x86_64__) // Only on x86*

/*
 * Checked against the Intel manual and GCC --hpreg
 *
 * volatile because reading from port can modify the state of the underlying
 * hardware.
 *
 * Note: The undocumented %z construct doesn't work (internal compiler error)
 *       with gcc-2.95.1
 */

#define __GCC_IN(s, type, name) \
static INLINE type              \
name(uint16 port)               \
{                               \
   type val;                    \
                                \
   __asm__ __volatile__(        \
      "in" #s " %w1, %0"        \
      : "=a" (val)              \
      : "Nd" (port)             \
   );                           \
                                \
   return val;                  \
}

__GCC_IN(b, uint8, INB)
__GCC_IN(w, uint16, INW)
__GCC_IN(l, uint32, IN32)


/*
 * Checked against the Intel manual and GCC --hpreg
 *
 * Note: The undocumented %z construct doesn't work (internal compiler error)
 *       with gcc-2.95.1
 */

#define __GCC_OUT(s, s2, port, val) do { \
   __asm__(                              \
      "out" #s " %" #s2 "1, %w0"         \
      :                                  \
      : "Nd" (port), "a" (val)           \
   );                                    \
} while (0)

#define OUTB(port, val) __GCC_OUT(b, b, port, val)
#define OUTW(port, val) __GCC_OUT(w, w, port, val)
#define OUT32(port, val) __GCC_OUT(l, , port, val)

#define GET_CURRENT_EIP(_eip) \
      __asm__ __volatile("call 0\n\tpopl %0" : "=r" (_eip): );

#endif // x86*

#elif defined(_MSC_VER) // } {
static INLINE  uint8
INB(uint16 port)
{
   return (uint8)_inp(port);
}
static INLINE void
OUTB(uint16 port, uint8 value)
{
   _outp(port, value);
}
static INLINE uint16
INW(uint16 port)
{
   return _inpw(port);
}
static INLINE void
OUTW(uint16 port, uint16 value)
{
   _outpw(port, value);
}
static INLINE  uint32
IN32(uint16 port)
{
   return _inpd(port);
}
static INLINE void
OUT32(uint16 port, uint32 value)
{
   _outpd(port, value);
}

#ifndef VM_X86_64
#ifdef NEAR
#undef NEAR
#endif

#define GET_CURRENT_EIP(_eip) do { \
   __asm call NEAR PTR $+5 \
   __asm pop eax \
   __asm mov _eip, eax \
} while (0)
#endif // VM_X86_64

#else // } {
#error
#endif // }

/* Sequence recommended by Intel for the Pentium 4. */
#define INTEL_MICROCODE_VERSION() (             \
   __SET_MSR(MSR_BIOS_SIGN_ID, 0),              \
   __GET_EAX_FROM_CPUID(1),                     \
   __GET_MSR(MSR_BIOS_SIGN_ID))

/*
 * Locate most and least significant bit set functions. Use our own name
 * space to avoid namespace collisions. The new names follow a pattern,
 * <prefix><size><option>, where:
 *
 * <prefix> is [lm]ssb (least/most significant bit set)
 * <size> is size of the argument: 32 (32-bit), 64 (64-bit) or Ptr (pointer)
 * <option> is for alternative versions of the functions
 *
 * NAME        FUNCTION                    BITS     FUNC(0)
 *-----        --------                    ----     -------
 * lssb32_0    LSB set (uint32)            0..31    -1
 * mssb32_0    MSB set (uint32)            0..31    -1
 * lssb64_0    LSB set (uint64)            0..63    -1
 * mssb64_0    MSB set (uint64)            0..63    -1
 * lssbPtr_0   LSB set (uintptr_t;32-bit)  0..31    -1
 * lssbPtr_0   LSB set (uintptr_t;64-bit)  0..63    -1
 * mssbPtr_0   MSB set (uintptr_t;32-bit)  0..31    -1
 * mssbPtr_0   MSB set (uintptr_t;64-bit)  0..63    -1
 * lssbPtr     LSB set (uintptr_t;32-bit)  1..32    0
 * lssbPtr     LSB set (uintptr_t;64-bit)  1..64    0
 * mssbPtr     MSB set (uintptr_t;32-bit)  1..32    0
 * mssbPtr     MSB set (uintptr_t;64-bit)  1..64    0
 * lssb32      LSB set (uint32)            1..32    0
 * mssb32      MSB set (uint32)            1..32    0
 * lssb64      LSB set (uint64)            1..64    0
 * mssb64      MSB set (uint64)            1..64    0
 */

#if defined(_MSC_VER)
static INLINE int
lssb32_0(const uint32 value)
{
   unsigned long idx;
   if (UNLIKELY(value == 0)) {
      return -1;
   }
   _BitScanForward(&idx, (unsigned long) value);
   return idx;
}

static INLINE int
mssb32_0(const uint32 value)
{
   unsigned long idx;
   if (UNLIKELY(value == 0)) {
      return -1;
   }
   _BitScanReverse(&idx, (unsigned long) value);
   return idx;
}

static INLINE int
lssb64_0(const uint64 value)
{
   if (UNLIKELY(value == 0)) {
      return -1;
   } else {
#if defined(VM_X86_64)
      unsigned long idx;
      _BitScanForward64(&idx, (unsigned __int64) value);
      return idx;
#else
      /* The coding was chosen to minimize conditionals and operations */
      int lowFirstBit = lssb32_0((uint32) value);
      if (lowFirstBit == -1) {
         lowFirstBit = lssb32_0((uint32) (value >> 32));
         if (lowFirstBit != -1) {
            return lowFirstBit + 32;
         }
      }
      return lowFirstBit;
#endif
   }
}

static INLINE int
mssb64_0(const uint64 value)
{
   if (UNLIKELY(value == 0)) {
      return -1;
   } else {
#if defined(VM_X86_64)
      unsigned long idx;
      _BitScanReverse64(&idx, (unsigned __int64) value);
      return idx;
#else
      /* The coding was chosen to minimize conditionals and operations */
      if (value > 0xFFFFFFFFULL) {
         return 32 + mssb32_0((uint32) (value >> 32));
      }
      return mssb32_0((uint32) value);
#endif
   }
}
#endif

#if defined(__GNUC__)
#if defined(__i386__) || defined(__x86_64__) // Only on x86*
#define USE_ARCH_X86_CUSTOM
#endif

static INLINE int
lssb32_0(uint32 value)
{
#if defined(USE_ARCH_X86_CUSTOM)
   if (UNLIKELY(value == 0)) {
      return -1;
   } else {
      int pos;
      __asm__ __volatile__("bsfl %1, %0\n" : "=r" (pos) : "rm" (value) : "cc");
      return pos;
   }
#else
   return __builtin_ffs(value) - 1;
#endif
}

static INLINE int
mssb32_0(uint32 value)
{
   if (UNLIKELY(value == 0)) {
      return -1;
   } else {
      int pos;

#if defined(USE_ARCH_X86_CUSTOM)
      __asm__ __volatile__("bsrl %1, %0\n" : "=r" (pos) : "rm" (value) : "cc");
#else
      pos = 32 - __builtin_clz(value) - 1;
#endif

      return pos;
   }
}

static INLINE int
lssb64_0(const uint64 value)
{
#if defined(USE_ARCH_X86_CUSTOM)
   if (UNLIKELY(value == 0)) {
      return -1;
   } else {
      intptr_t pos;

#if defined(VM_X86_64)
      __asm__ __volatile__("bsf %1, %0\n" : "=r" (pos) : "rm" (value) : "cc");
#else
      /* The coding was chosen to minimize conditionals and operations */
      pos = lssb32_0((uint32) value);
      if (pos == -1) {
         pos = lssb32_0((uint32) (value >> 32));
         if (pos != -1) {
            return pos + 32;
         }
      }
#endif

      return pos;
   }
#else
   return __builtin_ffsll(value) - 1;
#endif
}

static INLINE int
mssb64_0(const uint64 value)
{
   if (UNLIKELY(value == 0)) {
      return -1;
   } else {
      intptr_t pos;

#if defined(USE_ARCH_X86_CUSTOM)
#if defined(VM_X86_64)
      __asm__ __volatile__("bsr %1, %0\n" : "=r" (pos) : "rm" (value) : "cc");
#else
      /* The coding was chosen to minimize conditionals and operations */
      if (value > 0xFFFFFFFFULL) {
         pos = 32 + mssb32_0((uint32) (value >> 32));
      } else {
         pos = mssb32_0((uint32) value);
      }
#endif
#else
      pos = 64 - __builtin_clzll(value) - 1;
#endif

      return pos;
   }
}

#if defined(USE_ARCH_X86_CUSTOM)
#undef USE_ARCH_X86_CUSTOM
#endif

#endif

static INLINE int
lssbPtr_0(const uintptr_t value)
{
#if defined(VM_X86_64)
   return lssb64_0((uint64) value);
#else
   return lssb32_0((uint32) value);
#endif
}

static INLINE int
lssbPtr(const uintptr_t value)
{
   return lssbPtr_0(value) + 1;
}

static INLINE int
mssbPtr_0(const uintptr_t value)
{
#if defined(VM_X86_64)
   return mssb64_0((uint64) value);
#else
   return mssb32_0((uint32) value);
#endif
}

static INLINE int
mssbPtr(const uintptr_t value)
{
   return mssbPtr_0(value) + 1;
}

static INLINE int
lssb32(const uint32 value)
{
   return lssb32_0(value) + 1;
}

static INLINE int
mssb32(const uint32 value)
{
   return mssb32_0(value) + 1;
}

static INLINE int
lssb64(const uint64 value)
{
   return lssb64_0(value) + 1;
}

static INLINE int
mssb64(const uint64 value)
{
   return mssb64_0(value) + 1;
}

#ifdef __GNUC__
#if defined(__i386__) || defined(__x86_64__)

static INLINE void *
uint16set(void *dst, uint16 val, size_t count)
{
   int dummy0;
   int dummy1;

   __asm__ __volatile__("\t"
                        "cld"            "\n\t"
                        "rep ; stosw"    "\n"
                        : "=c" (dummy0), "=D" (dummy1)
                        : "0" (count), "1" (dst), "a" (val)
                        : "memory", "cc"
      );

   return dst;
}

static INLINE void *
uint32set(void *dst, uint32 val, size_t count)
{
   int dummy0;
   int dummy1;

   __asm__ __volatile__("\t"
                        "cld"            "\n\t"
                        "rep ; stosl"    "\n"
                        : "=c" (dummy0), "=D" (dummy1)
                        : "0" (count), "1" (dst), "a" (val)
                        : "memory", "cc"
      );

   return dst;
}

#else /* unknown system: rely on C to write */
static INLINE void *
uint16set(void *dst, uint16 val, size_t count)
{
   size_t i;
   for (i = 0; i < count; i++) {
     ((uint16 *) dst)[i] = val;
   }
   return dst;
}

static INLINE void *
uint32set(void *dst, uint32 val, size_t count)
{
   size_t i;
   for (i = 0; i < count; i++) {
     ((uint32 *) dst)[i] = val;
   }
   return dst;
}
#endif // defined(__i386__) || defined(__x86_64__)
#elif defined(_MSC_VER)

static INLINE void *
uint16set(void *dst, uint16 val, size_t count)
{
#ifdef VM_X86_64
   __stosw((uint16*)dst, val, count);
#else
   __asm { pushf;
           mov ax, val;
           mov ecx, count;
           mov edi, dst;
           cld;
           rep stosw;
           popf;
   }
#endif
   return dst;
}

static INLINE void *
uint32set(void *dst, uint32 val, size_t count)
{
#ifdef VM_X86_64
   __stosd((unsigned long*)dst, (unsigned long)val, count);
#else
   __asm { pushf;
           mov eax, val;
           mov ecx, count;
           mov edi, dst;
           cld;
           rep stosd;
           popf;
   }
#endif
   return dst;
}

#else
#error "No compiler defined for uint*set"
#endif


/*
 *-----------------------------------------------------------------------------
 *
 * Bswap32 --
 *
 *      Swap the 4 bytes of "v" as follows: 3210 -> 0123.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint32
Bswap32(uint32 v) // IN
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__)) // {
   /* Checked against the Intel manual and GCC. --hpreg */
   __asm__(
      "bswap %0"
      : "=r" (v)
      : "0" (v)
   );
   return v;
#else // } {
   return    (v >> 24)
          | ((v >>  8) & 0xFF00)
          | ((v & 0xFF00) <<  8)
          |  (v << 24)          ;
#endif // }
}
#define Bswap Bswap32


/*
 *-----------------------------------------------------------------------------
 *
 * Bswap64 --
 *
 *      Swap the 8 bytes of "v" as follows: 76543210 -> 01234567.
 *
 *-----------------------------------------------------------------------------
 */

static INLINE uint64
Bswap64(uint64 v) // IN
{
   return ((uint64)Bswap((uint32)v) << 32) | Bswap((uint32)(v >> 32));
}


#if defined(__i386__) || defined(__x86_64__)
/*
 * COMPILER_MEM_BARRIER prevents the compiler from re-ordering memory
 * references accross the barrier.  NOTE: It does not generate any
 * instruction, so the CPU is free to do whatever it wants to...
 */
#ifdef __GNUC__ // {
#define COMPILER_MEM_BARRIER()   __asm__ __volatile__ ("": : :"memory")
#define COMPILER_READ_BARRIER()  COMPILER_MEM_BARRIER()
#define COMPILER_WRITE_BARRIER() COMPILER_MEM_BARRIER()
#elif _MSC_VER
#define COMPILER_MEM_BARRIER()   _ReadWriteBarrier()
#define COMPILER_READ_BARRIER()  _ReadBarrier()
#define COMPILER_WRITE_BARRIER() _WriteBarrier()
#endif // }


/*
 * PAUSE is a P4 instruction that improves spinlock power+performance;
 * on non-P4 IA32 systems, the encoding is interpreted as a REPZ-NOP.
 * Use volatile to avoid NOP removal.
 */
static INLINE void
PAUSE(void)
#ifdef __GNUC__
{
   __asm__ __volatile__( "pause" :);
}
#elif defined(_MSC_VER)
#ifdef VM_X86_64
{
   _mm_pause();
}
#else /* VM_X86_64 */
#pragma warning( disable : 4035)
{
   __asm _emit 0xf3 __asm _emit 0x90
}
#pragma warning (default: 4035)
#endif /* VM_X86_64 */
#else  /* __GNUC__  */
#error No compiler defined for PAUSE
#endif


/*
 * Checked against the Intel manual and GCC --hpreg
 *
 * volatile because the tsc always changes without the compiler knowing it.
 */
static INLINE uint64
RDTSC(void)
#ifdef __GNUC__
{
#ifdef VM_X86_64
   uint64 tscLow;
   uint64 tscHigh;

   __asm__ __volatile__(
      "rdtsc"
      : "=a" (tscLow), "=d" (tscHigh)
   );

   return tscHigh << 32 | tscLow;
#else
   uint64 tim;

   __asm__ __volatile__(
      "rdtsc"
      : "=A" (tim)
   );

   return tim;
#endif
}
#elif defined(_MSC_VER)
#ifdef VM_X86_64
{
   return __rdtsc();
}
#else
#pragma warning( disable : 4035)
{
   __asm _emit 0x0f __asm _emit 0x31
}
#pragma warning (default: 4035)
#endif /* VM_X86_64 */
#else  /* __GNUC__  */
#error No compiler defined for RDTSC
#endif /* __GNUC__  */

/*
 *-----------------------------------------------------------------------------
 *
 * DEBUGBREAK --
 *
 *    Does an int3 for MSVC / GCC. This is a macro to make sure int3 is
 *    always inlined.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef _MSC_VER
#define DEBUGBREAK()   __debugbreak()
#else
#define DEBUGBREAK()   __asm__ (" int $3 ")
#endif
#endif // defined(__i386__) || defined(__x86_64__)

#endif // _VM_BASIC_ASM_H_
