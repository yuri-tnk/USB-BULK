;
; TNKernel real-time kernel
;
; Copyright © 2004,2006 Yuri Tiomkin
; All rights reserved.
;
; Interrupt context switch -  this source code is derived
;   on code written by Michael Anburaj (http://geocities.com/michaelanburaj/)
;
; ffs_asm() - this is the ffs algorithm devised by D.Seal and posted to
;             comp.sys.arm on  16 Feb 1994.
;
; Permission to use, copy, modify, and distribute this software in source
; and binary forms and its documentation for any purpose and without fee
; is hereby granted, provided that the above copyright notice appear
; in all copies and that both that copyright notice and this permission
; notice appear in supporting documentation.
;
; THIS SOFTWARE IS PROVIDED BY THE YURI TIOMKIN AND CONTRIBUTORS "AS IS" AND
; ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED. IN NO EVENT SHALL YURI TIOMKIN OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
; OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
; OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
; SUCH DAMAGE.

  ;---  ver 2.1 -----

  ;---- KEIL(c) ARM assembler  ----

     AREA  tn_port, CODE

  ;-- External references


     EXTERN DATA   (tn_curr_run_task)
     EXTERN DATA   (tn_next_task_to_run)
     EXTERN DATA   (tn_int_counter)
     EXTERN DATA   (tn_context_switch_request)
     EXTERN CODE32 (tn_cpu_irq_handler?A)
     EXTERN DATA   (tn_system_state)


  ;-- Public functions declared in this file

     PUBLIC  tn_switch_context_exit?A
     PUBLIC  tn_switch_context?A
     PUBLIC  tn_cpu_irq_isr
     PUBLIC  tn_cpu_fiq_isr
     PUBLIC  tn_cpu_save_sr?A
     PUBLIC  tn_cpu_restore_sr?A
     PUBLIC  tn_start_exe?A
     PUBLIC  tn_chk_irq_disabled?A
     PUBLIC  ffs_asm?A


  ;-- Constants

     USERMODE   EQU  0x10
     FIQMODE    EQU  0x11
     IRQMODE    EQU  0x12
     SVCMODE    EQU  0x13
     ABORTMODE  EQU  0x17
     UNDEFMODE  EQU  0x1b
     MODEMASK   EQU  0x1f
     NOINT      EQU  0xc0           ;-- Disable both IRQ & FIR
     TBIT       EQU  0x20
     IRQMASK    EQU  0x80
     FIQMASK    EQU  0x40

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
tn_start_exe?A:

        ldr    r4,=tn_system_state  ; Indicate that system has started
        mov    r5,#1                  ;- 1 -> TN_SYS_STATE_RUNNING
        strb   r5,[r4]

tn_switch_context_exit?A:

     ; Does not save curr task context
        mrs    r0, CPSR
        orr    r0, r0, #NOINT
        msr    CPSR_c, r0

        ldr    r0, =tn_curr_run_task
        ldr    r1, =tn_next_task_to_run
        ldr    r1, [r1]                   ; get stack pointer
        ldr    sp, [r1]                   ; switch to the new stack

        str    r1, [r0]                   ; set new current running task tcb address

        ldmfd  sp!, {r0}                  ; get SPSR from current running task's stack
        ldmfd  sp!, {r0}                  ; get CPSR from current running task's stack
        msr    cpsr_c, r0                 ; cpsr - svc32

        mrs    r0, CPSR
        bic    r0, r0, #NOINT
        msr    CPSR_c, r0

        ldmfd  sp!, {r0-r12,lr,pc}

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
tn_switch_context?A:

        stmfd  sp!,{lr}                ;  lr have to be pushed instead pc

        stmfd  sp!, {r0-r12, lr}
        mrs    r0, CPSR
        stmfd  sp!, {r0}
        mrs    r0, SPSR
        stmfd  sp!, {r0}

        mrs    r4, CPSR
        orr    r4, r4, #NOINT
        msr    CPSR_c, r4

        ldr    r1,=tn_curr_run_task
        ldr    r0,=tn_next_task_to_run
        cmp    r0,#0
        beq    tn_sw_restore?A
        cmp    r1,r0
        beq    tn_sw_restore?A

        ldr    r2, [r1]
        str    sp, [r2]                ; store sp in preempted tasks's TCB
        ldr    r0, [r0]
        ldr    sp, [r0]                ; get new task's sp

      ;-- tn_curr_run_task = tn_next_task_to_run
        str    r0, [r1]

tn_sw_restore?A:

        ldmfd  sp!, {r0}
        ldmfd  sp!, {r1}
        msr    CPSR_cxsf, r1
        msr    SPSR_cxsf, r0

        mrs    r4, CPSR
        bic    r4, r4, #NOINT
        msr    CPSR_c, r4

        ldmfd  sp!, {r0-r12,lr,pc}

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
tn_cpu_irq_isr:

        stmfd  sp!,{r0-r12,lr}           ;-- Save ALL registers

       ;--  interrupt_counter++

        ldr    r2, =tn_int_counter
        ldr    r3, [r2]                  ;--interrupt_counter -> r3
        add    r0, r3,#1                 ;--interrupt_counter++
        str    r0, [r2]

        bl     tn_cpu_irq_handler?A      ;-- Actually  Handle interrupt

       ;-- interrupt_counter--

        ldr    r2, =tn_int_counter
        ldr    r1, [r2]
        sub    r3, r1,#1
        str    r3, [r2]
        cmp    r3, #0x00               ;-- if it is nested int - return
        bne    exit_irq_int?A          ;-- return from int
        ldr    r0, =tn_context_switch_request  ;-- see if we need to do a context switch
        ldr    r1, [R0]
        cmp    r1, #0                  ;-- if 0 - return 
        beq    exit_irq_int?A
        mov    r1, #0                  ;-- else - clear req flag and  goto context switch
        str    r1, [r0]
        b      tn_int_ctx_switch?A

exit_irq_int?A:

        ldmfd  sp!,{r0-r12,lr}
        subs   pc,lr,#4
     ;------------------------------

tn_int_ctx_switch?A:

    ;-- Our target - get all registers,saved in IRQ's stack for
    ;-- interrrupted task and save them in interrupted task's stack


       ;-- Switch to SVC

        mrs    r0,spsr
        orr    r0,r0,#NOINT
        bic    r0,r0,#TBIT
        msr    cpsr_c,r0

       ;-- Now we in SVC

        mov    r1,sp                ;-- interrupted task sp
        mov    r2,lr                ;-- interrupted task lr

        bic    r0,r0,#MODEMASK
        orr    r0,r0,#IRQMODE
        msr    cpsr_c,r0

       ;-- Now again in IRQ

       ;-- Unwind IRQ's sp without poping r0-r12

        mov    r3,sp
        add    sp,sp,#(4*12)
        ldmfd  sp!,{r12,lr}

       ;-- Here we are on IRQ entry point

        sub    lr,lr,#4                ;-- now in lr - addr to return
                                       ;-- for interrupted task

       ;-- in r1 - interrupted task's stack in point of interrupting
       ;-- in r2 - interrupted task's lr


        stmfd  r1!,{r2,lr}             ;-- interrupted task's pc & lr
        stmfd  r1!,{r4-r12}            ;-- interrupted task's r12-r4

       ;-- Get r0-r3 from IRQ's stack

        mov    r6,r1              ;-- now use r6 instead r1 since r1 will
                                       ;-- be reloaded below
        mov    r4,r3              ;-- r3(IRQ's sp) - in registers list to load
        ldmfd  r4,{r0-r3}         ;-- in r4 - IRQ's sp

        stmfd  r6!,{r0-r3}        ;-- interrupted task's r3-r0 */

        mrs    r3,spsr            ;-- in spsr - interrupted task's cpsr
        stmfd  r6!,{r3}           ;-- interrupted task's cpsr
        sub    r6,r6,#4           ;-- stack ptr decrement to point to SPSR
                                  ;-- but SPSR not store(case it not valid here)

       ;-- get current running task's TCB address

        ldr    r4,=tn_curr_run_task
        ldr    r5,[r4]
        str    r6,[r5]            ;-- store interrupted task's TCB in its stack

       ;-- get  ready to run task's TCB address

        ldr    r6,= tn_next_task_to_run
        ldr    r6,[r6]
        ldr    r5,[r6]            ;-- in r5 - new task's stack pointer

       ;-- tn_curr_run_task = tn_next_task_to_run

        str    r6,[r4]               ;-- set new current run task TCB address

        add    r5,r5,#4              ;-- Increment new running tack's stack ptr
                                         ;-- (simular to pop SPSR from task's stack)
        ldmfd  r5!,{r4}              ;-- pop new task's cpsr

       ;-- Leave IRQ mode

        orr    r0,r4,#NOINT
        bic    r0,r0,#TBIT
        msr    cpsr_c,r0

       ;-- Now - in SVC mode

        msr    spsr_cxsf,r4          ;-- in r4 - current task's cpsr
        mov    sp,r5                 ;-- in r5 - current task's sp

        ldmfd  sp!,{r0-r12,lr,pc}^   ;-- pop new running task's r0-r12,lr & pc

;----------------------------------------------------------------------------
;   Do nothing here
;----------------------------------------------------------------------------
tn_cpu_fiq_isr:

        stmfd  SP!,{R0-R3,R12,LR}
        ldmfd  SP!,{R0-R3,R12,LR}      ;-- restore registers of interrupted task's stack
        subs   PC,LR,#4                ;-- return from IRQ

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
tn_cpu_save_sr?A:

        mrs    r0,CPSR              ;-- Disable both IRQ & FIQ interrupts
        orr    r1,r0,#NOINT
        msr    CPSR_c,r1
   ;-- Atmel add-on
        mrs    r1,CPSR              ;-- Check CPSR for correct contents
        and    r1,r1,#NOINT
        cmp    r1,#NOINT
        bne    tn_cpu_save_sr?A     ;-- Not disabled - loop to try again
   ;--------
        mov    pc,lr

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
tn_cpu_restore_sr?A:

        msr    CPSR_c,r0
        mov    pc,lr

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
tn_chk_irq_disabled?A:

        mrs    r0,cpsr    ;-- Get cpsr
        and    r0,r0,#IRQMASK
        mov    pc,lr

;----------------------------------------------------------------------------
;
;----------------------------------------------------------------------------
ffs_asm?A:

      ;-- Standard trick to isolate bottom bit in r0 or 0 if r0 = 0 on entry
        rsb    r1, r0, #0
        ands   r0, r0, r1


        ;-- now r0 has at most one set bit, call this X
        ;-- if X = 0, all further instructions are skipped

        adrne  r2, _L_ffs_table
        orrne  r0, r0, r0, lsl #4       ;-- r0 = X * 0x11
        orrne  r0, r0, r0, lsl #6       ;-- r0 = X * 0x451
        rsbne  r0, r0, r0, lsl #16      ;-- r0 = X * 0x0450fbaf

        ;-- now lookup in table indexed on top 6 bits of r0
        ldrneb r0, [ r2, r0, lsr #26 ]
        bx     lr


_L_ffs_table:
             ;--   0   1   2   3   4   5   6   7
        DCB        0,  1,  2, 13,  3,  7,  0, 14  ;  0- 7
        DCB        4,  0,  8,  0,  0,  0,  0, 15  ;  8-15
        DCB       11,  5,  0,  0,  9,  0,  0, 26  ; 16-23
        DCB        0,  0,  0,  0,  0, 22, 28, 16  ; 24-31
        DCB       32, 12,  6,  0,  0,  0,  0,  0  ; 32-39
        DCB       10,  0,  0, 25,  0,  0, 21, 27  ; 40-47
        DCB       31,  0,  0,  0,  0, 24,  0, 20  ; 48-55
        DCB       30,  0, 23, 19, 29, 18, 17,  0  ; 56-63
;----------------------------------------------------------------------------

        END
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------
;----------------------------------------------------------------------------



