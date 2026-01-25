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
#include "mmdvm.h"

typedef enum {
    SYNC_VOICE  = 0x20,
    SYNC_DATA   = 0x40
} sync_t;

typedef enum {
    DT_PI_HEADER            = 0b0000,
    DT_VOICE_LC_HEADER      = 0b0001,
    DT_TERMINATOR_WITH_LC   = 0b0010,
    DT_CSBK                 = 0b0011,
    DT_MBC_HEADER           = 0b0100,
    DT_MBC_CONTINUATION     = 0b0101,
    DT_DATA_HEADER          = 0b0110,
    DT_RATE_12_DATA         = 0b0111,
    DT_RATE_34_DATA         = 0b1000,
    DT_IDLE                 = 0b1001
} data_type_t;

static bool     write_cap = false;
static bool     echo = true;

static void get_rssi(uint8_t *data) {
    uint16_t raw = (data[0] << 8) + data[1];
}

static void lc_header_decode(uint8_t *data, size_t size) {
}

dmr_state_t dmr_recv(uint8_t *data, size_t size) {
    if (size == 36) {
        get_rssi(data + 34);

        size -= 2;
    }

    uint8_t sync = data[0] & 0xF0;
    uint8_t type = data[0] & 0x0F;

    dmr_state_t res = DMR_UNKNOWN;

    if (sync == SYNC_DATA) {
        switch (type) {
            case DT_VOICE_LC_HEADER:
                dump("Start", data, size);
                lc_header_decode(data, size);

                if (write_cap) {
                    cap_open(true);
                    cap_write(data, size);
                }

                if (echo) {
                    dmr_send(data, size);
                }

                res = DMR_DATA_START;
                break;

            case DT_TERMINATOR_WITH_LC:
                dump("End  ", data, size);
                printf("\n");

                if (write_cap) {
                    cap_write(data, size);
                    cap_close();
                }

                if (echo) {
                    dmr_send(data, size);
                }

                res = DMR_DATA_END;
                break;

            default:
                break;
        }
    } else {
        dump("Pack ", data, size);

        if (write_cap) {
            cap_write(data, size);
        }

        if (echo) {
            dmr_send(data, size);
        }

        res = DMR_DATA_PROCESS;
    }

    return res;
}

void dmr_send(uint8_t *data, size_t size) {
    mmdvm_send(data, size);
}
