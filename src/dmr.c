/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#include <stdio.h>

#include "dmr.h"
#include "util.h"
#include "cap.h"

#define DMR_SYNC_DATA           0x40U
#define DT_VOICE_LC_HEADER      0x01U
#define DT_TERMINATOR_WITH_LC   0x02U

static void get_rssi(uint8_t *data) {
    uint16_t raw = (data[0] << 8) + data[1];
}

static void lc_header_decode(uint8_t *data, size_t size) {
}

void dmr_data(uint8_t *data, size_t size) {
    if (size == 36) {
        get_rssi(data + 34);

        size -= 2;
    }

    uint8_t type = data[0];

    if (type & DMR_SYNC_DATA) {
        if (type & DT_VOICE_LC_HEADER) {
            dump("Start", data, size);
            lc_header_decode(data, size);

            cap_open(true);
            cap_write(data, size);
        }

        if (type & DT_TERMINATOR_WITH_LC) {
            dump("End  ", data, size);
            printf("\n");

            cap_write(data, size);
            cap_close();
        }
    } else {
        dump("Pack ", data, size);
        cap_write(data, size);
    }
}
