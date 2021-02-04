/*
TNKernel real-time kernel - USB examples

Copyright © 2004,2006 Yuri Tiomkin
All rights reserved.

Permission to use, copy, modify, and distribute this software in source
and binary forms and its documentation for any purpose and without fee
is hereby granted, provided that the above copyright notice appear
in all copies and that both that copyright notice and this permission
notice appear in supporting documentation.

THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS ``AS IS'' AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
*/


/*============================================================================
  usb_tst2.c

  Base test for TNKernel USB + LPC2146/8 - Using DMA

    Here we create 2 bidirectional bulk pipes using LPC214x's Endpoint 2 and
  Endpoint 5.

    Each physical endpoint has own task to operate. Data are
  transmitted/received in 64-bytes packets only.

    Endpoints EP2_rx and EP5_tx are used for loop-back data transferring
  (data  that EP2_rx receives are sent to EP5_tx for transmitting by
   queue 'queueEPExch')

  See remarks about USB data transfer performance on http:/www.tnkernel.com.

    Remark: In this example OS time tick period is 10 mS.
*===========================================================================*/

#include "LPC214x.h"
#include "../../tnkernel/tn.h"
#include "../../tnkernel/tn_port.h"
#include "utils.h"
#include "tn_usb_hw.h"
#include "tn_usb.h"
#include "tn_usb_ep_dma.h"


//----------- Tasks ----------------------------------------------------------

#define  TASK_PROCESSING_PRIORITY    5
#define  TASK_EP2_RX_PRIORITY        9
#define  TASK_EP2_TX_PRIORITY        8
#define  TASK_EP5_RX_PRIORITY        7
#define  TASK_EP5_TX_PRIORITY        6  // 9
#define  TASK_IO_PRIORITY           11

#define  TASK_PROCESSING_STK_SIZE   128
#define  TASK_EP2_RX_STK_SIZE       128
#define  TASK_EP2_TX_STK_SIZE       128
#define  TASK_EP5_RX_STK_SIZE       128
#define  TASK_EP5_TX_STK_SIZE       128
#define  TASK_IO_STK_SIZE           128

TN_TCB  task_processing;
TN_TCB  task_ep2_rx;
TN_TCB  task_ep2_tx;
TN_TCB  task_ep5_rx;
TN_TCB  task_ep5_tx;
TN_TCB  task_io;

unsigned int task_processing_stack[TASK_PROCESSING_STK_SIZE];
unsigned int task_ep2_rx_stack[TASK_EP2_RX_STK_SIZE];
unsigned int task_ep2_tx_stack[TASK_EP2_TX_STK_SIZE];
unsigned int task_ep5_rx_stack[TASK_EP5_RX_STK_SIZE];
unsigned int task_ep5_tx_stack[TASK_EP5_TX_STK_SIZE];
unsigned int task_io_stack[TASK_IO_STK_SIZE];

void task_processing_func(void * par);
void task_ep2_rx_func(void * par);
void task_ep2_tx_func(void * par);
void task_ep5_rx_func(void * par);
void task_ep5_tx_func(void * par);
void task_io_func(void * par);


//------- Queues ----------------------------

#define  QUEUE_EP2RX_SIZE     (EP2_RX_NUM_BUF)
#define  QUEUE_EP2TX_SIZE     (EP2_TX_NUM_BUF-1)
#define  QUEUE_EP5RX_SIZE     (EP5_RX_NUM_BUF)
#define  QUEUE_EP5TX_SIZE     (EP5_TX_NUM_BUF-1)
#define  QUEUE_EPEXCH_SIZE      16

   //--- EP2 RX queue
TN_DQUE  queueEP2RX_DD_rx;
void     * queueEP2RX_DD_rxMem[QUEUE_EP2RX_SIZE];

   //--- EP2 TX queue
TN_DQUE  queueEP2TX_tx;
void     * queueEP2TX_txMem[QUEUE_EP2TX_SIZE];

   //--- EP5 RX queue
TN_DQUE  queueEP5RX_DD_rx;
void     * queueEP5RX_DD_rxMem[QUEUE_EP5RX_SIZE];

   //--- EP5 TX queue
TN_DQUE  queueEP5TX_tx;
void     * queueEP5TX_txMem[QUEUE_EP5TX_SIZE];

   //---  tx/rx exch queue
TN_DQUE  queueEPExch;
void     * queueEPExchMem[QUEUE_EPEXCH_SIZE];


//------ Fixed-Sized Memory Pool ----------------


//-- Force item size align  to 4 - not needs in this case
#define USBBulkBufItemSize  USB_MAX_PACKET0  // MAKE_ALIG(USB_MAX_PACKET0)
//-- item size - in sizeof(int) units

TN_FMP EP2RXMemPool;
unsigned int memEP2RXMemPool[QUEUE_EP2RX_SIZE * (USBBulkBufItemSize / sizeof(int))];

TN_FMP EP2TXMemPool;
unsigned int memEP2TXMemPool[QUEUE_EP2TX_SIZE * (USBBulkBufItemSize / sizeof(int))];

TN_FMP EP5RXMemPool;
unsigned int memEP5RXMemPool[QUEUE_EP5RX_SIZE * (USBBulkBufItemSize / sizeof(int))];

TN_FMP EP5TXMemPool;
unsigned int memEP5TXMemPool[QUEUE_EP5TX_SIZE * (USBBulkBufItemSize / sizeof(int))];

//---

TN_FMP EPExchMemPool;
unsigned int memEPExchMemPool[QUEUE_EPEXCH_SIZE * (USBBulkBufItemSize / sizeof(int))];

//--- Non OS globals --------------------

   //-- USB

USB_DEVICE_INFO  gUSBInfo;
BYTE             gEP0Buf[USB_MAX_PACKET0];

EP_DMA_INFO  gEP2RX_EDI;
EP_DMA_INFO  gEP2TX_EDI;
EP_DMA_INFO  gEP5RX_EDI;
EP_DMA_INFO  gEP5TX_EDI;


extern const BYTE abDescriptors[];
extern const EP_DMA_ADDR gDMAAddresses[];


char * s_itoa(char * buffer, int i);

//----------------------------------------------------------------------------
int main()
{
   tn_arm_disable_interrupts();

 //  rMEMMAP = 0x1;  //-- Flash Build

   HardwareInit();

   //--- USB global data struct init ---

   gUSBInfo.Descriptors  = (BYTE*)&abDescriptors[0];   //-- Descriptors
   gUSBInfo.EP0Status.pbuf  = &gEP0Buf[0];             //-- EP0 buffer
   tn_usb_reset_data(&gUSBInfo);

   //----- USB Endpoints data structs init ---

   gEP2RX_EDI.ep_dma_addr = (EP_DMA_ADDR *)&gDMAAddresses[EP2_RX];
   gEP2RX_EDI.dd_ptr = NULL;
   gEP2RX_EDI.pUDCA_pos = (unsigned int*)(UDCA_BASE + EP2_RX * sizeof(int));
   gEP2RX_EDI.queue       = &queueEP2RX_DD_rx;
   gEP2RX_EDI.mem_pool    = &EP2RXMemPool;
   gEP2RX_EDI.ep_num_phys = EP2_RX;

   gEP2TX_EDI.ep_dma_addr = (EP_DMA_ADDR *)&gDMAAddresses[EP2_TX];
   gEP2TX_EDI.dd_ptr = NULL;
   gEP2TX_EDI.pUDCA_pos = (unsigned int*)(UDCA_BASE + EP2_TX * sizeof(int));
   gEP2TX_EDI.queue       = &queueEP2TX_tx;
   gEP2TX_EDI.mem_pool    = &EP2TXMemPool;
   gEP2TX_EDI.ep_num_phys = EP2_TX;

   gEP5RX_EDI.ep_dma_addr = (EP_DMA_ADDR *)&gDMAAddresses[EP5_RX];
   gEP5RX_EDI.dd_ptr = NULL;
   gEP5RX_EDI.pUDCA_pos = (unsigned int*)(UDCA_BASE + EP5_RX * sizeof(int));
   gEP5RX_EDI.queue       = &queueEP5RX_DD_rx;
   gEP5RX_EDI.mem_pool    = &EP5RXMemPool;
   gEP5RX_EDI.ep_num_phys = EP5_RX;

   gEP5TX_EDI.ep_dma_addr = (EP_DMA_ADDR *)&gDMAAddresses[EP5_TX];
   gEP5TX_EDI.dd_ptr = NULL;
   gEP5TX_EDI.pUDCA_pos = (unsigned int*)(UDCA_BASE + EP5_TX * sizeof(int));
   gEP5TX_EDI.queue       = &queueEP5TX_tx;
   gEP5TX_EDI.mem_pool    = &EP5TXMemPool;
   gEP5TX_EDI.ep_num_phys = EP5_TX;

   tn_usb_dma_rx_init(&gEP2RX_EDI);
   tn_usb_dma_tx_init(&gEP2TX_EDI);
   tn_usb_dma_rx_init(&gEP5RX_EDI);
   tn_usb_dma_tx_init(&gEP5TX_EDI);
//----------------------------------------

   tn_start_system(); //-- Never returns

   return 1;
}

//----------------------------------------------------------------------------
void  tn_app_init()
{

   //--- Task processing

   task_processing.id_task = 0;
   tn_task_create(&task_processing,              //-- task TCB
                 task_processing_func,           //-- task function
                 TASK_PROCESSING_PRIORITY,       //-- task priority
                 &(task_processing_stack         //-- task stack first addr in memory
                    [TASK_PROCESSING_STK_SIZE-1]),
                 TASK_PROCESSING_STK_SIZE,       //-- task stack size (in int,not bytes)
                 NULL,                           //-- task function parameter
                 TN_TASK_START_ON_CREATION       //-- Creation option
                 );

   //--- Task IO
   task_io.id_task = 0;
   tn_task_create(&task_io,                      //-- task TCB
                 task_io_func,                   //-- task function
                 TASK_IO_PRIORITY,               //-- task priority
                 &(task_io_stack                 //-- task stack first addr in memory
                    [TASK_IO_STK_SIZE-1]),
                 TASK_IO_STK_SIZE,               //-- task stack size (in int,not bytes)
                 NULL,                           //-- task function parameter
                 TN_TASK_START_ON_CREATION       //-- Creation option
                 );

   //-- Task USB EP2 rx
   task_ep2_rx.id_task = 0;
   tn_task_create(&task_ep2_rx,                  //-- task TCB
                 task_ep2_rx_func,               //-- task function
                 TASK_EP2_RX_PRIORITY,           //-- task priority
                 &(task_ep2_rx_stack             //-- task stack first addr in memory
                    [TASK_EP2_RX_STK_SIZE-1]),
                 TASK_EP2_RX_STK_SIZE,           //-- task stack size (in int,not bytes)
                 NULL,                           //-- task function parameter
                 TN_TASK_START_ON_CREATION       //-- Creation option
                 );

   //-- Task USB EP2 tx
   task_ep2_tx.id_task = 0;
   tn_task_create(&task_ep2_tx,                  //-- task TCB
                 task_ep2_tx_func,               //-- task function
                 TASK_EP2_TX_PRIORITY,           //-- task priority
                 &(task_ep2_tx_stack             //-- task stack first addr in memory
                    [TASK_EP2_TX_STK_SIZE-1]),
                 TASK_EP2_TX_STK_SIZE,           //-- task stack size (in int,not bytes)
                 NULL,                           //-- task function parameter
                 TN_TASK_START_ON_CREATION       //-- Creation option
                 );

   //-- Task USB EP5 rx
   task_ep5_rx.id_task = 0;
   tn_task_create(&task_ep5_rx,                  //-- task TCB
                 task_ep5_rx_func,               //-- task function
                 TASK_EP5_RX_PRIORITY,           //-- task priority
                 &(task_ep5_rx_stack             //-- task stack first addr in memory
                    [TASK_EP5_RX_STK_SIZE-1]),
                 TASK_EP5_RX_STK_SIZE,           //-- task stack size (in int,not bytes)
                 NULL,                           //-- task function parameter
                 TN_TASK_START_ON_CREATION       //-- Creation option
                 );

   //-- Task USB EP5 tx
   task_ep5_tx.id_task = 0;
   tn_task_create(&task_ep5_tx,                  //-- task TCB
                 task_ep5_tx_func,               //-- task function
                 TASK_EP5_TX_PRIORITY,           //-- task priority
                 &(task_ep5_tx_stack             //-- task stack first addr in memory
                    [TASK_EP5_TX_STK_SIZE-1]),
                 TASK_EP5_TX_STK_SIZE,           //-- task stack size (in int,not bytes)
                 NULL,                           //-- task function parameter
                 TN_TASK_START_ON_CREATION       //-- Creation option
                 );

  //--- Queues --------------------------------
   queueEP2RX_DD_rx.id_dque = 0;
   tn_queue_create(&queueEP2RX_DD_rx,        //-- Ptr to already existing TN_DQUE
                   &queueEP2RX_DD_rxMem[0],  //-- Ptr to already existing array of void * to store data queue entries.Can be NULL
                   QUEUE_EP2RX_SIZE    //-- Capacity of data queue(num entries).Can be 0
                   );
   queueEP2TX_tx.id_dque = 0;
   tn_queue_create(&queueEP2TX_tx,        //-- Ptr to already existing TN_DQUE
                   &queueEP2TX_txMem[0],  //-- Ptr to already existing array of void * to store data queue entries.Can be NULL
                   QUEUE_EP2TX_SIZE    //-- Capacity of data queue(num entries).Can be 0
                   );
  //--
   queueEP5RX_DD_rx.id_dque = 0;
   tn_queue_create(&queueEP5RX_DD_rx,        //-- Ptr to already existing TN_DQUE
                   &queueEP5RX_DD_rxMem[0],  //-- Ptr to already existing array of void * to store data queue entries.Can be NULL
                   QUEUE_EP5RX_SIZE    //-- Capacity of data queue(num entries).Can be 0
                   );
   queueEP5TX_tx.id_dque = 0;
   tn_queue_create(&queueEP5TX_tx,        //-- Ptr to already existing TN_DQUE
                   &queueEP5TX_txMem[0],  //-- Ptr to already existing array of void * to store data queue entries.Can be NULL
                   QUEUE_EP5TX_SIZE    //-- Capacity of data queue(num entries).Can be 0
                   );

   queueEPExch.id_dque = 0;
   tn_queue_create(&queueEPExch,        //-- Ptr to already existing TN_DQUE
                   &queueEPExchMem[0],  //-- Ptr to already existing array of void * to store data queue entries.Can be NULL
                   QUEUE_EPEXCH_SIZE    //-- Capacity of data queue(num entries).Can be 0
                   );

 //--- Fixed-sized memory pool

   EP2RXMemPool.id_fmp = 0;
   tn_fmem_create(&EP2RXMemPool,
                     (void *)&memEP2RXMemPool[0], // start_addr
                     USBBulkBufItemSize,
                     QUEUE_EP2RX_SIZE
                    );

   EP2TXMemPool.id_fmp = 0;
   tn_fmem_create(&EP2TXMemPool,
                     (void *)&memEP2TXMemPool[0], // start_addr
                     USBBulkBufItemSize,
                     QUEUE_EP2TX_SIZE
                    );

   EP5RXMemPool.id_fmp = 0;
   tn_fmem_create(&EP5RXMemPool,
                     (void *)&memEP5RXMemPool[0], // start_addr
                     USBBulkBufItemSize,
                     QUEUE_EP5RX_SIZE
                    );

   EP5TXMemPool.id_fmp = 0;
   tn_fmem_create(&EP5TXMemPool,
                     (void *)&memEP5TXMemPool[0], // start_addr
                     USBBulkBufItemSize,
                     QUEUE_EP5TX_SIZE
                    );

   EPExchMemPool.id_fmp = 0;
   tn_fmem_create(&EPExchMemPool,
                     (void *)&memEPExchMemPool[0], // start_addr
                     USBBulkBufItemSize,
                     QUEUE_EPEXCH_SIZE
                    );
}

//----------------------------------------------------------------------------
void task_processing_func(void * par)
{
   tn_usb_connect(TRUE);  //-- Connect USB - here

   for(;;)
   {
      tn_task_sleep(10);
   }
}

//----------------------------------------------------------------------------
void task_ep2_rx_func(void * par)
{
   int rc;
   unsigned char * ptr;
   unsigned char * ptr1;

   for(;;)
   {
      rc = tn_queue_receive(&queueEP2RX_DD_rx,(void **)&ptr1,TN_WAIT_INFINITE);
      if(rc == TERR_NO_ERR)
      {
         //-- Processing data from buf --
          rc = tn_fmem_get(&EPExchMemPool,(void **)&ptr,TN_WAIT_INFINITE);
          if(rc == TERR_NO_ERR)
          {
             s_memcpy(ptr,ptr1,64);
             tn_queue_send(&queueEPExch,(void *)ptr,TN_WAIT_INFINITE);
          }
         //------------------------------
          tn_fmem_release(&EP2RXMemPool,(void*)ptr1);
          rUSBEpDMAEn = (1 << EP2_RX); //-- Enable ep DMA int
      }
   }
}

//----------------------------------------------------------------------------
void task_ep2_tx_func(void * par)
{
   char * ptr;
   int rc;
   int i;
   volatile int dly;
   int cnt = 0;
   for(;;)
   {
      rc = tn_fmem_get(&EP2TXMemPool,(void **)&ptr,TN_WAIT_INFINITE);
      if(rc == TERR_NO_ERR)
      {
         for(dly=0;dly<40;dly++); //-- Small delay before 'ptr' access

    //---- Fill buf by user's data ---------
         s_memset(ptr,'0',12);
         s_itoa(ptr,cnt);
         for(i=12;i<62;i++)
            ptr[i] = ' ';
         ptr[62] = '\r';
         ptr[63] = '\n';

         cnt++;
    //-------------------------------------
         tn_queue_send(&queueEP2TX_tx,(void *)ptr,TN_WAIT_INFINITE);
      }
   }
}

//----------------------------------------------------------------------------
void task_ep5_rx_func(void * par)
{
   int rc;
   unsigned char * ptr;
   volatile int dly;

   char dbg_buf[64];

   for(;;)
   {
      rc = tn_queue_receive(&queueEP5RX_DD_rx,(void **)&ptr,TN_WAIT_INFINITE);
      if(rc == TERR_NO_ERR)
      {
         for(dly =0;dly <40;dly++);  //-- Short delay before 'ptr' access

         //-- Processing data from buf --
         s_memcpy(dbg_buf,ptr,64);        //-- To check (by debbugger) rx data
         //-------------------------------
         tn_fmem_release(&EP5RXMemPool,(void*)ptr);
         rUSBEpDMAEn = (1 << EP5_RX); //-- Enable Rx int
      }
   }
}

//----------------------------------------------------------------------------
void task_ep5_tx_func(void * par)
{
   char * ptr;
   char * ptr1;
   int rc;
   for(;;)
   {
      rc = tn_queue_receive(&queueEPExch,(void **)&ptr1,TN_WAIT_INFINITE);
      if(rc == TERR_NO_ERR)
      {
      //--- Tx --
         rc = tn_fmem_get(&EP5TXMemPool,(void **)&ptr,TN_WAIT_INFINITE);
         if(rc == TERR_NO_ERR)
         {
             s_memcpy(ptr,ptr1,64);
           //-------
             tn_queue_send(&queueEP5TX_tx,(void *)ptr,TN_WAIT_INFINITE);
         }
         tn_fmem_release(&EPExchMemPool,(void*)ptr1);
      }
   }
}

//----------------------------------------------------------------------------
void task_io_func(void * par)
{
   for(;;)
   {
      tn_task_sleep(10);
      //-- if no configuration then  Blink LED
      if(gUSBInfo.Configuration == 0)
      {
         rIO0SET |= (1<<14);
         tn_task_sleep(10);
         rIO0CLR |= (1<<14);
      }
   }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



