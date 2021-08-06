/* 
 * From https://github.com/Tom-McDermott/gr-hpsdr/
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

    void ScheduleTxFrame(unsigned long RxBufCount);
    void UpdateHermes(void);
    void BuildControlRegs(unsigned RegNum, RawBuf_t outbuf);
     int PutTxIQ(const complex float *in0, int nsamples);
RawBuf_t GetNextTxBuf(void);
    void SendTxIQ(void);

#endif

