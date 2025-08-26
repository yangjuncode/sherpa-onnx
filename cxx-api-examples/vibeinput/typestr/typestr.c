#include "typestr.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "base/os.h"
#include "base/microsleep.h"

/* We include this header to bring in unicodeType() and input_utf() definitions
 * as these are header-defined in this codebase. This ensures the standalone
 * library has the needed implementations when compiled independently. */
#if defined(USE_X11)
#include "base/xdisplay_c.h"
#endif
#include "key/keypress_c.h"

/* Minimal UTF-8 decoder.
 * Returns number of bytes consumed (1..4) or 0 on invalid sequence.
 * Writes decoded codepoint to *out if non-NULL. */
static int utf8_decode_one(const unsigned char *s, uint32_t *out) {
    if (!s) return 0;
    uint32_t cp;
    if (s[0] < 0x80) { cp = s[0]; if (out) *out = cp; return 1; }
    if ((s[0] & 0xE0) == 0xC0) {
        if ((s[1] & 0xC0) != 0x80) return 0;
        cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        if (cp < 0x80) return 0; /* overlong */
        if (out) *out = cp; return 2;
    }
    if ((s[0] & 0xF0) == 0xE0) {
        if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
        cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        if (cp < 0x800) return 0; /* overlong */
        if (out) *out = cp; return 3;
    }
    if ((s[0] & 0xF8) == 0xF0) {
        if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80) return 0;
        cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) return 0; /* overlong or out of range */
        if (out) *out = cp; return 4;
    }
    return 0;
}

static void typestr_x11(const char *text, uintptr pid, int tm, int tm1, int8_t isPid) {
    (void)pid;    /* X11 path ignores pid in underlying helpers */
    (void)isPid;  /* for parity with API */

    const unsigned char *p = (const unsigned char *)text;
    while (*p) {
        uint32_t cp = 0;
        int n = utf8_decode_one(p, &cp);
        if (n <= 0) { /* Fallback: treat as single byte */
            cp = *p;
            n = 1;
        }

        if (cp < 0x80) {
            /* ASCII and simple chars go via unicodeType (which maps to toggleKey) */
            unicodeType((unsigned)cp, 0, 0);
            if (tm > 0) microsleep((double)tm);
        } else {
            /* For wider codepoints, use X11 keysym textual form "U<hex>" via input_utf. */
            char buf[16];
            /* Uppercase hex, no leading backslash to align with robotgo's ToUC replacement */
            snprintf(buf, sizeof(buf), "U%X", (unsigned)cp);
            input_utf(buf);
            if (tm1 > 0) microsleep((double)tm1);
            if (tm > 0) microsleep((double)tm);
        }
        p += n;
    }
}

static void typestr_generic(const char *text, uintptr pid, int tm, int8_t isPid) {
    const unsigned char *p = (const unsigned char *)text;
    while (*p) {
        uint32_t cp = 0;
        int n = utf8_decode_one(p, &cp);
        if (n <= 0) { cp = *p; n = 1; }
        unicodeType((unsigned)cp, pid, isPid);
        if (tm > 0) microsleep((double)tm);
        p += n;
    }
}

void typestr(const char* text, uintptr pid, int tm, int tm1, int8_t isPid) {
    if (!text) return;
#if defined(USE_X11)
    typestr_x11(text, pid, tm, tm1, isPid);
#else
    typestr_generic(text, pid, tm, isPid);
#endif
}

void typestr_simple(const char* text) {
    typestr(text, 0, 0, 7, 0);
}
