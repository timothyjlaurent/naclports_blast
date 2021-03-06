/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_DEBUGPRINT_H_
#define PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_DEBUGPRINT_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif
  ssize_t __real_write(int fd, const void *buf, size_t count);
#ifdef __cplusplus
}
#endif

static int dbgprintf(const char* format, ...) {
  const int buf_size = 1000;
  char buf[buf_size];
  va_list args;
  va_start(args, format);
  ssize_t r = vsnprintf(buf, buf_size, format, args);
  if (r > 0)
#ifndef __native_client__
    write(2, buf, r);
#else
    __real_write(2, buf, r);
#endif
  va_end(args);
  return r;
}

#endif  // PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_DEBUGPRINT_H_
