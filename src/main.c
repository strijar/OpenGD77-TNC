/*
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *
 *  OpenGD77 TNC
 *
 *  Copyright (c) 2026 Belousov Oleg aka R1CBU
 */

#include <unistd.h>

#include "serial.h"

int main(int argc, char *argv[]) {
    if (serial_init()) {
        while (true) {
            usleep(100000);
        }
    }

    return 0;
}
