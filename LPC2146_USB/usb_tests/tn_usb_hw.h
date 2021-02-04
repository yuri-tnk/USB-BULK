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


#ifndef _TN_USB_HW_H_
#define _TN_USB_HW_H_


#ifndef BYTE
#define BYTE unsigned char
#endif
#ifndef WORD
#define WORD unsigned short
#endif
#ifndef BOOL
#define BOOL int
#endif

   //-- Philips LPC2148

#define  CCEMTY               (1<<4)         //-- rUSBDevIntClr
#define  CDFULL               (1<<5)         //-- rUSBDevIntClr
#define  RD_EN                    1          //-- rUSBCtrl
#define  WR_EN                (1<<1)         //-- rUSBCtrl
#define  PKT_RDY             (1<<11)         //-- rUSBRxPLen
#define  PKT_LNGTH_MASK        0x3FF         //-- rUSBRxPLen
#define  EP_RLZED             (1<<8)         //-- rUSBDevIntxx
#define  DEV_STAT             (1<<3)         //-- USBDevIntSt
#define  EP_SLOW              (1<<2)         //-- USBDevIntSt

//-- Protocol Engine Commands
   //-- Device
#define  CMD_DEV_STATUS              0xFE           //-- read/write
#define  CMD_DEV_CONFIG              0xD8
#define  CMD_DEV_SET_ADDRESS         0xD0
#define  CMD_DEV_SET_MODE            0xF3

   //-- Endpoint
#define  CMD_EP_SELECT               0x00
#define  CMD_EP_SELECT_CLEAR         0x40
#define  CMD_EP_SET_STATUS           0x40
#define  CMD_EP_CLEAR_BUFFER         0xF2
#define  CMD_EP_VALIDATE_BUFFER      0xFA


//-- CMD_EP_SET_STATUS
#define  EP_ST                      (1<<0)
#define  EP_DA                      (1<<5)
#define  EP_RF_MO                   (1<<6)
#define  EP_CND_ST                  (1<<7)

//-- CMD_DEV_CONFIG
#define CONF_DEVICE                 (1<<0)

//-- CMD_DEV_STATUS
#define  CON                            1
#define  CON_CH                     (1<<1)
#define  SUS                        (1<<2)
#define  SUS_CH                     (1<<3)
#define  RST                        (1<<4)

//-- CMD_EP_SELECT_CLEAR, CMD_EP_SELECT
#define  STP                        (1<<2)

//------ Prototypes ---

//-- tn_usb_hw.c

 int tn_usb_connect(int mode);
void tn_usb_config_EP0(void);
void tn_usb_int_suspend(void);
void tn_usb_int_resume(void);
void tn_usb_lpc_cmd(int cmd);
void tn_usb_lpc_cmd_write(int cmd, int data);
 int tn_usb_lpc_cmd_read(int cmd);
 int tn_usb_ep_read(int ep_num_logical,BYTE * buf);
 int tn_usb_ep0_write(int ep_num_logical,BYTE * buf,int nbytes);
 int tn_usb_ep_write(int ep_num_logical,BYTE * buf,int nbytes);
void tn_usb_stall_ep(int ep_num_logical,int mode);
void tn_usb_set_addr(int addr);
void tn_usb_reset_ep(int ep_num_logical);
void tn_usb_suspend(void);
void tn_usb_resume(void);
void tn_usb_wakeup_config(int mode);
void tn_usb_configure(int mode);
void tn_usb_configure_device(int mode);
void tn_usb_config_ep(BYTE * ptr);

//--- int func prototype - in tn_user.c
void tn_usb_int_func(void);


#endif
