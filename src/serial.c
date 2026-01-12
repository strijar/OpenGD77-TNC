/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>
#include <string.h>

#include "serial.h"
#include "dmr.h"
#include "util.h"
#include "cap.h"

#define FRAME       0xE0

#define GET_STATUS  0x01
#define SET_CONFIG  0x02
#define SET_MODE    0x03
#define SET_FREQ    0x04
#define DMR_DATA2   0x1A

#define ACK         0x70
#define NAK         0x7F

typedef enum {
    STATE_NEED_INIT = 0,
    STATE_SET_FREQ,
    STATE_READY,
    STATE_SEND_DATA,
    STATE_GET_STATUS,
    STATE_DONE
} state_t;

static int      fd;
static state_t  state = STATE_NEED_INIT;
static uint8_t  free_buffers = 0;

static void send_get_status() {
    uint8_t cmd[] = { FRAME, 3, GET_STATUS };

    write(fd, cmd, sizeof(cmd));
}

static void send_set_mode() {
    uint8_t cmd[] = { FRAME, 4, SET_MODE, 0x00 };

    write(fd, cmd, sizeof(cmd));
}

static void send_set_freq(uint32_t rx, uint32_t tx) {
    uint8_t cmd[] = {
        FRAME, 12, SET_FREQ, 0x00,
        (rx >> 0) & 0xFF, (rx >> 8) & 0xFF, (rx >> 16) & 0xFF, (rx >> 24) & 0xFF,
        (tx >> 0) & 0xFF, (tx >> 8) & 0xFF, (tx >> 16) & 0xFF, (tx >> 24) & 0xFF
    };

    write(fd, cmd, sizeof(cmd));
}

static void send_data2(uint8_t *data, size_t size) {
    uint8_t cmd[3 + size];

    cmd[0] = FRAME;
    cmd[1] = size + 3;
    cmd[2] = DMR_DATA2;

    memcpy(&cmd[3], data, size);
    write(fd, cmd, sizeof(cmd));

    dump("Send", cmd, sizeof(cmd));
}

static void next_data() {
    uint8_t buf[256];

    if (free_buffers < 20) {
        return;
    }

    if (cap_read(buf, 34)) {
        send_data2(buf, 34);
        state = STATE_SEND_DATA;
    } else {
        printf("Done\n");
        cap_close();
        state = STATE_DONE;
    }
}

static void * serial_worker(void *p) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    uint8_t buf[256];

    while (true) {
        int n = read(fd, &buf, sizeof(buf));

        if (n > 0) {
            if (buf[0] != FRAME || buf[1] != n) {
                continue;
            }

            switch (buf[2]) {
                case ACK:
                    switch (buf[3]) {
                        case SET_FREQ:
                            printf("Set freq: Ok\n");
                            state = STATE_READY;
                            break;

                        case DMR_DATA2:
                            send_get_status();
                            state = STATE_GET_STATUS;
                            break;

                        default:
                            break;
                    }
                    break;

                case NAK:
                    switch (buf[3]) {
                        case SET_FREQ:
                            printf("Set freq: Error %i\n", buf[4]);
                            break;

                        case DMR_DATA2:
                            printf("Send data: Error %i\n", buf[4]);
                            state = STATE_DONE;
                            break;

                        default:
                            break;
                    }
                    break;

                case GET_STATUS:
                    free_buffers = buf[8];

                    switch (state) {
                        case STATE_NEED_INIT:
                            send_set_freq(446003120, 446003120);
                            state = STATE_SET_FREQ;
                            break;

                        case STATE_READY:
                            if (false) {
                                state = STATE_DONE;
                            } else {
                                cap_open(false);
                                next_data();
                            }
                            break;

                        case STATE_GET_STATUS:
                            next_data();
                            break;

                        default:
                            break;
                    }
                    break;

                case DMR_DATA2:
                    dmr_data(buf + 3, n - 3);
                    break;

                default:
                    dump("Serial", buf, n);
                    break;
            }
        } else {
            send_get_status();
        }
    }
}

bool serial_init() {
    fd = open("/dev/ttyACM0", O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        return false;
    }

    struct termios tty;

    if (tcgetattr(fd, &tty) != 0) {
        return false;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag |= (CLOCAL | CREAD);    /* Ignore modem control lines, enable receiver */
    tty.c_cflag &= ~CSIZE;              /* Clear size bits */
    tty.c_cflag |= CS8;                 /* 8 data bits */
    tty.c_cflag &= ~PARENB;             /* No parity bit */
    tty.c_cflag &= ~CSTOPB;             /* 1 stop bit */

    tty.c_lflag &= ~ICANON;             /* Set non-canonical mode */
    tty.c_lflag &= ~ECHO;               /* Disable echo */
    tty.c_lflag &= ~ECHOE;              /* Disable erasure */
    tty.c_lflag &= ~ECHONL;             /* Disable new-line echo */
    tty.c_lflag &= ~ISIG;               /* Disable interpretation of INTR, QUIT, SUSP characters */

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); /* Disable flow control */
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); /* Disable special handling of bytes */

    tty.c_oflag &= ~OPOST;              /* Prevent special interpretation of output bytes */
    tty.c_oflag &= ~ONLCR;              /* Prevent conversion of newline to carriage return/line feed */

    tty.c_cc[VMIN] = 0;                 /* Minimum number of characters to read */
    tty.c_cc[VTIME] = 2;                /* Timeout in 0.1s units (5 units = 0.5 seconds) */

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        return false;
    }

    pthread_t thread;

    pthread_create(&thread, NULL, serial_worker, NULL);
    pthread_detach(thread);

    return true;
}
