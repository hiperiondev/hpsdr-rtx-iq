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

#ifndef HermesProxy_H
#define HermesProxy_H

#include <stdbool.h>
#include <complex.h>

#define NUMRXIQBUFS    128 // number of receiver IQ buffers in circular queue.
                           // Must be integral power of 2 (2,4,8,16,32,64, etc.)
#define RXBUFSIZE      256 // number of floats in one RxIQBuf, #complexes is half
                           // Must be integral power of 2 (2,4,8,16,32,64, etc.)
#define NUMTXBUFS      128 // number of transmit buffers in circular queue
                           // Must be integral power of 2
#define TXBUFSIZE      512 // number of bytes in one TxBuf
#define TXINITIALBURST   4 // Number of Ethernet frames to holdoff before bursting
                           // to fill hardware TXFIFO
#define MAXRECEIVERS     8 // Maximum number of receivers defined by protocol specification

typedef unsigned char *RawBuf_t; // Raw transmit buffer type
    void HermesProxy_init(int RxFreq0, int RxFreq1, int RxFreq2, int RxFreq3, int RxFreq4, int RxFreq5, int RxFreq6, int RxFreq7, int TxFreq, int RxPre,
            int PTTModeSel, int PTTTxMute, int PTTRxMute, unsigned char TxDr, int RxSmp, const char *Intfc, const char *ClkS, int AlexRA, int AlexTA, int AlexHPF,
            int AlexLPF, int Verb, int NumRx, const char *MACAddr);
    void ScheduleTxFrame(unsigned long RxBufCount);
    void UpdateHermes(void);
    void BuildControlRegs(unsigned RegNum, RawBuf_t outbuf);
     int PutTxIQ(const complex float *in0, int nsamples);
RawBuf_t GetNextTxBuf(void);
    void SendTxIQ(void);
    void ReceiveRxIQ(unsigned char *inbuf);
    void Stop(void);
    void Start(void);
    void PrintRawBuf(RawBuf_t inbuf);

#endif

