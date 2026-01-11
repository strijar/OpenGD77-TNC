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

#include "util.h"

void dump(char *label, const uint8_t *buf, size_t len) {
    printf("%s %i: ", label, len);

    for (size_t i = 0; i < len; i++)
        printf("%02X ", buf[i]);

    printf("\n");
}
