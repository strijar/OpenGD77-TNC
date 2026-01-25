/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "cap.h"

static int fd;

bool cap_open(bool create) {
    fd = open("cap.dat", create ? O_WRONLY | O_CREAT : O_RDONLY, 0644);

    return fd > 0;
}

void cap_close() {
    close(fd);
}

void cap_write(uint8_t *data, size_t size) {
    write(fd, data, size);
}

bool cap_read(uint8_t *data, size_t size) {
    read(fd, data, size) == size;
}
