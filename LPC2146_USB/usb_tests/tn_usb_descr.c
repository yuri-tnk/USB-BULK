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


#define  NUM_ENDPOINTS   4


const BYTE abDescriptors[] =
{

//-- Device descriptor
   sizeof(USB_DEVICE_DESCRIPTOR),  //-- bLength
   USB_DESCR_TYPE_DEVICE,          //-- bDescriptorType
   TWOBYTES_LE(0x0110),            //-- bcdUSB    1.10
   0x00,                           //-- bDeviceClass
   0x00,                           //-- bDeviceSubClass
   0x00,                           //-- bDeviceProtocol
   USB_MAX_PACKET0,                //-- bMaxPacketSize
   TWOBYTES_LE(0x1237),            //-- idVendor - Dummy
   TWOBYTES_LE(0xABCD),            //-- idProduct - Dummy
   TWOBYTES_LE(0x0100),            //-- bcdDevice     1.00
   1,                              //-- iManufacturer  - index of string 1
   2,                              //-- iProduct       - index of string 2
   3,                              //-- iSerialNumber  - index of string 3
   1,                              //-- bNumConfigurations

//-- Configuration descriptor
   sizeof(USB_CONFIGURATION_DESCRIPTOR),  //-- bLength
   USB_DESCR_TYPE_CONFIGURATION,          //-- bDescriptorType
   TWOBYTES_LE(
      sizeof(USB_CONFIGURATION_DESCRIPTOR) +            //-- wTotalLength
      sizeof(USB_INTERFACE_DESCRIPTOR)     +            //-- 1st Interface
      NUM_ENDPOINTS * sizeof(USB_ENDPOINT_DESCRIPTOR)
  //  +
  //  sizeof(USB_INTERFACE_DESCRIPTOR)     +
  //  NUM_ENDPOINTS * sizeof(USB_ENDPOINT_DESCRIPTOR)
      ),
   1,                                     //-- bNumInterfaces
   1,                                     //-- bConfigurationValue
   0,                                     //-- iConfiguration
   USB_CONFIG_BUS_POWERED | USB_CONFIG_REMOTE_WAKEUP,  //-- bmAttributes
   100>>1,                                //-- bMaxPower (mA /2)

//-- Interface descriptor
   sizeof(USB_INTERFACE_DESCRIPTOR), //-- bLength
   USB_DESCR_TYPE_INTERFACE,         //-- bDescriptorType
   0x00,                             //-- bInterfaceNumber
   0x00,                             //-- bAlternateSetting
   NUM_ENDPOINTS,                    //-- bNumEndPoints
   0x00,                             //-- bInterfaceClass
   0x00,                             //-- bInterfaceSubClass
   0x00,                             //-- bInterfaceProtocol
   0x00,                             //-- iInterface

//-- EP2 OUT
   sizeof(USB_ENDPOINT_DESCRIPTOR),  //-- bLength
   USB_DESCR_TYPE_ENDPOINT,          //-- bDescriptorType
   2,                                //-- bEndpointAddress
   USB_ENDPOINT_TYPE_BULK,           //-- bmAttributes = bulk
   TWOBYTES_LE(64),                  //-- wMaxPacketSize
   0x00,                             //-- bInterval
//-- EP2 IN
   sizeof(USB_ENDPOINT_DESCRIPTOR),  //-- bLength
   USB_DESCR_TYPE_ENDPOINT,          //-- bDescriptorType
   2 | 0x80,                         //-- bEndpointAddress
   USB_ENDPOINT_TYPE_BULK,           //-- bmAttributes = bulk
   TWOBYTES_LE(64),                  //-- wMaxPacketSize
   0x00,                             //-- bInterval
//-- EP5 OUT
   sizeof(USB_ENDPOINT_DESCRIPTOR),  //-- bLength
   USB_DESCR_TYPE_ENDPOINT,          //-- bDescriptorType
   5,                                //-- bEndpointAddress
   USB_ENDPOINT_TYPE_BULK,           //-- bmAttributes = bulk
   TWOBYTES_LE(64),                  //-- wMaxPacketSize
   0x00,                             //-- bInterval
//-- EP5 IN
   sizeof(USB_ENDPOINT_DESCRIPTOR),  //-- bLength
   USB_DESCR_TYPE_ENDPOINT,          //-- bDescriptorType
   5 | 0x80,                         //-- bEndpointAddress
   USB_ENDPOINT_TYPE_BULK,           //-- bmAttributes = bulk
   TWOBYTES_LE(64),                  //-- wMaxPacketSize
   0x00,                             //-- bInterval

//-- String descriptors
   4,                                //-- bLength
   USB_DESCR_TYPE_STRING,            //-- bDescriptorType
   TWOBYTES_LE(0x0409),              //-- Lang - English

   24,                               //-- bLength
   USB_DESCR_TYPE_STRING,            //-- bDescriptorType
   'L', 0,
   'P', 0,
   'C', 0,
   '2', 0,
   '1', 0,
   '4', 0,
   'x', 0,
   ' ', 0,
   'U', 0,
   'S', 0,
   'B', 0,

   26,                               //-- bLength
   USB_DESCR_TYPE_STRING,            //-- bDescriptorType
   'T', 0,
   'N', 0,
   'K', 0,
   'e', 0,
   'r', 0,
   'n', 0,
   'e', 0,
   'l', 0,
   ' ', 0,
   'U', 0,
   'S', 0,
   'B', 0,

   14,                               //-- bLength
   USB_DESCR_TYPE_STRING,            //-- bDescriptorType
   '0', 0,
   '0', 0,
   '0', 0,
   '0', 0,
   '1', 0,
   '1', 0,

//-- Terminating zero
   0
};

