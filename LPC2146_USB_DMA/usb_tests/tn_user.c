/*
TNKernel real-time kernel - USB examples - LPC2146/8 + DMA

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


#include "../../tnkernel/tn.h"
#include "LPC214x.h"
#include "utils.h"
#include "tn_usb_hw.h"
#include "tn_usb.h"
#include "tn_usb_ep_dma.h"
#if defined (__GNUC__)
#include <string.h>
#endif


extern USB_DEVICE_INFO  gUSBInfo;

extern EP_DMA_INFO  gEP2RX_EDI;
extern EP_DMA_INFO  gEP2TX_EDI;
extern EP_DMA_INFO  gEP5RX_EDI;
extern EP_DMA_INFO  gEP5TX_EDI;

typedef void (*int_func)(void);

//----------------------------------------------------------------------------
void tn_cpu_irq_handler(void)
{
   int_func ifunc;
   ifunc = (int_func)VICVectAddr;
   if(ifunc != NULL)
      (*ifunc)();
}
//----------------------------------------------------------------------------
void tn_usb_int_func(void)
{
   unsigned int dev_status;
   unsigned int ep_int_status;

   if(rUSBDevIntSt & DEV_STAT) //-- USB Bus reset,Connect change,Suspend change
   {
      dev_status = tn_usb_lpc_cmd_read(CMD_DEV_STATUS);

      if(dev_status & RST)        //-- Bus reset
      {
         tn_usb_int_reset(&gUSBInfo);
      }
      if(dev_status & SUS_CH)     //-- Suspend/Resume toggle
      {
         if(dev_status & SUS)     //-- Suspend
            tn_usb_int_suspend();
         else
            tn_usb_int_resume();
      }
      if(dev_status & CON_CH)     //-- Connect changed
         tn_usb_int_con_ch(dev_status,&gUSBInfo); // & CON);

      rUSBDevIntClr = DEV_STAT;
   }

   if(rUSBDevIntSt & EP_SLOW)     //-- Endpoints (here -only EP0)
   {
      ep_int_status = rUSBEpIntSt;

      //-- EP0 - Control
      if(ep_int_status & 1)              //-- Rx
         tn_usb_EP0_rx_int_func(&gUSBInfo); //-- OUT

      else if(ep_int_status & (1<<1))    //-- Tx
         tn_usb_EP0_tx_int_func(&gUSBInfo); //-- IN

      rUSBDevIntClr = EP_SLOW; // clear EP_SLOW
   }

   //------------- DMA -------------------

   if(rUSBDMAIntSt & 4) //-- Err
   {
      ep_int_status = rUSBEoTIntSt;
   }

   if(rUSBDMAIntSt & 1)  //-- End_of_transfer
   {
      ep_int_status = rUSBEoTIntSt;

      if(ep_int_status & (1<<EP2_RX))
         ep_rx_end_of_transfer(&gEP2RX_EDI);

      if(ep_int_status & (1<<EP2_TX))
         rUSBEoTIntClr = (1 << EP2_TX);  //-- Clear int bit

      if(ep_int_status & (1<<EP5_RX))
         ep_rx_end_of_transfer(&gEP5RX_EDI);

      if(ep_int_status & (1<<EP5_TX))
         rUSBEoTIntClr = (1 << EP5_TX);  //-- Clear int bit
   }

   //--DMA - DD_request
   if(rUSBDMAIntSt & 2)  //-- DD_request
   {
      ep_int_status = rUSBNDDRIntSt;

      if(ep_int_status & (1<<EP2_RX))
         ep_rx_DD_request(&gEP2RX_EDI);

      if(ep_int_status & (1<<EP2_TX))
         ep_tx_int_DD_request(&gEP2TX_EDI);

      if(ep_int_status & (1<<EP5_RX))
         ep_rx_DD_request(&gEP5RX_EDI);

      if(ep_int_status & (1<<EP5_TX))
         ep_tx_int_DD_request(&gEP5TX_EDI);
   }

   VICVectAddr  = 0;
}

//----------------------------------------------------------------------------
void tn_timer0_int_func(void)
{
   rT0IR = 0xFF;                //-- clear int source

   tn_tick_int_processing();    //-- OS timer ticks

   VICVectAddr = 0xFF;
}

//----------------------------------------------------------------------------
void tn_uart0_int_func(void)  //-- Not uses here
{
   volatile int data;

   data = rU0IIR;               //-- clear int source

   VICVectAddr = 0xFF;
}

//----------------------------------------------------------------------------
void tn_int_default_func(void)
{
   VICVectAddr = 0xFF;
}

//----------------------------------------------------------------------------
// Processor specific routine
//
// For LPC2xxx, here we enable all int vectors that we use ( not just tick timer)
// and than enable IRQ int in ARM core
//
// In LPC214x USB example we need just enable IRQ int in ARM core
//----------------------------------------------------------------------------
void tn_cpu_int_enable()
{
   tn_arm_enable_interrupts();
}








