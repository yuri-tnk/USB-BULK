/*
TNKernel real-time kernel - USB examples (LPC2146/8 DMA)

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
#include "../../tnkernel/tn_utils.h"
#include "utils.h"
#include "tn_usb_hw.h"
#include "tn_usb.h"
#include "tn_usb_ep_dma.h"

#define  DMA_EP_BULK_WORD_1  (((EP_MAX_PACKET_SIZE)<<16) | \
                             ((EP_MAX_PACKET_SIZE)<<5))


/* ----------- USB RAM map --------------------------------------------------

  USB RAM start ->  UDCA
                    EP2 rx DDs
                    EP2 rx data buffers
                    EP2 tx DDs
                    EP2 tx data buffers
                    EP5 rx DDs
                    EP5 rx data buffers
                    EP5 tx DDs
                    EP5 tx data buffers
----------------------------------------------------------------------------*/
//-- Here  '4' means  sizeof(int)
#define  EP2_RX_DD       ((UDCA_BASE) + (32*4))      //-- Immediately after UDCA
#define  EP2_RX_DATABUF  ((EP2_RX_DD) + (EP_DD_TOTAL_SIZE)*4*(EP2_RX_NUM_BUF))
#define  EP2_TX_DD       ((EP2_RX_DATABUF) +  (EP_MAX_PACKET_SIZE)*(EP2_RX_NUM_BUF))
#define  EP2_TX_DATABUF  ((EP2_TX_DD) + (EP_DD_TOTAL_SIZE)*4*(EP2_RX_NUM_BUF))
#define  EP5_RX_DD       ((EP2_TX_DATABUF) + (EP_MAX_PACKET_SIZE)*(EP2_TX_NUM_BUF))
#define  EP5_RX_DATABUF  ((EP5_RX_DD) + (EP_DD_TOTAL_SIZE)*4*(EP5_RX_NUM_BUF))
#define  EP5_TX_DD       ((EP5_RX_DATABUF) + (EP_MAX_PACKET_SIZE)*(EP5_RX_NUM_BUF))
#define  EP5_TX_DATABUF  ((EP5_TX_DD) + (EP_DD_TOTAL_SIZE)*4*(EP5_RX_NUM_BUF))


const EP_DMA_ADDR gDMAAddresses[] =
{
   {0,0,0,0,0}, //-- EP0  Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP1  Rx,
   {0,0,0,0,0}, //-- -//- Tx

   //-- EP2 Rx - 4
   {DMA_EP_BULK_WORD_1,
   EP2_RX_NUM_BUF,      //-- word 1 DD(configuration)
   EP2_RX_DD,      //-- pDD - start addr in USB RAM for DD group
   EP2_RX_DATABUF, //-- p_data_buffers - start addr in USB RAM
   0},                   //-- For isochronous exch only
   //-- EP2 Rx - 5
   {DMA_EP_BULK_WORD_1,
   EP2_TX_NUM_BUF,      //-- Configuration  bits[15..0] - n buffers
   EP2_TX_DD,      //-- pEP_DD - start addr in USB RAM for DD group
   EP2_TX_DATABUF, //-- pEP_data_buffers - start addr in USB RAM
   0},                   //-- For isochronous exch only

   {0,0,0,0,0}, //-- EP3  Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP4  Rx,
   {0,0,0,0,0}, //-- -//- Tx

   //-- EP5 Rx - 10
   {DMA_EP_BULK_WORD_1,
   EP5_RX_NUM_BUF,      //-- Configuration  bits[15..0] - n buffers
   EP5_RX_DD,      //-- pEP_DD - start addr in USB RAM for DD group
   EP5_RX_DATABUF, //-- pEP_data_buffers - start addr in USB RAM
   0},                   //-- For isochronous exch only
   //-- EP5 Rx - 11
   {DMA_EP_BULK_WORD_1,
   EP5_TX_NUM_BUF,      //-- Configuration  bits[15..0] - n buffers
   EP5_TX_DD,      //-- pEP_DD - start addr in USB RAM for DD group
   EP5_TX_DATABUF, //-- pEP_data_buffers - start addr in USB RAM
   0},                   //-- For isochronous exch only

   {0,0,0,0,0}, //-- EP6  Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP7  Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP8  Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP9  Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP10 Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP11 Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP12 Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP13 Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP14 Rx,
   {0,0,0,0,0}, //-- -//- Tx
   {0,0,0,0,0}, //-- EP15 Rx,
   {0,0,0,0,0}  //-- -//- Tx
};

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
   if(ep_status & STP) //-- SETUP command arrived
   {
      tn_usb_ep_read(0,(BYTE *)usp); //-- Read SETUP request data

      eps->pdata = eps->pbuf; //-- Uses EP0DataBuff
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
void ep_rx_end_of_transfer(EP_DMA_INFO * edi)
{
   int rc;
   unsigned char * ptr;
   unsigned int * dd_ptr;
   unsigned int buf;

   rc = tn_fmem_get_ipolling(edi->mem_pool,(void **) &ptr);
   if(rc == TERR_NO_ERR)
   {
      dd_ptr = (unsigned int*)*edi->pUDCA_pos;
      buf = *(dd_ptr+5);  //--- Get start buf addr
      s_memcpy(ptr,(unsigned char *)buf, USB_MAX_PACKET0);
      tn_queue_isend_polling(edi->queue,(void *)ptr);
   }
   rUSBEoTIntClr = (1 << edi->ep_num_phys); //-- Clear int bit
}

//----------------------------------------------------------------------------
void ep_rx_DD_request(EP_DMA_INFO * edi)
{
   int i;
   int nbuf;
   unsigned int * dd_ptr;
   int flag;

   TN_DQUE * dque = edi->queue;

   flag = ((dque->tail_cnt == 0 && dque->header_cnt == dque->num_entries - 1)
             || dque->header_cnt == dque->tail_cnt-1);
   if(flag)    //-- FIFO full - do not set DMA_ENABLE bit
   {
      rUSBNDDRIntClr = (1 << edi->ep_num_phys);  //-- Clear int bit
      return;
   }

   dd_ptr = (unsigned int *)edi->ep_dma_addr->pDD;
   nbuf   = edi->ep_dma_addr->nbuffers; //- Except no_packet

   for(i=0; i < nbuf; i++)
   {
      if(dd_ptr != (unsigned int *)*edi->pUDCA_pos)
      {
         if(*(dd_ptr+3) & 1) //-- DD_retired
         {
            *dd_ptr = 0;                //--- Clear word 0
                                        //--- Copy word 1 ???
            *(dd_ptr+3) = 0;            //--- Clear word 3
            *(dd_ptr+2) = *(dd_ptr+5);  //-- Restore DMA_buffer_start addr

            *edi->pUDCA_pos = (unsigned int)dd_ptr;
            break;
         }
      }
      dd_ptr += EP_DD_TOTAL_SIZE;  //-- 6
   }

   rUSBNDDRIntClr = (1 << edi->ep_num_phys);  //-- Clear int bit
   rUSBEpDMAEn    = (1 << edi->ep_num_phys);  //-- Set DMA_ENABLE bit
}

//----------------------------------------------------------------------------
void ep_tx_int_DD_request(EP_DMA_INFO * edi)
{
   int i;
   int rc;
   int nbuf;
   unsigned int * dd_ptr = NULL;
   unsigned int * dptr = NULL;
   unsigned char * ptr;
   unsigned char * q_ptr;

   TN_DQUE * que = edi->queue;
   TN_FMP * fmp  = edi->mem_pool;

   rc = tn_queue_ireceive(que,(void **)&q_ptr);
   if(rc == TERR_NO_ERR)
   {
      dd_ptr = (unsigned int *)edi->ep_dma_addr->pDD;
      nbuf   = edi->ep_dma_addr->nbuffers - 1; //- Except no_packet

      for(i=0; i < nbuf; i++)
      {
         if(*(dd_ptr+3) & 1) //-- DD_retired
         {
         //-- Free DD ----------------
            *dd_ptr = 0;               //--- Clear word 0
            *(dd_ptr+3) = 0;           //--- Clear word 3
            *(dd_ptr+4) = 0;           //--- Clear word 4
            *(dd_ptr+2) = (unsigned int)(dd_ptr+5); //-- Restore DMA_buffer_start addr
         //------------------------------
            dptr = dd_ptr;
            break;
         }
         dd_ptr += EP_DD_TOTAL_SIZE;  //-- 6 words
      }

      if(dptr != NULL)
      {
         dd_ptr = dptr;
         ptr = (unsigned char*) *(dd_ptr +2);
         s_memcpy(ptr,q_ptr,USB_MAX_PACKET0);
      }
      tn_fmem_irelease(fmp,(void*)q_ptr);
   }

   if(rc != TERR_NO_ERR ||  dptr == NULL)
   {
      dd_ptr = (unsigned int *)edi->ep_dma_addr->pDD;
      dd_ptr += (edi->ep_dma_addr->nbuffers-1) * EP_DD_TOTAL_SIZE; //--Last packet

      *dd_ptr = 0;
      *(dd_ptr+1) = 0; //-- no_packet
      *(dd_ptr+3) = 0;
      *(dd_ptr+2) = *(dd_ptr+5); //-- Restore DMA_buffer_start addr
   }

   *edi->pUDCA_pos = (unsigned int)dd_ptr;

   rUSBNDDRIntClr = (1 << edi->ep_num_phys);  //-- Clear int bit
   rUSBEpDMAEn    = (1 << edi->ep_num_phys); //-- Set DMA_ENABLE bit
}

//----------------------------------------------------------------------------
void tn_usb_dma_tx_init(EP_DMA_INFO * edi)
{
   int i;
   int nbuf;
   int buf_size;
   unsigned int * dd_ptr;
   unsigned int buf_ptr;

   dd_ptr = (unsigned int *)edi->ep_dma_addr->pDD;
   nbuf   = edi->ep_dma_addr->nbuffers;
   buf_size = ((edi->ep_dma_addr->word_1>>16) & 0xFFFF)>>2;  //-- in words
   buf_ptr =  edi->ep_dma_addr->p_data_buffers;

   for(i=0; i < nbuf; i++)
   {
      *dd_ptr = 0;  //--- Clear word 0
      *(dd_ptr+1) = edi->ep_dma_addr->word_1; //--- Copy word 1
      if(i == nbuf-1)   //-- last DD
      {
         *(dd_ptr+1) &= 0x0000001F; //-- no_packed DD
         *edi->pUDCA_pos = (unsigned int)dd_ptr;
         *(dd_ptr+3) = 0;
      }
      else
         *(dd_ptr+3) = 1;

      *(dd_ptr + 2) = buf_ptr;   //--- Fill Word 2
      *(dd_ptr + 4) = 0;         //--- Clear word 4 - not is0
      *(dd_ptr + 5) = buf_ptr;   //-- Fill recover buff address - word 5

      dd_ptr  += EP_DD_TOTAL_SIZE;
      buf_ptr += buf_size;
   }
}

//----------------------------------------------------------------------------
void tn_usb_dma_rx_init(EP_DMA_INFO * edi)
{
   int i;
   int nbuf;
   int buf_size;
   unsigned int * dd_ptr;
   unsigned int buf_ptr;

   dd_ptr = (unsigned int *)edi->ep_dma_addr->pDD;
   nbuf   = edi->ep_dma_addr->nbuffers;
   buf_size = ((edi->ep_dma_addr->word_1>>16) & 0xFFFF)>>2;  //-- in words
   buf_ptr =  edi->ep_dma_addr->p_data_buffers;

   for(i=0; i < nbuf; i++)
   {
      *dd_ptr = 0;  //--- Clear word 0
      *(dd_ptr+1) = edi->ep_dma_addr->word_1;  //--- Copy word 1
      if(i == 0)   //-- First DD
      {
         *edi->pUDCA_pos = (unsigned int)dd_ptr;
         *(dd_ptr+3) = 0;
      }
      else
         *(dd_ptr+3) = 1;

      *(dd_ptr + 2) = buf_ptr;  //--- Fill Word 2
      *(dd_ptr+4) = 0;          //--- Clear word 4 - not is0
      *(dd_ptr + 5) = buf_ptr;  //-- Fill recover buff address - word 5

      dd_ptr  += EP_DD_TOTAL_SIZE;
      buf_ptr += buf_size;
   }
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
