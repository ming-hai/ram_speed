﻿// -----------------------------------------------------------------------------------------
// ram_speed by rigaya
// -----------------------------------------------------------------------------------------
// The MIT License
//
// Copyright (c) 2014-2017 rigaya
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// --------------------------------------------------------------------------------------------

#pragma once
#ifndef __RAM_SPEED_OSDEP_H__
#define __RAM_SPEED_OSDEP_H__

#if defined(_WIN32) || defined(_WIN64)
#include <tchar.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else

static pthread_t GetCurrentThread() {
    return pthread_self();
}

static void SetThreadAffinityMask(pthread_t thread, size_t mask) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (uint32_t j = 0; j < sizeof(mask) * 8; j++) {
        if (mask & (1 << j)) {
            CPU_SET(j, &cpuset);
        }
    }
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
}

enum {
    THREAD_PRIORITY_NORMAL,
    THREAD_PRIORITY_HIGHEST,
    THREAD_PRIORITY_ABOVE_NORMAL,
    THREAD_PRIORITY_BELOW_NORMAL,
    THREAD_PRIORITY_LOWEST,
    THREAD_PRIORITY_IDLE,
};

static void SetThreadPriority(pthread_t thread, int priority) {
    return; //何もしない
}

#include <cstddef>
#include <cstring>
#include <stdarg.h>

#define _stricmp strcasecmp
#define __fastcall
#define __stdcall

template <typename _CountofType, size_t _SizeOfArray>
char (*__countof_helper(_CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
#define _countof(_Array) (int)sizeof(*__countof_helper(_Array))

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

static inline int sprintf_s(char *dst, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsprintf(dst, format, args);
    va_end(args);
    return ret;
}
static inline int sprintf_s(char *dst, size_t size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsprintf(dst, format, args);
    va_end(args);
    return ret;
}

typedef char TCHAR;
#define _T(x) x
#define _tmain main
#define _tcslen strlen
#define _ftprintf fprintf
#define _stscanf_s sscanf
#define _stscanf sscanf
#define _tcscmp strcmp
#define _tcsnccmp strncmp
#define _tcsicmp strcasecmp
#define _tcschr strchr
#define _tcsstr strstr
#define _tcstol strtol
#define _tcsdup strdup
#define _tfopen fopen
#define _tfopen_s fopen_s
#define _stprintf_s sprintf_s
#define _vsctprintf _vscprintf
#define _vstprintf_s _vsprintf_s
#define _tcstok_s strtok_s
#define _tcserror strerror
#define _fgetts fgets
#define _tcscpy strcpy

static inline char *_tcscpy_s(TCHAR *dst, const TCHAR *src) {
    return strcpy(dst, src);
}

static inline char *_tcscpy_s(TCHAR *dst, size_t size, const TCHAR *src) {
    return strcpy(dst, src);
}

#endif //!(defined(_WIN32) || defined(_WIN64))

#endif //__RAM_SPEED_OSDEP_H__
