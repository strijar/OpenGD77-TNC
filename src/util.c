/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "util.h"

void dump(char *label, const uint8_t *buf, size_t len) {
    printf("%s %i: ", label, len);

    for (size_t i = 0; i < len; i++)
        printf("%02X ", buf[i]);

    printf("\n");
}

uint64_t get_time() {
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    uint64_t usec = (uint64_t) now.tv_sec * 1000000L + now.tv_nsec / 1000;

    return usec / 1000;
}
