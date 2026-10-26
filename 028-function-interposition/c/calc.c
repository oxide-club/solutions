/* SPDX-License-Identifier: MIT */
#include "calc.h"
#include <errno.h>
#include <limits.h>

int calc_div(int a, int b, int *out) {
    if (out == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (b == 0) {
        errno = EDOM;
        return -1;
    }
    if (a == INT_MIN && b == -1) {
        errno = ERANGE;
        return -1;
    }
    *out = a / b;
    return 0;
}
