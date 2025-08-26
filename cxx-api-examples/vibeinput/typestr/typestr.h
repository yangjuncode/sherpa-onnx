#pragma once
#ifndef TYPESTR_H
#define TYPESTR_H

/* A small C library extracted from robotgo to type UTF-8 strings.
 * It replicates the behavior of robotgo's TypeStr using the existing C helpers:
 *   - unicodeType(unsigned value, uintptr pid, int8_t isPid)
 *   - input_utf(const char* utf)  // Linux/X11 only
 *   - microsleep(double ms)
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "base/types.h"  /* for uintptr definition */

/* Type a UTF-8 string.
 * Parameters:
 *  - text: UTF-8 encoded string to type
 *  - pid:  target process/window identifier (platform-specific). Pass 0 for active app
 *  - tm:   delay (ms) between each character
 *  - tm1:  extra delay (ms) for X11 input_utf path (Linux only). Typical default is 7
 *  - isPid: on Windows, treat `pid` as HWND when 1; otherwise treat as process id when 0
 */
void typestr(const char* text, uintptr pid, int tm, int tm1, int8_t isPid);

/* Convenience helper using defaults: pid=0, tm=0, tm1=7, isPid=0. */
void typestr_simple(const char* text);

#ifdef __cplusplus
}
#endif

#endif /* TYPESTR_H */
