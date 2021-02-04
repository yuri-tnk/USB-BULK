;------------------------------------------------------------------------------
;
;    TNKernel startup code init for ARM processors
;    (for ARM(c) compiler)
;
;
; Copyright © 2004,2005 Yuri Tiomkin
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
;------------------------------------------------------------------------------

;--  ver 2.0

;-----    ARM(c) assembler ----


        PRESERVE8

MODE_BITS   EQU     0x1F               ;-- Bit mask for mode bits in CPSR
USR_MODE    EQU     0x10               ;-- User mode
FIQ_MODE    EQU     0x11               ;-- Fast Interrupt Request mode
IRQ_MODE    EQU     0x12               ;-- Interrupt Request mode
SVC_MODE    EQU     0x13               ;-- Supervisor mode
ABT_MODE    EQU     0x17               ;-- Abort mode
UND_MODE    EQU     0x1B               ;-- Undefined Instruction mode
SYS_MODE    EQU     0x1F               ;-- System mode

;----- This is a place to set stacks/heap sizes ---------------------------------

        ;-- Amount of memory (in bytes) allocated for stacks

stack_und_len      EQU       0
stack_abt_len      EQU       0
stack_irq_len      EQU     256
stack_fiq_len      EQU     128
stack_app_len      EQU    1024        ;-- app stack in TNKernel - CVS stack


stack_und_offset   EQU      stack_und_len
stack_abt_offset   EQU      ((stack_und_offset) + stack_abt_len)
stack_irq_offset   EQU      ((stack_abt_offset) + stack_irq_len)
stack_fiq_offset   EQU      ((stack_irq_offset) + stack_fiq_len)
stack_app_offset   EQU      ((stack_fiq_offset) + stack_app_len)

HEAP_SIZE          EQU    1024

;-----------------------------------------------------------------------------

        IMPORT  tn_cpu_irq_isr
        IMPORT  tn_cpu_fiq_isr
        IMPORT  tn_startup_hardware_init

        GBLS    MainEntry
MainEntry       SETS         "main"
        IMPORT  $MainEntry

        AREA Vect, CODE, READONLY

        ENTRY

        EXPORT  reset

reset
        ldr  pc, reset_handler_address
        ldr  pc, undef_handler_address
        ldr  pc, swi_handler_address
        ldr  pc, pabort_handler_address
        ldr  pc, dabort_handler_address
        DCD  0xB8A06F58                  ; 0 - (sum of other vectors instructions)
        ldr  pc, irq_handler_address
        ldr  pc, fiq_handler_address

reset_handler_address   DCD  reset_handler
undef_handler_address   DCD  undef_handler
swi_handler_address     DCD  swi_handler
pabort_handler_address  DCD  pabort_handler
dabort_handler_address  DCD  dabort_handler
                        DCD   0x00
irq_handler_address     DCD  tn_cpu_irq_isr
fiq_handler_address     DCD  tn_cpu_fiq_isr


reset_handler
        b  _start
undef_handler
        b  undef_handler
swi_handler
        b  swi_handler
pabort_handler
        b  pabort_handler
dabort_handler
        b  dabort_handler

;---- Start

_start
        ldr   r0,=tn_startup_hardware_init      ;-- vital hardware init
        mov   lr,pc
        bx    r0

     ;--- init stacks

        ldr   r1, =|Image$$ER_RW$$Limit|
        ldr   r0, =|Image$$ER_ZI$$ZI$$Length|
        add   r1,r1,r0
        mov   r4,r1                             ;-- Store end of BSS
        add   r1,r1,#HEAP_SIZE

        mrs   r0, cpsr
        bic   r0, r0, #MODE_BITS                ;-- Clear the mode bits
        orr   r0, r0, #UND_MODE                 ;-- Undefined mode
        msr   cpsr_c, r0                        ;-- Change the mode
        mov   r0, r1                            ;-- heap_limit
        add   r0, r0, #stack_und_offset
        mov   sp, r0                            ;-- End of UND_STACK

        mrs   r0, cpsr
        bic   r0, r0, #MODE_BITS                ;-- Clear the mode bits
        orr   r0, r0, #ABT_MODE                 ;-- Abort mode
        msr   cpsr_c, r0                        ;-- Change the mode
        mov   r0, r1                            ;-- heap_limit
        add   r0, r0, #stack_abt_offset
        mov   sp, r0                            ;-- End of ABT_STACK

        mrs   r0, cpsr
        bic   r0, r0, #MODE_BITS                ;-- Clear the mode bits
        orr   r0, r0, #IRQ_MODE                 ;-- Set IRQ mode bits
        msr   cpsr_c, r0                        ;-- Change the mode
        mov   r0, r1                            ;-- heap_limit
        add   r0, r0, #stack_irq_offset
        mov   sp, r0                            ;-- End of IRQ_STACK

        mrs   r0, cpsr
        bic   r0, r0, #MODE_BITS                ;-- Clear the mode bits
        orr   r0, r0, #FIQ_MODE                 ;-- Set FIQ mode bits
        msr   cpsr_c, r0                        ;-- Change the mode
        mov   r0, r1                            ;-- heap_limit
        add   r0, r0, #stack_fiq_offset
        mov   sp, r0                            ;-- End of FIQ_STACK

        ;-- Leave core in SVC mode

        mrs   r0, cpsr
        bic   r0, r0, #MODE_BITS                ;-- Clear the mode bits
        orr   r0, r0, #SVC_MODE                 ;-- Set Supervisor mode bits
        msr   cpsr_c, r0                        ;-- Change the mode
        mov   r0, r1                            ;-- heap_limit
        add   r0, r0, #stack_app_offset
        mov   sp, r0                            ;-- End of stack

   ;--- Semihosting not uses !!!

        ;-- The initial values for any
        ;-- initialized variables (RW segment) must be copied from ROM to RAM

InitData
        ldr         r0, TopOfROM          ;-- Get pointer to ROM data
        ldr         r1, BaseOfBSS         ;-- and RAM copy
        ldr         r3, BaseOfZero        ;-- Zero init base => top of
                                              ;--    initialised data
        cmp         r0, r1
        beq         NoInitCopy
InitCopyLoop
        cmp         r1, r3
        ldrccb      r2, [r0], #1
        strccb      r2, [r1], #1
        bcc         InitCopyLoop
NoInitCopy


  ;--- Zero the bss.

        ldr r0, BaseOfZero
        ;ldr r1, EndOfBSS
        mov  r1, r4              ; r1 - End of BSS

        mov r2, #0
zero_bss_loop
        cmp r0, r1
        beq zero_bss_end
        strb r2, [r0], #+1
        b zero_bss_loop
zero_bss_end

        ldr pc, GotoMain     ;-- goto main()

GotoMain   DCD   $MainEntry

        ;-- Return from main, loop forever.

exit_loop

        b exit_loop


;--  Location  of ROM and RAM  (heap also)

        IMPORT  |Image$$ER_RO$$Base|  ; start of ROM code
        IMPORT  |Image$$ER_RO$$Limit| ; end of ROM code
        IMPORT  |Image$$ER_RW$$Base|  ; destination address of initialised R/W
        IMPORT  |Image$$ER_RW$$Limit| ; used to size initialised R/W section
        IMPORT  |Image$$ER_ZI$$ZI$$Base|  ; start of the zero initialised (BSS) data.
        IMPORT  |Image$$ER_ZI$$ZI$$Limit| ; End of variable RAM space
        IMPORT  |Image$$ER_ZI$$ZI$$Length|

BaseOfROM     DCD |Image$$ER_RO$$Base|
TopOfROM      DCD |Image$$ER_RO$$Limit|
BaseOfBSS     DCD |Image$$ER_RW$$Base|
BaseOfZero    DCD |Image$$ER_ZI$$ZI$$Base|
EndOfBSS      DCD |Image$$ER_ZI$$ZI$$Limit|


        EXPORT __user_initial_stackheap

__user_initial_stackheap FUNCTION

        ldr r0,  EndOfBSS

        MOV   pc,lr
        ENDFUNC

        END


;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------
;------------------------------------------------------------------------------





