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

bool serial_init();
void serial_send(uint8_t *data, size_t size);
