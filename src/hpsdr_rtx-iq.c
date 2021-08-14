/*
 * Copyright 2021 Emiliano Gonzalez LU3VEA (lu3vea @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/hpsdr-rtx-iq *
 *
 * This is based on other projects:
 *    https://github.com/Tom-McDermott/gr-hpsdr/
 *
 *    please contact their authors for more information.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <arpa/inet.h>

#include "hermes_proxy.h"
#include "metis.h"

#define IQBURST 63

FILE *FileInHandle = NULL;
float IQBuffer[IQBURST * 2];
int nbread, i;
float complex in0[IQBURST];

int main(int argc, char **argv) {
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
    printf("found metis: %s %s\n", metis_ip_address(1), metis_mac_address(1));

    HermesProxy_init(
            0,                  // RxFreq0
            0,                  // RxFreq1
            0,                  // RxFreq2
            0,                  // RxFreq3
            0,                  // RxFreq4
            0,                  // RxFreq5
            0,                  // RxFreq6
            0,                  // RxFreq7
            147360000,          // TxFreq
            0,                  // RxPre
            0,                  // PTTModeSel
            0,                  // PTTTxMute
            0,                  // PTTRxMute
            0,                  // TxDr
            48000,              // RxSmp
            argv[1],            // Intfc
            "",                 // ClkS
            0,                  // AlexRA
            0,                  // AlexTA
            0,                  // AlexHPF
            0,                  // AlexLPF
            0,                  // Verb
            8,                  // NumRx
            "AA:BB:CC:DD:EE:FF" // MACAddr
            );

    printf("Start read stdin\n");
    Start();
    while (1) {
        nbread = fread(IQBuffer, sizeof(float), IQBURST * 2, stdin);
        if (nbread > 0) {
            for (i = 0; i < nbread / 2; i++) {
                in0[i] = IQBuffer[i * 2], IQBuffer[i * 2 + 1] * I;
            }
            PutTxIQ(in0, nbread / 2);
            ScheduleTxFrame(63);
        }
    }
    Stop();
    return 0;
}
