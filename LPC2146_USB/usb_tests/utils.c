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

#include "lpc214x.h"
#include "utils.h"
#include "tn_usb_hw.h"
#include "tn_usb.h"

//----------------------------------------------------------------------------
//    VALID ONLY FOR 'LPC2146 USB TEST' PROJECT WITHOUT DMA
//----------------------------------------------------------------------------

/*-------- Project specific pinout description ------

                                        I/O dir set            I/O dir
                                       -------------          ---------

 P0.0  - TxD0          U-TXD-2             auto                  out
 P0.1  - RxD0          U-RXD0-2            auto                  in
 P0.2 -  GPIO          Not used            manual                Out
 P0.3  - GPIO          Not used            manual                out
 P0.4  - GPIO          Not used            manual                out
 P0.5  - GPIO          Not used            manual                out
 P0.6  - GPIO          Not used            manual                out
 P0.7  - GPIO          Not used            manual                out
 P0.8  - GPIO          Not used            manual                out
 P0.9  - GPIO          Not used            manual                out
 P0.10 - GPIO          Not used            manual                out
 P0.11 - GPIO          Not used            manual                out
 P0.12 - GPIO          Not used            manual                out
 P0.13 - GPIO          Not used            manual                out
 P0.14 - sys input     LED                 manual                out
 P0.15 - GPIO          Not used            manual                out
 P0.16 - GPIO          Not used            manual                out
 P0.17 - GPIO          Not used            manual                out
 P0.18 - GPIO          Not used            manual                out
 P0.19 - GPIO          Not used            manual                out
 P0.20 - GPIO          Not used            manual                out
 P0.21 - GPIO          Not used            manual                out
 P0.22 - GPIO          Not used            manual                out
 P0.23 - VBUS          VBUS                auto                  in
 P0.24 - not exists    ???
 P0.25 - GPIO          Not used            manual                out
 P0.26 - GPIO          Not used            manual                out
 P0.27 - GPIO          AUX2                manual                out
 P0.28 - GPIO          AUX4                manual                out
 P0.29 - GPIO          AUX5                manual                out
 P0.30 - GPIO          Not used            manual                out
 P0.31 - CONNECT       CONNECT             auto                  out

 P1.16 - GPIO          Not used            manual                out
 P1.17 - GPIO          AUX3                manual                out
 P1.18 - GPIO          Not used            manual                out
 P1.19 - GPIO          Not used            manual                out
 P1.20 - GPIO          Not used            auto                  in
 P1.21 - GPIO          Not used            manual                out
 P1.22 - GPIO          Not used            manual                out
 P1.23 - GPIO          Not used            manual                out
 P1.24 - GPIO          Not used            manual                out
 P1.25 - GPIO          Not used            manual                out
 P1.26 - GPIO          Not used            auto                  in
 P1.27 - TDO
 P1.28 - TDI
 P1.29 - TCK
 P1.30 - TMS
 P1.31 - TRST
----------------------------------------------------------------------------*/

static void InitUART0(void);
static void InitUSB(void);  //-- external


//--- Checking .data segment only - you may omit it
volatile int chk_data_seg = 0x12345678;

//----------------------------------------------------------------------------
void  HardwareInit(void)
{
   int i;

 //-- Crystal 12 MHz

 //---  Peripheral - on max speed  bits 1..0 = 01 -> prc;
   rVPBDIV = 0x00000001;

 //--- PLL(Fout = 60MHz) - multiplay at 5 (F = Fosc * MSEL * 2 * P / 2*P)

   rPLL0CFG = (1<<5) | 0x4;  //-- MSEL = 5 [4:0] = 00100;   P=2 ([6:5] = 01)
   rPLL0CON = 0x03;          //-- PLLE = 1; PLLC = 1

   for(i=0;i<100;i++);
   rPLL0FEED = 0xAA;
   rPLL0FEED = 0x55;
   for(i=0;i<100;i++);

  //--- Disable all int ------

   VICIntEnable = 0;

   VICDefVectAddr = (unsigned int)tn_int_default_func; //-- default int processing

 //--- Timer 0  - interrupt 10 ms

   rT0PR = 0;  //-- Prscaler = 0
   rT0PC = 0;

   rT0MR0 = 60000 * 10;
   rT0MCR = 3; //-- bit 0=1 -int on MR0 , bit 1=1 - Reset on MR0

   rT0TCR = 1; //-- Timer 0 - run

   //--  int for Timer 0
   VICVectAddr1 = (unsigned int)tn_timer0_int_func;   //-- Timer 0 int - priority top-1
   VICVectCntl1 = 0x20 | 4;
   VICIntEnable = (1<<4);

 //---- UART 0 ----

   InitUART0();

   //--  int for UART0
  // VICVectAddr2 = (unsigned int)tn_uart0_int_func;   //-- UART0 int - priority top-2
 //  VICVectCntl2 = 0x20 | 6;
 //  VICIntEnable = (1 << 6);

 //---- USB ----

   InitUSB();

   //-- int for USB
   VICVectAddr0 = (unsigned int)tn_usb_int_func;   //-- USB int - top priority
   VICVectCntl0 = 0x20 | 22;
   VICIntEnable = (1 << 22);

 //--- Pins - to output & set to initial values ------------------------------

//   rIO0DIR |= 1<<2;  //-- output
//   rIO0SET |= 1<<2;  //-- Set P0.2 to 1

//-- P0.2 - P0.22  - GPIO/out

#define MASK_PINS_P0_2_P0_22   0x007FFFFC

   rIO0DIR |= MASK_PINS_P0_2_P0_22;  //-- output
   rIO0SET |= MASK_PINS_P0_2_P0_22;  //-- Set  to 1

//-- P0.23 -  VBUS                auto                in
//-- P0.25 - P0.30 - GPIO/out

#define MASK_PINS_P0_25_P0_30  0x7E000000

   rIO0DIR |= MASK_PINS_P0_25_P0_30;  //-- output
   rIO0SET |= MASK_PINS_P0_25_P0_30;  //-- Set  to 1

//-- P1.16 - P1.19 - GPIO/out

#define MASK_PINS_P1_16_P1_19  0x000F0000

   rIO0DIR |= MASK_PINS_P1_16_P1_19;  //-- output
   rIO0SET |= MASK_PINS_P1_16_P1_19;  //-- Set  to 1

//-- P1.21 - P1.25 - GPIO/out

#define MASK_PINS_P1_21_P1_25  0x03E00000

   rIO0DIR |= MASK_PINS_P1_21_P1_25;  //-- output
   rIO0SET |= MASK_PINS_P1_21_P1_25;  //-- Set  to 1

//-----------------------------

}

//----------------------------------------------------------------------------
// UART0 init - FIFO enabled,IRQ enabled(later)
//----------------------------------------------------------------------------
void InitUART0(void)
{
   //---- pinout -----
   rPINSEL0 |= (1<<2) | 1; //-- Pins P0.0 & P0.1 - for UART0
    //-- enable access to divisor latch regs
   rU0LCR = LCR_ENABLE_LATCH_ACCESS;
    //-- set divisor for desired baud
   rU0DLM = 0;
   rU0DLL = 32; // 32;    // (14.746*4)/16*115200  = 32
    //-- disable access to divisor latch regs (enable access to xmit/rcv fifos
    //-- and int enable regs)
   rU0LCR = LCR_DISABLE_LATCH_ACCESS;

  //-- Enable UART0 rx interrupts
   rU0IER = 3;  //-- Enable RDA(Receive Data Available) int

   rU0FCR = (0x3<<6) | 1;  //-- Int Trigger - 0xE bytes, Enable FIFO,Reset Tx FIFO & Rx FIFO
    //-- setup line control reg - disable break transmittion, even parity,
    //-- 1 stop bit, 8 bit chars
   rU0LCR = 0x13;//-- 0b00010011
}

//----------------------------------------------------------------------------
static void InitUSB(void)  //-- Hardware init
{
   int i;

  //-- VBUS & SoftConnect pins
   rPINSEL1 |= 1<<14;      //-- P0.23 -> VBUS
   rPINSEL1 |= 2u << 30;   //-- P0.31 -> CONNECT

  //--- USB PLL -PLL1(Fout = 48MHz) - multiplay at 4 (F = Fosc * MSEL * 2 * P / 2*P)

   rPLL1CFG = 0x23;  //-- b00100011 bits 4..0 - MSEL4:0 = 4(3+1); bits 6:5 - PSEL 1:0 = 2(1+1);
   rPLL1CON = 0x03;  //-- PLLE = 1; PLLC = 1

   for(i=0;i<100;i++);
   rPLL1FEED = 0xAA;
   rPLL1FEED = 0x55;
   while(!(rPLL1STAT & (1<<10))); // Wait for PLL Lock

  //-- Enable USB hardware

   rPCONP |= 1u << 31;

  //-- Cmd- disconnect

   tn_usb_lpc_cmd_write(CMD_DEV_STATUS,0);

     //-- Endpoints  int
   rUSBEpIntClr = 0xFFFFFFFF;

   tn_usb_config_EP0();

     //-- USB device int
   rUSBDevIntClr = 0xFFFFFFFF;
   rUSBDevIntEn |= (1<<3) | (1<<2); //-- DEV_STAT,EP_SLOW

   tn_usb_set_addr(0);
}

//----------------------------------------------------------------------------
char * s_itoa(char * buffer, int i)
{
   char * ptr;

   ptr = buffer + 12;
   *ptr = 0;
   do
   {
      ptr--;
      *ptr = (i % 10) + '0';
      i /= 10;
   }while( i > 0);

   return ptr;
}

//----------------------------------------------------------------------------
void * s_memset(void * dst,  int ch, int length)
{
  register char *ptr = (char *)dst - 1;

  while(length--)
     *++ptr = ch;

  return dst;
}

//----------------------------------------------------------------------------
void * s_memcpy(void * s1, const void *s2, int n)
{
   register int mask = (int)s1 | (int)s2 | n;

   if(n == 0)
      return s1;

   if(mask & 0x1)
   {
      register const char *src = s2;
      register char *dst = s1;

      do
      {
         *dst++ = *src++;
      }while(--n);

      return s1;
   }

   if(mask & 0x2)
   {
      register const short *src = s2;
      register       short *dst = s1;

      do
      {
        *dst++ = *src++;
      }while( n -= 2);

      return s1;
   }
   else
   {
      register const int *src = s2;
      register       int *dst = s1;

      do
      {
         *dst++ = *src++;
      }while (n -= 4);

      return s1;
   }

   return s1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------





























