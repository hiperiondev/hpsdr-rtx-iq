/*
 * Copyright 2021 Emiliano Gonzalez LU3VEA (lu3vea @ gmail . com))
 * * Project Site: https://github.com/hiperiondev/hpsdr-rpitx *
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

#ifndef METIS_H
#define METIS_H

enum {
    RxStream_Off,		// Hermes Receiver Stream Controls
    RxStream_NB_On,		// Narrow Band (down converted)
    RxStream_WB_On,		// Wide Band (raw ADC samples)
    RxStream_NBWB_On	// Narrow Band and Wide Band both On
};

typedef struct _METIS_CARD {
    char ip_address[16];
    char mac_address[18];
} METIS_CARD;

void metis_discover(const char *interface);
 int metis_found();
char *metis_ip_address(int entry);
char *metis_mac_address(int entry);
void metis_receive_stream_control(unsigned char, unsigned int);
void metis_stop_receive_thread();
 int metis_write(unsigned char ep, unsigned char *buffer, int length);
void metis_send_buffer(unsigned char *buffer, int length);
void *metis_receive_thread(void *arg);

#endif  // METIS_H

