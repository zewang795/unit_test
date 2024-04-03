/**
    MIT License

    Copyright (c) 2019 coolxv

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
 */

#ifndef __STUB_H__
#define __STUB_H__

#ifdef _WIN32
// windows
#include <processthreadsapi.h>
#include <windows.h>
#else
// linux
#include <sys/mman.h>
#include <unistd.h>
#endif
// c
#include <cstddef>
#include <cstring>
// c++
#include <map>

#define ADDR(CLASS_NAME, MEMBER_NAME) (&CLASS_NAME::MEMBER_NAME)

/**********************************************************
                  replace function
**********************************************************/
#ifdef _WIN32
#define CACHEFLUSH(addr, size) FlushInstructionCache(GetCurrentProcess(), addr, size)
#else
#define CACHEFLUSH(addr, size) __builtin___clear_cache(addr, addr + size)
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define CODESIZE 16U
#define CODESIZE_MIN 16U
#define CODESIZE_MAX CODESIZE
// ldr x9, +8
// br x9
// addr
#define REPLACE_FAR(t, fn, fn_stub)            \
  ((uint32_t *)fn)[0] = 0x58000040 | 9;        \
  ((uint32_t *)fn)[1] = 0xd61f0120 | (9 << 5); \
  *(long long *)(fn + 8) = (long long)fn_stub; \
  CACHEFLUSH((char *)fn, CODESIZE);
#define REPLACE_NEAR(t, fn, fn_stub) REPLACE_FAR(t, fn, fn_stub)
#elif defined(__arm__) || defined(_M_ARM)
#define CODESIZE 8U
#define CODESIZE_MIN 8U
#define CODESIZE_MAX CODESIZE
// ldr pc, [pc, #-4]
#define REPLACE_FAR(t, fn, fn_stub)        \
  ((uint32_t *)fn)[0] = 0xe51ff004;        \
  ((uint32_t *)fn)[1] = (uint32_t)fn_stub; \
  CACHEFLUSH((char *)fn, CODESIZE);
#define REPLACE_NEAR(t, fn, fn_stub) REPLACE_FAR(t, fn, fn_stub)
#elif defined(__thumb__) || defined(_M_THUMB)
#define CODESIZE 12
#define CODESIZE_MIN 12
#define CODESIZE_MAX CODESIZE
// NOP
// LDR.W PC, [PC]
#define REPLACE_FAR(t, fn, fn_stub)                  \
  uint32_t clearBit0 = fn & 0xfffffffe;              \
  char *f = (char *)clearBit0;                       \
  if (clearBit0 % 4 != 0) {                          \
    *(uint16_t *)&f[0] = 0xbe00;                     \
  }                                                  \
  *(uint16_t *)&f[2] = 0xf8df;                       \
  *(uint16_t *)&f[4] = 0xf000;                       \
  *(uint16_t *)&f[6] = (uint16_t)(fn_stub & 0xffff); \
  *(uint16_t *)&f[8] = (uint16_t)(fn_stub >> 16);    \
  CACHEFLUSH((char *)f, CODESIZE);
#define REPLACE_NEAR(t, fn, fn_stub) REPLACE_FAR(t, fn, fn_stub)
#elif defined(__mips64)
#define CODESIZE 80U
#define CODESIZE_MIN 80U
#define CODESIZE_MAX CODESIZE
// MIPS has no PC pointer, so you need to manually enter and exit the stack
// 120000ce0:  67bdffe0    daddiu  sp, sp, -32  //enter the stack
// 120000ce4:  ffbf0018    sd  ra, 24(sp)
// 120000ce8:  ffbe0010    sd  s8, 16(sp)
// 120000cec:  ffbc0008    sd  gp, 8(sp)
// 120000cf0:  03a0f025    move    s8, sp

// 120000d2c:  03c0e825    move    sp, s8  //exit the stack
// 120000d30:  dfbf0018    ld  ra, 24(sp)
// 120000d34:  dfbe0010    ld  s8, 16(sp)
// 120000d38:  dfbc0008    ld  gp, 8(sp)
// 120000d3c:  67bd0020    daddiu  sp, sp, 32
// 120000d40:  03e00008    jr  ra

#define REPLACE_FAR(t, fn, fn_stub)                  \
  ((uint32_t *)fn)[0] = 0x67bdffe0;                  \
  ((uint32_t *)fn)[1] = 0xffbf0018;                  \
  ((uint32_t *)fn)[2] = 0xffbe0010;                  \
  ((uint32_t *)fn)[3] = 0xffbc0008;                  \
  ((uint32_t *)fn)[4] = 0x03a0f025;                  \
  *(uint16_t *)(fn + 20) = (long long)fn_stub >> 32; \
  *(fn + 22) = 0x19;                                 \
  *(fn + 23) = 0x24;                                 \
  ((uint32_t *)fn)[6] = 0x0019cc38;                  \
  *(uint16_t *)(fn + 28) = (long long)fn_stub >> 16; \
  *(fn + 30) = 0x39;                                 \
  *(fn + 31) = 0x37;                                 \
  ((uint32_t *)fn)[8] = 0x0019cc38;                  \
  *(uint16_t *)(fn + 36) = (long long)fn_stub;       \
  *(fn + 38) = 0x39;                                 \
  *(fn + 39) = 0x37;                                 \
  ((uint32_t *)fn)[10] = 0x0320f809;                 \
  ((uint32_t *)fn)[11] = 0x00000000;                 \
  ((uint32_t *)fn)[12] = 0x00000000;                 \
  ((uint32_t *)fn)[13] = 0x03c0e825;                 \
  ((uint32_t *)fn)[14] = 0xdfbf0018;                 \
  ((uint32_t *)fn)[15] = 0xdfbe0010;                 \
  ((uint32_t *)fn)[16] = 0xdfbc0008;                 \
  ((uint32_t *)fn)[17] = 0x67bd0020;                 \
  ((uint32_t *)fn)[18] = 0x03e00008;                 \
  ((uint32_t *)fn)[19] = 0x00000000;                 \
  CACHEFLUSH((char *)fn, CODESIZE);
#define REPLACE_NEAR(t, fn, fn_stub) REPLACE_FAR(t, fn, fn_stub)

#elif defined(__riscv)
#define CODESIZE 8U
#define CODESIZE_MIN 8U
#define CODESIZE_MAX CODESIZE
// absolute offset(64), not supported
#define REPLACE_FAR(t, fn, fn_stub)

// relative offset(32)
// auipc t1, uimm20
// jalr x0, t1, simm12
#define REPLACE_NEAR(t, fn, fn_stub)        \
  unsigned int tmp = (int)(fn_stub - fn);   \
  unsigned int tmp20 = tmp & 0xfffff000;    \
  unsigned int auipc = tmp20 + 0x317;       \
  *(unsigned int *)(fn) = auipc;            \
  unsigned int tmp12 = (tmp & 0xfff) << 20; \
  unsigned int jalr = tmp12 + 0x30067;      \
  *(unsigned int *)(fn + 4) = jalr;         \
  CACHEFLUSH((char *)fn, CODESIZE);

#else  //__i386__ _x86_64__  _M_IX86 _M_X64
#define CODESIZE 13U
#define CODESIZE_MIN 5U
#define CODESIZE_MAX CODESIZE
// 13 byte(jmp m16:64)
// movabs $0x102030405060708,%r11
// jmpq   *%r11
#define REPLACE_FAR(t, fn, fn_stub)            \
  *fn = 0x49;                                  \
  *(fn + 1) = 0xbb;                            \
  *(long long *)(fn + 2) = (long long)fn_stub; \
  *(fn + 10) = 0x41;                           \
  *(fn + 11) = 0xff;                           \
  *(fn + 12) = 0xe3;                           \
  CACHEFLUSH((char *)fn, CODESIZE);

// 5 byte(jmp rel32)
#define REPLACE_NEAR(t, fn, fn_stub)                     \
  *fn = 0xE9;                                            \
  *(int *)(fn + 1) = (int)(fn_stub - fn - CODESIZE_MIN); \
  CACHEFLUSH((char *)fn, CODESIZE);
#endif

struct func_stub {
  char *fn;
  unsigned char code_buf[CODESIZE];
  bool far_jmp;
};

class Stub {
 public:
  Stub() {
#ifdef _WIN32
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    m_pagesize = sys_info.dwPageSize;
#else
    m_pagesize = sysconf(_SC_PAGE_SIZE);
#endif

    if (m_pagesize < 0) {
      m_pagesize = 4096;
    }
  }

  ~Stub() { clear(); }

  void clear() {
    std::map<char *, func_stub *>::iterator iter;
    struct func_stub *pstub;
    for (iter = m_result.begin(); iter != m_result.end(); iter++) {
      pstub = iter->second;
#ifdef _WIN32
      DWORD lpflOldProtect;
      if (0 != VirtualProtect(pageof(pstub->fn), m_pagesize * 2, PAGE_EXECUTE_READWRITE, &lpflOldProtect))
#else
      if (0 == mprotect(pageof(pstub->fn), m_pagesize * 2, PROT_READ | PROT_WRITE | PROT_EXEC))
#endif
      {

        if (pstub->far_jmp) {
          std::memcpy(pstub->fn, pstub->code_buf, CODESIZE_MAX);
        } else {
          std::memcpy(pstub->fn, pstub->code_buf, CODESIZE_MIN);
        }

        CACHEFLUSH(pstub->fn, CODESIZE);

#ifdef _WIN32
        VirtualProtect(pageof(pstub->fn), m_pagesize * 2, PAGE_EXECUTE_READ, &lpflOldProtect);
#else
        mprotect(pageof(pstub->fn), m_pagesize * 2, PROT_READ | PROT_EXEC);
#endif
      }

      iter->second = NULL;
      delete pstub;
    }

    m_result.clear();
  }
  template <typename T, typename S>
  void set(T addr, S addr_stub) {
    char *fn;
    char *fn_stub;
    fn = addrof(addr);
    fn_stub = addrof(addr_stub);
    struct func_stub *pstub;
    pstub = new func_stub;
    // start
    reset(fn);  //
    pstub->fn = fn;

    if (distanceof(fn, fn_stub)) {
      pstub->far_jmp = true;
      std::memcpy(pstub->code_buf, fn, CODESIZE_MAX);
    } else {
      pstub->far_jmp = false;
      std::memcpy(pstub->code_buf, fn, CODESIZE_MIN);
    }

#ifdef _WIN32
    DWORD lpflOldProtect;
    if (0 == VirtualProtect(pageof(pstub->fn), m_pagesize * 2, PAGE_EXECUTE_READWRITE, &lpflOldProtect))
#else
    if (-1 == mprotect(pageof(pstub->fn), m_pagesize * 2, PROT_READ | PROT_WRITE | PROT_EXEC))
#endif
    {
      throw("stub set memory protect to w+r+x faild");
    }

    if (pstub->far_jmp) {
      REPLACE_FAR(this, fn, fn_stub);
    } else {
      REPLACE_NEAR(this, fn, fn_stub);
    }

#ifdef _WIN32
    if (0 == VirtualProtect(pageof(pstub->fn), m_pagesize * 2, PAGE_EXECUTE_READ, &lpflOldProtect))
#else
    if (-1 == mprotect(pageof(pstub->fn), m_pagesize * 2, PROT_READ | PROT_EXEC))
#endif
    {
      throw("stub set memory protect to r+x failed");
    }
    m_result.insert(std::pair<char *, func_stub *>(fn, pstub));
    return;
  }

  template <typename T>
  void reset(T addr) {
    char *fn;
    fn = addrof(addr);

    std::map<char *, func_stub *>::iterator iter = m_result.find(fn);

    if (iter == m_result.end()) {
      return;
    }
    struct func_stub *pstub;
    pstub = iter->second;

#ifdef _WIN32
    DWORD lpflOldProtect;
    if (0 == VirtualProtect(pageof(pstub->fn), m_pagesize * 2, PAGE_EXECUTE_READWRITE, &lpflOldProtect))
#else
    if (-1 == mprotect(pageof(pstub->fn), m_pagesize * 2, PROT_READ | PROT_WRITE | PROT_EXEC))
#endif
    {
      throw("stub reset memory protect to w+r+x faild");
    }

    if (pstub->far_jmp) {
      std::memcpy(pstub->fn, pstub->code_buf, CODESIZE_MAX);
    } else {
      std::memcpy(pstub->fn, pstub->code_buf, CODESIZE_MIN);
    }

    CACHEFLUSH(pstub->fn, CODESIZE);

#ifdef _WIN32
    if (0 == VirtualProtect(pageof(pstub->fn), m_pagesize * 2, PAGE_EXECUTE_READ, &lpflOldProtect))
#else
    if (-1 == mprotect(pageof(pstub->fn), m_pagesize * 2, PROT_READ | PROT_EXEC))
#endif
    {
      throw("stub reset memory protect to r+x failed");
    }
    m_result.erase(iter);
    delete pstub;

    return;
  }

 protected:
  char *pageof(char *addr) {
#ifdef _WIN32
    return (char *)((unsigned long long)addr & ~(m_pagesize - 1));
#else
    return (char *)((unsigned long)addr & ~(m_pagesize - 1));
#endif
  }

  template <typename T>
  char *addrof(T addr) {
    union {
      T _s;
      char *_d;
    } ut;
    ut._s = addr;
    return ut._d;
  }

  bool distanceof(char *addr, char *addr_stub) {
    std::ptrdiff_t diff = addr_stub >= addr ? addr_stub - addr : addr - addr_stub;
    if ((sizeof(addr) > 4) && (((diff >> 31) - 1) > 0)) {
      return true;
    }
    return false;
  }

 protected:
#ifdef _WIN32
  // LLP64
  long long m_pagesize;
#else
  // LP64
  long m_pagesize;
#endif
  std::map<char *, func_stub *> m_result;
};

#endif