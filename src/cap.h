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

void cap_open(bool create);
void cap_close();

void cap_write(uint8_t *data, size_t size);
bool cap_read(uint8_t *data, size_t size);
