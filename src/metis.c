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
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <netpacket/packet.h>

#include "hermes_proxy.h"
#include "metis.h"

#define MAX_METIS_CARDS 10
METIS_CARD metis_cards[MAX_METIS_CARDS];

#define DISCOVER_IDLE 0
#define DISCOVER_SENT 1

#define PORT 1024
#define DISCOVERY_SEND_PORT PORT
#define DISCOVERY_RECEIVE_PORT PORT
#define DATA_PORT PORT

static int discovery_socket;
static struct sockaddr_in discovery_addr;
static int discovery_length;

static int discovering;

static unsigned char hw_address[6];
static long ip_address;

static struct sockaddr_in data_addr;
static int data_addr_length;

static long send_sequence = -1;
static unsigned char buffer[70];

static pthread_t receive_thread_id;
static int found = 0;

static unsigned char output_buffer[1032];
static int offset = 8;

int ep;
long sequence = -1;

#define inaddrr(x) (*(struct in_addr *) &ifr->x[sizeof sa.sin_port])

static int get_addr(int sock, const char *ifname) {

    struct ifreq *ifr;
    struct ifreq ifrr;
    struct sockaddr_in sa;
    struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa = NULL;

    int i;
    struct ifconf ifc;
    char buf[8192];
    struct ifreq *ifquery;
    int nInterfaces;
    //struct ifreq *item;

    /* Query available interfaces. */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0) {
        printf("ioctl(SIOCGIFCONF) error");
        return -1;
    }

    /* Iterate through the list of interfaces. */
    ifquery = ifc.ifc_req;
    nInterfaces = ifc.ifc_len / sizeof(struct ifreq);

    for (int i = 0; i < nInterfaces; i++)
        fprintf(stderr, "Interface[%d]:%s  ", i, (char*) &ifquery[i].ifr_name);

    fprintf(stderr, "\n");

    ifr = &ifrr;
    ifrr.ifr_addr.sa_family = AF_INET;
    strncpy(ifrr.ifr_name, ifname, sizeof(ifrr.ifr_name));

    if (ioctl(sock, SIOCGIFADDR, ifr) < 0) {
        printf("No %s interface.\n", ifname);
        return -1;
    }

    ip_address = inaddrr(ifr_addr.sa_data).s_addr;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
    } else {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if ((ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_PACKET)) {
                struct sockaddr_ll *s = (struct sockaddr_ll*) ifa->ifa_addr;
                printf("%-8s ", ifa->ifa_name);

                if (!strcmp(ifa->ifa_name, ifname))
                    found = true;

                for (i = 0; i < s->sll_halen; i++) {
                    printf("%02x%c", (s->sll_addr[i]), (i + 1 != s->sll_halen) ? ':' : '\n');
                    if (!strcmp(ifa->ifa_name, ifname)) {
                        hw_address[i] = (s->sll_addr[i]);

                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }

    if (!found) {
        printf("No %s interface.\n", ifname);
        return -1;
    }

    return 0;
}

void metis_discover(const char *interface) {
    int rc = 0;
    int i;
    int on = 1;

    fprintf(stderr, "Looking for Metis/Hermes card on interface %s\n", interface);

    discovering = 1;

    // send a broadcast to locate metis boards on the network
    discovery_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (discovery_socket < 0) {
        perror("create socket failed for discovery_socket\n");
        exit(1);
    }

    // get my MAC address and IP address
    if (get_addr(discovery_socket, interface) < 0) {
        exit(1);
    }

    printf("%s IP Address: %ld.%ld.%ld.%ld\n", interface, ip_address & 0xFF, (ip_address >> 8) & 0xFF, (ip_address >> 16) & 0xFF, (ip_address >> 24) & 0xFF);

    printf("%s MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n", interface, hw_address[0], hw_address[1], hw_address[2], hw_address[3], hw_address[4],
            hw_address[5]);

    // start a receive thread to get discovery responses
    rc = pthread_create(&receive_thread_id, NULL, metis_receive_thread, NULL);
    if (rc != 0) {
        fprintf(stderr, "pthread_create failed on metis_receive_thread: rc=%d\n", rc);
        exit(1);
    }
    sleep(1);

    // bind to this interface
    struct sockaddr_in name = { 0 };
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = ip_address;
    name.sin_port = htons(DISCOVERY_SEND_PORT);
    bind(discovery_socket, (struct sockaddr*) &name, sizeof(name));

    // allow broadcast on the socket
    rc = setsockopt(discovery_socket, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    if (rc != 0) {
        fprintf(stderr, "cannot set SO_BROADCAST: rc=%d\n", rc);
        exit(1);
    }
    rc = setsockopt(discovery_socket, SOL_SOCKET, SO_BINDTODEVICE, interface, strlen(interface));
    if (rc != 0) {
        fprintf(stderr, "cannot set SO_BINDTODEVICE: rc=%d\n", rc);
        exit(1);
    }

    discovery_length = sizeof(discovery_addr);
    memset(&discovery_addr, 0, discovery_length);
    discovery_addr.sin_family = AF_INET;
    discovery_addr.sin_port = htons(DISCOVERY_SEND_PORT);
    discovery_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    buffer[0] = 0xEF;
    buffer[1] = 0xFE;
    buffer[2] = 0x02;
    for (i = 0; i < 60; i++) {
        buffer[i + 3] = 0x00;
    }

    if (sendto(discovery_socket, buffer, 63, 0, (struct sockaddr*) &discovery_addr, discovery_length) < 0) {
        perror("sendto socket failed for discovery_socket\n");
        exit(1);
    }
}

int metis_found() {
    return found;
}

// close socket, stop receive thread, wait for thread to terminate
void metis_stop_receive_thread() {
    shutdown(discovery_socket, 2);
    pthread_cancel(receive_thread_id);
    pthread_join(receive_thread_id, NULL);
}

char* metis_ip_address(int entry) {
    if (entry >= 0 && entry < found) {
        return metis_cards[entry].ip_address;
    }
    return NULL;
}

char* metis_mac_address(int entry) {
    if (entry >= 0 && entry < found) {
        return metis_cards[entry].mac_address;
    }
    return NULL;
}

void metis_receive_stream_control(unsigned char streamControl, unsigned int entry) {
    int i;
    struct hostent *h;
    discovering = 0;

    h = gethostbyname(metis_cards[entry].ip_address);
    if (h == NULL) {
        fprintf(stderr, "metis_start_receiver_stream unknown target.  MAC: %s    IP: %s\n", metis_cards[entry].mac_address, metis_cards[entry].ip_address);
        exit(1);
    }

    data_addr_length = sizeof(data_addr);
    memset(&data_addr, 0, data_addr_length);
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(DATA_PORT);
    memcpy((char*) &data_addr.sin_addr.s_addr, h->h_addr_list[0], h->h_length);

    // send a packet to start or stop the stream
    buffer[0] = 0xEF;
    buffer[1] = 0xFE;
    buffer[2] = 0x04;    // data send state
    buffer[3] = streamControl;	// 0x0 = off, 0x01 = EP6 (NB data), 0x02 = (EP4) WB data, 0x03 = both on

    for (i = 0; i < 60; i++)
        buffer[i + 4] = 0x00;

    if (sendto(discovery_socket, buffer, 64, 0, (struct sockaddr*) &data_addr, data_addr_length) < 0) {
        perror("sendto socket failed for start\n");
        exit(1);
    }

    if (streamControl == 0)
        send_sequence = -1;	// reset HPSDR Tx Ethernet sequence number on stream stop
}

int metis_write(unsigned char ep, unsigned char *buffer, int length) {
    int i;

    if (offset == 8) {
        send_sequence++;
        output_buffer[0] = 0xEF;
        output_buffer[1] = 0xFE;
        output_buffer[2] = 0x01;
        output_buffer[3] = ep;
        output_buffer[4] = (send_sequence >> 24) & 0xFF;
        output_buffer[5] = (send_sequence >> 16) & 0xFF;
        output_buffer[6] = (send_sequence >> 8) & 0xFF;
        output_buffer[7] = (send_sequence) & 0xFF;

        // copy the buffer over
        for (i = 0; i < 512; i++) {
            output_buffer[i + offset] = buffer[i];
        }
        offset = 520;
    } else {
        // copy the buffer over
        for (i = 0; i < 512; i++) {
            output_buffer[i + offset] = buffer[i];
        }
        offset = 8;

        // send the buffer
        metis_send_buffer(&output_buffer[0], 1032);

    }

    return length;
}

void metis_send_buffer(unsigned char *buffer, int length) {
    if (sendto(discovery_socket, buffer, length, 0, (struct sockaddr*) &data_addr, data_addr_length) != length) {
        perror("sendto socket failed for metis_send_data\n");
        exit(1);
    }
}

void* metis_receive_thread(void *arg) {
    printf("Start metis receive thread\n");
    struct sockaddr_in addr;
    int length;
    unsigned char buffer[2048];
    int bytes_read;

    length = sizeof(addr);
    while (1) {
        bytes_read = recvfrom(discovery_socket, buffer, sizeof(buffer), 0, (struct sockaddr*) &addr, (socklen_t*) &length);
        if (bytes_read < 0) {
            if (errno == EINTR)  // new code to handle case of signal received
                continue;

            printf("recvfrom socket failed for metis_receive_thread\n");
            exit(1);
        }

        printf("Received packet\n");
        if (bytes_read == 0)
            continue;

        if (bytes_read > 1048)
            fprintf(stderr, "Metis Receive Thread: bytes_read = %d  (>1048)\n", bytes_read);

        if (buffer[0] == 0xEF && buffer[1] == 0xFE) {
            switch (buffer[2]) {
            case 1:
                if (!discovering) {
                    // get the end point
                    ep = buffer[3] & 0xFF;

                    // get the sequence number
                    sequence = ((buffer[4] & 0xFF) << 24) + ((buffer[5] & 0xFF) << 16) + ((buffer[6] & 0xFF) << 8) + (buffer[7] & 0xFF);
                    switch (ep) {
                    case 6: // EP6: Send to Hermes Narrowband
                        // process the data
                        if (bytes_read != 1032)
                            fprintf(stderr, "Metis: bytes_read = %d (!= 1032)\n", bytes_read);
                        ReceiveRxIQ(&buffer[0]); // send Ethernet frame to Proxy
                        break;

                    case 4: // EP4: Send to Hermes Wideband
                        ReceiveRxIQ(&buffer[0]); // send Ethernet frame to Proxy
                        break;

                    default:
                        fprintf(stderr, "unexpected EP %d length=%d\n", ep, bytes_read);
                        break;
                    }
                } else {
                    fprintf(stderr, "unexpected data packet when in discovery mode\n");
                }
                break;
            case 2:  // response to a discovery packet
                if (discovering) {
                    if (found < MAX_METIS_CARDS) {
                        // get MAC address from reply
                        sprintf(metis_cards[found].mac_address, "%02X:%02X:%02X:%02X:%02X:%02X", buffer[3] & 0xFF, buffer[4] & 0xFF, buffer[5] & 0xFF,
                                buffer[6] & 0xFF, buffer[7] & 0xFF, buffer[8] & 0xFF);
                        fprintf(stderr, "Metis MAC address %s\n", metis_cards[found].mac_address);

                        // get ip address from packet header
                        sprintf(metis_cards[found].ip_address, "%d.%d.%d.%d", addr.sin_addr.s_addr & 0xFF, (addr.sin_addr.s_addr >> 8) & 0xFF,
                                (addr.sin_addr.s_addr >> 16) & 0xFF, (addr.sin_addr.s_addr >> 24) & 0xFF);
                        fprintf(stderr, "Metis IP address %s\n", metis_cards[found].ip_address);
                        found++;
                    } else {
                        fprintf(stderr, "too many metis/Hermes cards!\n");
                    }
                } else {
                    fprintf(stderr, "unexepected discovery response when not in discovery mode\n");
                }
                break;
            default:
                fprintf(stderr, "unexpected packet type: 0x%02X\n", buffer[2]);
                break;
            }
        } else {
            fprintf(stderr, "received bad header bytes on data port %02X,%02X\n", buffer[0], buffer[1]);
        }
    }
}
