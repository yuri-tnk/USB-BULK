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

#include "tn_usb_hw.h"
#include "tn_usb.h"
#include "LPC214x.h"


//-----------------------------------------------------------------------------
void tn_usb_reset_data(USB_DEVICE_INFO * udi)
{
   USB_EP_STATUS * eps = &udi->EP0Status;

   udi->Configuration = 0;

   eps->nbytes = 0;        //-- Num bytes to transmit
}

//----------------------------------------------------------------------------
void tn_usb_st_DATAIN(USB_EP_STATUS * eps)
{
   int bytes;

   if(eps->nbytes > USB_MAX_PACKET0)
      bytes = USB_MAX_PACKET0;
   else
      bytes = eps->nbytes;

   bytes = tn_usb_ep0_write(0x80,(BYTE*)eps->pdata, bytes); //-- Tx EP0
   eps->nbytes -= bytes;
   eps->pdata  += bytes;
}

//----------------------------------------------------------------------------
BOOL static usb_SET_ADDRESS(USB_SETUP_PACKET  * usp)
{
   int rc = FALSE;
   if((usp->bmRequestType & CMD_MASK_RECIP) == CMD_RECIP_DEVICE)
   {
      tn_usb_set_addr(usp->wValue);
      rc = TRUE;
   }
   return rc;
}

//----------------------------------------------------------------------------
BOOL static usb_SET_FEATURE(USB_DEVICE_INFO * udi)
{
   int rc = FALSE;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;

   switch(usp->bmRequestType & CMD_MASK_RECIP)
   {
      case CMD_RECIP_DEVICE:
         break;
      case CMD_RECIP_ENDPOINT:
         break;
   }
   return rc;
}

//----------------------------------------------------------------------------
BOOL static usb_CLEAR_FEATURE(USB_SETUP_PACKET  * usp)
{
   int rc = FALSE;

   switch(usp->bmRequestType & CMD_MASK_RECIP)
   {
      case CMD_RECIP_DEVICE:
         break;

      case CMD_RECIP_ENDPOINT:
         if(usp->wValue == 0) //-- Feature should be 0 for endpoints
         {
            tn_usb_stall_ep(usp->wIndex,FALSE);
            rc = TRUE;
         }
         break;
   }
   return rc;
}

//----------------------------------------------------------------------------
BOOL static usb_SET_CONFIGURATION(USB_DEVICE_INFO * udi)
{
   unsigned char * ptr;

   int rc = FALSE;
   int aset = 0;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;

   if(usp->wValue & 0xFF) //-- Configuration Value
   {
      ptr = (BYTE*)udi->Descriptors; //-- Start position
      while(*ptr) //bLength        //-- Scan through all (0- terminator)
      {
         switch (*(ptr+1)) //-- bDescriptorType
         {
            case USB_DESCR_TYPE_CONFIGURATION:

               if(*(ptr + USB_IND_bConfigurationValue) == (usp->wValue & 0xFF))  //-- Matched
               {
                  udi->Configuration = usp->wValue & 0xFF;
                  tn_usb_configure_device(TRUE);
                  rc = TRUE;
               }
               else
               {
                  ptr += *((WORD*)(ptr + USB_IND_wTotalLength)); //-- Little endian
                  continue;
               }
               break;

            case USB_DESCR_TYPE_INTERFACE:
               aset = *(ptr + USB_IND_bAlternateSetting);
               break;
            case USB_DESCR_TYPE_ENDPOINT:
               if(aset == 0) //-- if no AlternateSetting
                  tn_usb_config_ep(ptr);
               break;
         }
         ptr += *ptr; //-- bLength;
      }
   }
   else  //-- Switch to ADDRESS State
   {
      udi->Configuration = 0;
      tn_usb_configure_device(FALSE);
   }

   if(udi->Configuration == (usp->wValue & 0xFF)) //  device was configured
      rc = TRUE;

   return rc;
}
//----------------------------------------------------------------------------
BOOL static usb_SET_INTERFACE(USB_DEVICE_INFO * udi)
{
   int rc = FALSE;
   return rc;
}

//----------------------------------------------------------------------------
void static usb_exch_stage_STALLOUT(USB_EP_STATUS * eps)
{
   tn_usb_stall_ep(0x00,TRUE);  //-- Rx EP0
   eps->nbytes = 0;
}

//----------------------------------------------------------------------------
BOOL static usb_GET_STATUS(USB_DEVICE_INFO * udi)
{
   int rc = FALSE;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;
   USB_EP_STATUS * eps = &udi->EP0Status;

   switch(usp->bmRequestType & CMD_MASK_RECIP)
   {
      case CMD_RECIP_DEVICE:

         eps->pdata[0] = 0;   //-- bit [0] self-powered , bit [1] remote wakeup
         eps->pdata[1] = 0;
         eps->nbytes   = 2;
         rc =TRUE;
         break;

      case CMD_RECIP_INTERFACE:

         eps->pdata[0] = 0;   //-- no bits specified
         eps->pdata[1] = 0;
         eps->nbytes   = 2;
         rc =TRUE;
         break;

      case CMD_RECIP_ENDPOINT:

         eps->pdata[0] = 0;   //-- bit 0 = endpointed halted or not
         eps->pdata[1] = 0;
         eps->nbytes   = 2;
         rc = TRUE;
         break;
   }
   return rc;
}

//----------------------------------------------------------------------------
BOOL static usb_GET_DESCRIPTOR(USB_DEVICE_INFO * udi)
{
   BYTE * ptr;
   int ind;
   int req_type;
   int req_index;

   int rc = FALSE;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;
   USB_EP_STATUS * eps = &udi->EP0Status;

   switch(usp->bmRequestType & CMD_MASK_RECIP)
   {
      case CMD_RECIP_DEVICE:

         req_type  = (usp->wValue)>>8;
         req_index = (usp->wValue) & 0xFF;

         ptr = (BYTE *)&udi->Descriptors[0];
         ind = 0;

         while(*ptr != 0)              //-- [0] -  bLength
         {
            if(*(ptr+1) == req_type)   //-- [1] - bDescriptorType
            {
               if(ind == req_index)
               {
                  eps->pdata = (unsigned int *)ptr;
                  if(req_type == USB_DESC_CONFIGURATION)
                     eps->nbytes  =  *((WORD*)(ptr+USB_IND_wTotalLength)); //-- Little endian
                  else
                     eps->nbytes = *ptr;

                  rc = TRUE;       //return TRUE;
                  break;
               }
               ind++;
            }
            ptr += *ptr;           //-- +=  bLength -> to next descriptor
         }
         break;

      case CMD_RECIP_INTERFACE:
      case CMD_RECIP_ENDPOINT:
         break;
   }
   return rc;
}

//----------------------------------------------------------------------------
BOOL static usb_GET_CONFIGURATION(USB_DEVICE_INFO * udi)
{
   int rc = FALSE;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;
   USB_EP_STATUS * eps = &udi->EP0Status;

   if((usp->bmRequestType & CMD_MASK_RECIP) == CMD_RECIP_DEVICE)
   {
      eps->pdata[0] =  udi->Configuration;
      eps->nbytes   = 1;
      rc = TRUE;
   }

   return rc;
}

//----------------------------------------------------------------------------
BOOL static usb_GET_INTERFACE(USB_SETUP_PACKET  * usp)
{
   int rc = FALSE;

   switch(usp->bmRequestType & CMD_MASK_RECIP)
   {
      case CMD_RECIP_INTERFACE:
         break;

      case CMD_RECIP_DEVICE:
      case CMD_RECIP_ENDPOINT:
         break;
   }

   return rc;
}

//----------------------------------------------------------------------------
BOOL tn_usb_class_request(USB_DEVICE_INFO * udi)
{
   int rc = FALSE;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;
   USB_EP_STATUS * eps = &udi->EP0Status;

   switch(usp->bRequest)
   {
      case 0xFE:
         eps->pdata[0] = 0;                // No LUNs
         eps->nbytes = 1;
         rc = TRUE;
         break;
      case 0xFF:
         rc = TRUE;
         break;
   }

   return rc;
}

//----------------------------------------------------------------------------
BOOL tn_usb_EP0_SETUP(USB_DEVICE_INFO * udi)
{
   int rc = FALSE;
   USB_SETUP_PACKET  * usp = &udi->EP0SetupPacket;
   USB_EP_STATUS * eps = &udi->EP0Status;

   switch (usp->bmRequestType & USB_CMD_MASK_COMMON)
   {
      case USB_CMD_STD_DEV_OUT:       //-- STANDARD OUT device requests

         switch(usp->bRequest)
         {
            case SET_ADDRESS:
               rc = usb_SET_ADDRESS(usp);
               break;
            case SET_FEATURE:
               rc = usb_SET_FEATURE(udi);
               break;
            case CLEAR_FEATURE:
               rc = usb_CLEAR_FEATURE(usp);
               break;
            case SET_CONFIGURATION:             //-- if ok-STATUSIN else-stall
               rc = usb_SET_CONFIGURATION(udi);
               break;
            case SET_INTERFACE:                 //-- if ok-STATUSIN else-stall
               rc = usb_SET_INTERFACE(udi);
               break;
            case SET_DESCRIPTOR:
               usb_exch_stage_STALLOUT(eps); //-- Not supported -> stall out
            default:
            //-- All other OUT requests not supported (default rc = EP0_ERR)
               break;
         }
         break;

      case USB_CMD_STD_DEV_IN:        //-- STANDARD IN request

         switch(usp->bRequest)
         {
            case GET_STATUS:               //-- if ok- DATAIN; else - stall
               rc = usb_GET_STATUS(udi);
               break;
            case GET_DESCRIPTOR:           //-- if ok- DATAIN; else - stall
               rc = usb_GET_DESCRIPTOR(udi);
               break;
            case GET_CONFIGURATION:       //-- if ok- DATAIN; else - stall
               rc = usb_GET_CONFIGURATION(udi);
               break;
            case GET_INTERFACE:          //-- if ok- DATAIN; else - stall
               rc = usb_GET_INTERFACE(usp);
               break;
            //case SYNCH_FRAME:
            default:
            //-- All other IN requests not supported (default rc = EP0_ERR)
               break;
         }
         break;

      case  USB_CMD_CLASS_IN:       //-- CLASS request
      case  USB_CMD_CLASS_OUT:

         rc = tn_usb_class_request(udi);
         break;

      default:
     //-- All other requests not supported here
         break;

   }
   return rc;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


