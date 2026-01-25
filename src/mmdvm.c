/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mmdvm.h"
#include "serial.h"
#include "dmr.h"
#include "util.h"

#define FRAME           0xE0

#define CMD_GET_VERSION 0x00
#define CMD_GET_STATUS  0x01
#define CMD_SET_MODE    0x03
#define CMD_SET_FREQ    0x04
#define CMD_DMR_DATA2   0x1A

#define ACK             0x70
#define NAK             0x7F

#define MODE_IDLE       0x00
#define MODE_DMR        0x02

typedef struct item_t {
    uint8_t         *data;
    size_t          size;
    struct item_t   *next;
} item_t;

static struct item_t    *head = NULL;
static struct item_t    *tail = NULL;

static state_t          state = STATE_INIT;
static uint8_t          free_buffers = 0;

static uint64_t         status_time = 0;
static uint64_t         tx_delay = 0;

/* * */

static void send_get_version() {
    uint8_t cmd[] = { FRAME, 3, CMD_GET_VERSION };

    serial_send(cmd, sizeof(cmd));
}

static void send_get_status() {
    uint8_t cmd[] = { FRAME, 3, CMD_GET_STATUS };

    serial_send(cmd, sizeof(cmd));
}

static void send_set_mode() {
    uint8_t cmd[] = { FRAME, 4, CMD_SET_MODE, MODE_DMR };

    serial_send(cmd, sizeof(cmd));
}

static void send_set_freq(uint32_t rx, uint32_t tx) {
    uint8_t cmd[] = {
        FRAME, 12, CMD_SET_FREQ, 0x00,
        (rx >> 0) & 0xFF, (rx >> 8) & 0xFF, (rx >> 16) & 0xFF, (rx >> 24) & 0xFF,
        (tx >> 0) & 0xFF, (tx >> 8) & 0xFF, (tx >> 16) & 0xFF, (tx >> 24) & 0xFF
    };

    serial_send(cmd, sizeof(cmd));
}

static void send_data2(uint8_t *data, size_t size) {
    uint8_t cmd[3 + size];

    cmd[0] = FRAME;
    cmd[1] = size + 3;
    cmd[2] = CMD_DMR_DATA2;

    memcpy(&cmd[3], data, size);
    serial_send(cmd, sizeof(cmd));
}

/* * */

static void resp_version(uint8_t *data, size_t size) {
    data[size] = 0;
    printf("Version: protocol %i, %s\n", data[0], &data[1]);
}

static void resp_status(uint8_t *data, size_t size) {
    bool tx = (data[2] & 0x01);

    free_buffers = data[5];
    status_time = get_time() + 500;

    if (state == STATE_TX_WAIT && tx == false) {
        printf("TX Done\n");
        state = STATE_RX_WAIT;
    }
}

static void process_queue() {
    while (free_buffers > 20 && head != NULL) {
        item_t *item = head;

        send_data2(item->data, item->size);

        if (head == tail) {
            head = tail  = NULL;
        } else {
            head = head->next;
        }

        free(item->data);
        free(item);

        free_buffers--;
    }

    if (head == NULL) {
        state = STATE_TX_WAIT;
    }
}

/* * */

void mmdvm_recv(uint8_t *data, size_t size) {
    if (data[0] != FRAME || data[1] != size) {
        return;
    }

    uint8_t     cmd = data[2];
    dmr_state_t dmr;

    data += 3;
    size -= 3;

    switch (cmd) {
        case CMD_GET_VERSION:
            resp_version(data, size);
            send_set_mode();
            break;

        case CMD_GET_STATUS:
            resp_status(data, size);
            break;

        case CMD_DMR_DATA2:
            dmr = dmr_recv(data, size);
            send_get_status();

            switch (dmr) {
                case DMR_DATA_END:
                    if (head != NULL) {
                        state = STATE_TX_DELAY;
                        tx_delay = get_time() + 1000;
                    } else {
                        state = STATE_RX_WAIT;
                    }
                    break;

                case DMR_DATA_START:
                case DMR_DATA_PROCESS:
                    state = STATE_RX_PROCESS;
                    break;

                default:
                    break;
            }
            break;

        case ACK:
            switch (data[0]) {
                case CMD_SET_MODE:
                    printf("Set mode: Ok\n");
                    send_set_freq(446003120, 446003120);
                    break;

                case CMD_SET_FREQ:
                    printf("Set freq: Ok\n");
                    state = STATE_RX_WAIT;
                    break;

                default:
                    break;
            }

        default:
            break;
    }
}

void mmdvm_send(uint8_t *data, size_t size) {
    item_t *item = malloc(sizeof(item_t));

    item->data = malloc(size);
    item->size = size;

    memcpy(item->data, data, size);

    if (head == NULL && tail == NULL) {
        head = tail = item;
    } else {
        tail->next = item;
        tail = item;
    }
}

void mmdvm_idle() {
    uint64_t now = get_time();

    switch (state) {
        case STATE_INIT:
            send_get_version();
            break;

        case STATE_TX_DELAY:
            if (now > tx_delay) {
                printf("TX start\n");
                state = STATE_TX;
                process_queue();
            }
            break;

        case STATE_TX:
            if (now > status_time) {
                send_get_status();
            } else {
                process_queue();
            }
            break;

        case STATE_RX_WAIT:
        case STATE_TX_WAIT:
            if (now > status_time) {
                send_get_status();
            }
            break;

        default:
            break;
    }
}
