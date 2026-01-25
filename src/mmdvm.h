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
    STATE_INIT   = 0,
    STATE_RX_WAIT,
    STATE_RX_PROCESS,
    STATE_TX_DELAY,
    STATE_TX,
    STATE_TX_WAIT,
} state_t;

void mmdvm_recv(uint8_t *data, size_t size);
void mmdvm_send(uint8_t *data, size_t size);
void mmdvm_idle();
