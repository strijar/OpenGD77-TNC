/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    DMR_UNKNOWN = 0,
    DMR_DATA_START,
    DMR_DATA_PROCESS,
    DMR_DATA_END
} dmr_state_t;

dmr_state_t dmr_recv(uint8_t *data, size_t size);
void dmr_send(uint8_t *data, size_t size);
