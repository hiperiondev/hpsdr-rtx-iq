/*
 * newhpsdr - an implementation of the hpsdr protocol
 * Copyright (C) 2019 Sebastien Lorquet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "p1dev.h"

void enumerator(struct p1dev_s *dev, void *ctx) {
    printf("[%02X:%02X:%02X:%02X:%02X:%02X] %s - type %s [%d] (state %s [%d])\n", dev->mac[0], dev->mac[1], dev->mac[2], dev->mac[3], dev->mac[4], dev->mac[5],
            inet_ntoa(dev->data_addr.sin_addr), p1dev_describe_type(dev->type), dev->type, p1dev_describe_state(dev->state), dev->state);
}

int main(int argc, char **argv) {
    unsigned int delay = 3;

    if (argc == 2) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            printf("hpsdr_enum\n"
                    "  -h --help    Show this help\n"
                    "  <delay>      Enumeration timeout in seconds, min 1, max 100, default 3\n");
            return 0;
        }
        delay = strtol(argv[1], NULL, 10);
        if (delay == 0) {
            fprintf(stderr, "Incorrect number\n");
            return 1;
        }
        if (delay > 100) {
            fprintf(stderr, "Incorrect delay\n");
            return 1;
        }
    }

    printf("Start of enumeration, timeout = %u s\n", delay);

    p1dev_discover_async(enumerator, NULL, delay);

    printf("End of enumeration\n");

    return 0;
}
