/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <pthread.h>

#include "serial.h"
#include "mmdvm.h"

static int      fd;

static void * serial_worker(void *p) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    uint8_t buf[256];

    while (true) {
        int n = read(fd, &buf, sizeof(buf));

        if (n > 0) {
            mmdvm_recv(buf, n);
        } else {
            mmdvm_idle();
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
    tty.c_cc[VTIME] = 2;                /* Timeout in 0.1s units */

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        return false;
    }

    pthread_t thread;

    pthread_create(&thread, NULL, serial_worker, NULL);
    pthread_detach(thread);

    return true;
}

void serial_send(uint8_t *data, size_t size) {
    write(fd, data, size);
}
