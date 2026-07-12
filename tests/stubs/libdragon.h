#ifndef TESTS_STUBS_LIBDRAGON_H_
#define TESTS_STUBS_LIBDRAGON_H_

#include <stdarg.h>

/*
 * Host-test stub for libdragon's debugf logger.
 * Keep this intentionally tiny: tests only need the symbol to link.
 */
static inline void debugf(const char *fmt, ...) {
    (void)fmt;
}

#endif /* TESTS_STUBS_LIBDRAGON_H_ */
