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

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "hermes_proxy.h"
#include "metis.h"


int main(int argc, char **argv) {
    int n;
    if (argc == 2) {
        if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            printf("hpsdr_enum\n"
                    "  -h --help    Show this help\n"
                    "  interface    name of ethernet interface\n");
            return 0;
        }
    }

    metis_discover(argv[1]);

    sleep(5);
    for (n = 0; n <= metis_found(); n++)
        printf("found metis(%d): %s %s\n", n, metis_ip_address(n), metis_mac_address(n));

    return 0;
}
