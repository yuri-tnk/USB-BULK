;------------------------------------------------------------------------------
;
;    TNKernel startup hardware init for LPC2101/03 & LPC214X processors
;    (for ARM(c) compiler)
;
;    ARM(c) assembler
;
; Copyright © 2004,2006 Yuri Tiomkin
; All rights reserved.
;
;Permission to use, copy, modify, and distribute this software in source
;and binary forms and its documentation for any purpose and without fee
;is hereby granted, provided that the above copyright notice appear
;in all copies and that both that copyright notice and this permission
;notice appear in supporting documentation.
;
;THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS ``AS IS'' AND
;ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
;IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
;ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
;FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
;DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
;OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
;HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
;LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
;OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
;SUCH DAMAGE.

;  ver 2.1


  ;-- Public functions declared in this file

     EXPORT tn_startup_hardware_init

  ;-- Extern reference

     IMPORT reset

   ;-- Constants


MC_FMR    EQU  0xFFFFFF60
WDT_MR    EQU  0xFFFFFD44
CKGR_MOR  EQU  0xFFFFFC20
PMC_SR    EQU  0xFFFFFC68
CKGR_PLLR EQU  0xFFFFFC2C
PMC_MCKR  EQU  0xFFFFFC30
MC_RCR    EQU  0xFFFFFF00

NOINT     EQU  0xc0

;----------------------------------------------------------------------------

           AREA    HardwareInit, CODE, READONLY

           CODE32

rMAMTIM   DCD  0xE01FC004
rMAMCR    DCD  0xE01FC000


;----------------------------------------------------------------------------
;  This routine is called immediately after reset to setup hardware that is
; vital for processor's functionality (for instance,SDRAM controller setup,
; PLL setup,etc.)
;  It is assumed that other hardware's init routine(s) will be invoked later
; by C-language function call.
;----------------------------------------------------------------------------

tn_startup_hardware_init

     ;--  For LPC2101/03, LPC2141/48


    ;-- Flash speed
    ;-- rMAMTIM = 3

      ldr   r0, rMAMTIM
      mov   r1, #3
      strb  r1, [r0]

    ;-- rMAMCR  = 2

      ldr   r0, rMAMCR
      mov   r1, #2
      strb  r1, [r0]

      bx   lr

;----------------------------------------------------------------------------
      EXPORT  tn_arm_disable_interrupts

tn_arm_disable_interrupts

     mrs  r0, cpsr
     orr  r0, r0, #NOINT
     msr  cpsr_c, r0
     bx   lr


     EXPORT  tn_arm_enable_interrupts


tn_arm_enable_interrupts

     mrs  r0, cpsr
     bic  r0, r0, #NOINT
     msr  cpsr_c, r0
     bx   lr


     END

;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------




