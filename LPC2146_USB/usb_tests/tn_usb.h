
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

#ifndef  _TN_USB_H_
#define  _TN_USB_H_


#ifndef  NULL
#define  NULL   0
#endif
#ifndef  TRUE
#define  TRUE   1
#endif
#ifndef  FALSE
#define  FALSE  0
#endif

#define  ep_num_lg2ph(x)     ((((x) & 0x0F)<<1) | (((x)& 0x80)>>7))
#define  ep_num_lg2mask(x)   ((x) & 0x80) ? ((1 << 16) << (x)) : (1 << (x))
#define  TWOBYTES_LE(x)      ((x) & 0xFF),(((x) >> 8) & 0xFF)


//--Device Standard Request (EP0)
#define  USB_CMD_MASK_COMMON    0xE0   //-- Common request mask
#define  USB_CMD_STD_DEV_OUT    0x00   //-- Standard Device Request OUT
#define  USB_CMD_STD_DEV_IN     0x80   //-- Standard Device Request IN
#define  USB_CMD_CLASS_IN      ((1<<5) | 0x80)
#define  USB_CMD_CLASS_OUT     (1<<5)

//-- Standard Request Codes
#define  GET_STATUS             0x00
#define  CLEAR_FEATURE          0x01
#define  SET_FEATURE            0x03
#define  SET_ADDRESS            0x05
#define  GET_DESCRIPTOR         0x06
#define  SET_DESCRIPTOR         0x07
#define  GET_CONFIGURATION      0x08
#define  SET_CONFIGURATION      0x09
#define  GET_INTERFACE          0x0A
#define  SET_INTERFACE          0x0B
#define  SYNCH_FRAME            0x0C

//-- Define device states
#define  STATE_DEV_DEFAULT         0
#define  STATE_DEV_ADDRESS         1
#define  STATE_DEV_CONFIG          2

// bmRequestType Masks
#define  CMD_MASK_RECIP                 0x1F    //-- Request recipient bit mask
// bmRequestType Recipient Field
#define  CMD_RECIP_DEVICE               0x00    //-- Device
#define  CMD_RECIP_INTERFACE            0x01    //-- Interface
#define  CMD_RECIP_ENDPOINT             0x02    //-- Endpoint

#define  DEVICE_REMOTE_WAKEUP           0x01    //-- Remote Wakeup selector
#define  ENDPOINT_HALT                  0x00    //-- Endpoint Halt selector

//-- USB GET_STATUS Bit Values
#define  USB_GETSTATUS_SELF_POWERED     0x01
#define  USB_GETSTATUS_REMOTE_WAKEUP    0x02
#define  USB_GETSTATUS_ENDPOINT_STALL   0x01

//-- USB Standard Feature selectors
#define  USB_FEATURE_ENDPOINT_STALL        0
#define  USB_FEATURE_REMOTE_WAKEUP         1

//-- USB Descriptor Types
#define  USB_DESCR_TYPE_DEVICE                1
#define  USB_DESCR_TYPE_CONFIGURATION         2
#define  USB_DESCR_TYPE_STRING                3
#define  USB_DESCR_TYPE_INTERFACE             4
#define  USB_DESCR_TYPE_ENDPOINT              5
#define  USB_DESCR_TYPE_DEVICE_QUALIFIER      6
#define  USB_DESCR_TYPE_OTHER_SPEED_CONFIG    7
#define  USB_DESCR_TYPE_INTERFACE_POWER       8

#define  USB_CONFIG_POWERED_MASK                0xC0
#define  USB_CONFIG_BUS_POWERED                 0x80
#define  USB_CONFIG_SELF_POWERED                0x40
#define  USB_CONFIG_REMOTE_WAKEUP               0x20

//-- USB Device Classes
#define  USB_DEVICE_CLASS_RESERVED                 0
#define  USB_DEVICE_CLASS_AUDIO                    1
#define  USB_DEVICE_CLASS_COMMUNICATIONS           2
#define  USB_DEVICE_CLASS_HUMAN_INTERFACE          3
#define  USB_DEVICE_CLASS_MONITOR                  4
#define  USB_DEVICE_CLASS_PHYSICAL_INTERFACE       5
#define  USB_DEVICE_CLASS_POWER                    6
#define  USB_DEVICE_CLASS_PRINTER                  7
#define  USB_DEVICE_CLASS_STORAGE                  8
#define  USB_DEVICE_CLASS_HUB                      9
#define  USB_DEVICE_CLASS_VENDOR_SPECIFIC       0xFF

//-- bmAttributes in Endpoint Descriptor
#define USB_ENDPOINT_TYPE_CONTROL          0
#define USB_ENDPOINT_TYPE_ISOCHRONOUS      1
#define USB_ENDPOINT_TYPE_BULK             2
#define USB_ENDPOINT_TYPE_INTERRUPT        3

//-- Offset in descriptors structures(as byte array)
#define  USB_IND_bConfigurationValue       5
#define  USB_IND_bNumInterfaces            4
#define  USB_IND_CFG_bmAttributes          7
#define  USB_IND_wTotalLength              2
#define  USB_IND_bAlternateSetting         3
#define  USB_IND_bEndpointAddress          2
#define  USB_IND_bInterfaceNumber          2
#define  USB_IND_wMaxPacketSize            4    //-- in Std EP Descriptor only


#define  USB_DESC_CONFIGURATION            2


#if defined (__GNUC__)
#define __pk__  __attribute__ ((__packed__))
#else
#define __pk__
#endif

#if defined (__CC_ARM)      //-- ARM ADS & RVCT
#define __pkd__  __packed
#else
#define __pkd__
#endif


#ifndef __CC_ARM
#pragma pack(1)
#endif
//-- Control endpoint command (from host)
__pkd__ struct _USB_SETUP_PACKET
{
   BYTE bmRequestType;  //-- Request type
   BYTE bRequest;       //-- Specific request
   WORD wValue;         //-- Misc field
   WORD wIndex;         //-- Misc index
   WORD wLength;        //-- Length of the data segment for this request
}__pk__;
typedef struct _USB_SETUP_PACKET USB_SETUP_PACKET;

//-- USB Standard Device Descriptor
__pkd__ struct _USB_DEVICE_DESCRIPTOR
{
   BYTE  bLength;
   BYTE  bDescriptorType;
   WORD  bcdUSB;
   BYTE  bDeviceClass;
   BYTE  bDeviceSubClass;
   BYTE  bDeviceProtocol;
   BYTE  bMaxPacketSize0;
   WORD  idVendor;
   WORD  idProduct;
   WORD  bcdDevice;
   BYTE  iManufacturer;
   BYTE  iProduct;
   BYTE  iSerialNumber;
   BYTE  bNumConfigurations;
}__pk__;
typedef struct _USB_DEVICE_DESCRIPTOR USB_DEVICE_DESCRIPTOR;

//-- USB Standard Configuration Descriptor
__pkd__ struct _USB_CONFIGURATION_DESCRIPTOR
{
   BYTE  bLength;
   BYTE  bDescriptorType;
   WORD  wTotalLength;
   BYTE  bNumInterfaces;
   BYTE  bConfigurationValue;
   BYTE  iConfiguration;
   BYTE  bmAttributes;
   BYTE  MaxPower;
}__pk__;
typedef struct _USB_CONFIGURATION_DESCRIPTOR USB_CONFIGURATION_DESCRIPTOR;

//-- USB Standard Interface Descriptor
__pkd__ struct _USB_INTERFACE_DESCRIPTOR
{
   BYTE  bLength;
   BYTE  bDescriptorType;
   BYTE  bInterfaceNumber;
   BYTE  bAlternateSetting;
   BYTE  bNumEndpoints;
   BYTE  bInterfaceClass;
   BYTE  bInterfaceSubClass;
   BYTE  bInterfaceProtocol;
   BYTE  iInterface;
}__pk__;
typedef struct _USB_INTERFACE_DESCRIPTOR USB_INTERFACE_DESCRIPTOR;

//-- USB Standard Endpoint Descriptor
__pkd__ struct _USB_ENDPOINT_DESCRIPTOR
{
   BYTE  bLength;
   BYTE  bDescriptorType;
   BYTE  bEndpointAddress;
   BYTE  bmAttributes;
   WORD  wMaxPacketSize;
   BYTE  bInterval;
}__pk__;
typedef struct _USB_ENDPOINT_DESCRIPTOR USB_ENDPOINT_DESCRIPTOR;

#ifndef __CC_ARM
#pragma pack()
#endif


//----- Configuration file
#include "tn_usb_conf.h"


//-- Endpoint status (used for IN, OUT, and Endpoint0)
typedef struct _USB_EP_STATUS
{
   int  nbytes;           //-- Num bytes to transmit
   unsigned int * pdata;  //-- data to transmit
   void * pbuf;           //-- Endpoint buffer start
}USB_EP_STATUS;


typedef struct _USB_DEVICE_INFO
{
   USB_EP_STATUS     EP0Status;
   USB_SETUP_PACKET  EP0SetupPacket;
   BYTE * Descriptors;  //-- All descriptors(device,config,interface,
                        //-- endpoints,strings) are joined in single array (BYTE [])
   int  Configuration;

}USB_DEVICE_INFO;

//--- tn_usb_req.c

void tn_usb_reset_data(USB_DEVICE_INFO * udi);
void tn_usb_st_DATAIN(USB_EP_STATUS * eps);
BOOL tn_usb_class_request(USB_DEVICE_INFO * udi);
BOOL tn_usb_EP0_SETUP(USB_DEVICE_INFO * udi);

//--- tn_usb_ep.c

void tn_usb_EP0_tx_int_func(USB_DEVICE_INFO * udi); //-- IN
void tn_usb_EP0_rx_int_func(USB_DEVICE_INFO * udi); //-- OUT
void tn_usb_EP2_tx_int_func(USB_DEVICE_INFO * udi);
void tn_usb_EP2_rx_int_func(USB_DEVICE_INFO * udi);
void tn_usb_EP5_tx_int_func(USB_DEVICE_INFO * udi);
void tn_usb_EP5_rx_int_func(USB_DEVICE_INFO * udi);

//--- tn_usb_hw.c

void tn_usb_int_reset(USB_DEVICE_INFO *udi);
void tn_usb_int_con_ch(unsigned int dev_status,USB_DEVICE_INFO *udi);



#endif


