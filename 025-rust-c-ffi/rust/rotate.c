/* SPDX-License-Identifier: MIT */
#include "oxide.h"

uint8_t oc_rotate(uint8_t value) {
    /* A conversão final mantém somente os oito bits da rotação. */
    return (uint8_t)(((unsigned int)value << 1) | ((unsigned int)value >> 7));
}
