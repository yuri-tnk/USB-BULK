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

//----------------------------------------------------------------------------
//    VALID ONLY FOR 'LPC2146 USB TEST' PROJECT  WITH DMA
//----------------------------------------------------------------------------


#ifndef  _UTILS_H_
#define  _UTILS_H_


#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#define __min(a,b)  (((a) < (b)) ? (a) : (b))


#define LCR_DISABLE_LATCH_ACCESS    0x00000000
#define LCR_ENABLE_LATCH_ACCESS     0x00000080


//---- Prototypes -----

void  HardwareInit(void);

char * s_itoa(char * buffer, int i);
void * s_memset(void * dst,  int ch, int length);
void * s_memcpy(void * s1, const void *s2, int n);

//-- Interrupt functions prototypes ---

void tn_int_default_func(void);
void tn_timer0_int_func(void);
void tn_uart0_int_func(void);

//--- TNKernel core functions

void tn_arm_enable_interrupts(void);
void tn_arm_disable_interrupts(void);

#endif


