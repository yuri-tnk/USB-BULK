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

#include "LPC214x.h"
#include "../../tnkernel/tn.h"
#include "../../tnkernel/tn_port.h"
#include "tn_usb_hw.h"
#include "tn_usb.h"
#include "tn_usb_ep.h"


//----------------------------------------------------------------------------
void tn_usb_EP0_tx_int_func(USB_DEVICE_INFO * udi) //-- IN
{
   volatile unsigned int ep_status;
   USB_EP_STATUS * eps = &udi->EP0Status;

   ep_status = tn_usb_lpc_cmd_read(CMD_EP_SELECT_CLEAR | 1); //-- 1 - EP0TX

   tn_usb_st_DATAIN(eps);

   rUSBEpIntClr = 1<<1;
}

//----------------------------------------------------------------------------
void tn_usb_EP0_rx_int_func(USB_DEVICE_INFO * udi) //-- OUT
{
   int bytes;
   unsigned int ep_status;
   int rc;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;
   USB_EP_STATUS * eps = &udi->EP0Status;

   ep_status = tn_usb_lpc_cmd_read(CMD_EP_SELECT_CLEAR | 0); //-- 0 - EP0RX
   if(ep_status & STP)               //-- SETUP command arrived
   {
      tn_usb_ep_read(0,(BYTE *)usp); //-- Read SETUP request data

      eps->pdata = eps->pbuf;        //-- Uses EP0DataBuff
      eps->nbytes = usp->wLength;
      bytes = usp->wLength;

      if(usp->wLength == 0 || (usp->bmRequestType& 0x80))
      {
         rc = tn_usb_EP0_SETUP(udi);
         if(rc == FALSE)
            tn_usb_stall_ep(0,TRUE);
         else
         {
             if(eps->nbytes > usp->wLength)
                eps->nbytes = usp->wLength;
             tn_usb_st_DATAIN(eps);
         }
      }
   }
   else
   {
      if(eps->nbytes > 0)
      {
         bytes = tn_usb_ep_read(0,(BYTE*)eps->pdata);
         eps->nbytes -= bytes;
         eps->pdata  += bytes;
         if(eps->nbytes == 0)
         {
            eps->pdata  = eps->pbuf;
            rc = tn_usb_EP0_SETUP(udi);
            if(rc == FALSE)
               tn_usb_stall_ep(0,TRUE);
            else
               tn_usb_st_DATAIN(eps);  //-- if eps->nbytes == 0 - as Status In
         }
         else
            tn_usb_ep_read(0,NULL); //-- Here - dummy read
      }
   }

   rUSBEpIntClr = 1;  //--  Clear EP0 RX int
}

//----------------------------------------------------------------------------
void tn_usb_ep_rx_int(EP_INFO * ei) //-- OUT
{
   int rc;
   unsigned int len;
   unsigned int * ptr;
   unsigned int * ptr1;
   unsigned int ep_status;

   ep_status = tn_usb_lpc_cmd_read(CMD_EP_SELECT_CLEAR | ei->ep_num_phys);
   if((ep_status & ((1<<5)|(1<<6))) != 0) //-- In NAK int,there is not empty buf
   {
      rc = tn_fmem_get_ipolling(ei->mem_pool,(void **) &ptr);
      if(rc == TERR_NO_ERR)
      {
        //-- Read 
         rUSBCtrl = RD_EN | ((ei->ep_num_phys>>1) << 2);  //-- set read enable bit
         do
         {
            len = rUSBRxPLen;
         }while((len & PKT_RDY) == 0);
  
         len &= PKT_LNGTH_MASK;  // get length
  
       //--- Buf is word-aligned
         ptr1 = ptr;
         while(rUSBCtrl & RD_EN)
            *ptr1++ = rUSBRxData; 
  
         rUSBCtrl = 0;
   
         tn_usb_ep_rx_enable((ei->ep_num_phys));
  
         tn_queue_isend_polling(ei->queue,(void *)ptr);
      }
   }
   rUSBEpIntClr = 1 << ei->ep_num_phys;
}

//----------------------------------------------------------------------------
void tn_usb_ep_tx_int(EP_INFO * ei) //-- IN
{
   int rc;
   unsigned int * ptr;
   unsigned int * ptr1;

   rc = tn_queue_ireceive(ei->queue,(void **)&ptr);
   if(rc == TERR_NO_ERR)
   {
      rUSBCtrl = WR_EN | ((ei->ep_num_phys>>1) << 2); 

      rUSBTxPLen = USB_MAX_PACKET0;  

      ptr1 = ptr;
      while(rUSBCtrl & WR_EN)         //--- Buf is word-aligned
         rUSBTxData = *ptr1++;

      rUSBCtrl = 0;

      tn_usb_lpc_cmd(CMD_EP_SELECT | ei->ep_num_phys);
      tn_usb_lpc_cmd(CMD_EP_VALIDATE_BUFFER);
   
      tn_fmem_irelease(ei->mem_pool,(void*)ptr);
   }

   rUSBEpIntClr = 1 << ei->ep_num_phys;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
