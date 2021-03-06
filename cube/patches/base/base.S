#***************************************************************************
# Device indepentent Read code launch pad
#
# we have 0x1800 bytes to play with at 0x80001800 (code+data)
# This code is placed at 0x80001800
#**************************************************************************
#define _LANGUAGE_ASSEMBLY
#include "../../reservedarea.h"

.extern __DSPHandler
.section .text
	.globl _start, __main
_start:
	b		calculate_speed			# Calculates how long it takes to read 1024 bytes from the device
	b		patched_memcpy			# void patched_memcpy(dst,src,size)
	b		stop_di_irq
	b		trigger_di_irq
	b		__DSPHandler
	b		check_pad
	b		igr_exit

	.globl trigger_di_irq
trigger_di_irq:
	stwu	sp,	-64(sp) 		# grow stack frame
	mflr	r0
	stw		r0,   12(sp)		# store lr

	mfmsr	r5
	stw		r5,	  32(sp)		# store old msr
	rlwinm	r5,r5,0,17,15
	mtmsr	r5
	
	bl		DIUpdateRegisters

	lwz		r5,	  32(sp)		# restore old msr
	mtmsr	r5
	lwz		r0,   12(sp)		# restore lr
	mtlr	r0
	addi	sp, sp, 64
	blr

	.globl stop_di_irq
stop_di_irq:
	lis		r3, 0xCC00
	lwz		r0, 0x6000(r3)
	stw		r0, 0x6000(r3)
	li		r0, 0
	lis		r3, VAR_AREA
	stw		r0, VAR_FAKE_IRQ_SET(r3)
	blr

	.globl dcache_flush_icache_inv
dcache_flush_icache_inv:
	clrlwi. 	5, 3, 27  # check for lower bits set in address
	beq 1f
	addi 		r4, r4, 0x20 
1:
	addi 		r4, r4, 0x1f
	srwi 		r4, r4, 5
	mtctr 		r4
2:
	dcbf 		r0, r3
	icbi		r0, r3		#todo kill this off
	addi 		r3, r3, 0x20
	bdnz 		2b
	sc
	sync
	isync
	blr
	
	.globl process_read_queue
process_read_queue:
	stwu	sp,	-64(sp) 		# grow stack frame
	stw		r0,   12(sp)		# store r0
	stw		r3,   16(sp)		# store r3
	stw		r4,   20(sp)		# store r4
	stw		r5,   24(sp)		# store r5
	stw		r6,   28(sp)		# store r6
	stw		r7,   32(sp)		# store r7
	stw		r8,   36(sp)		# store r8
	stw		r9,   40(sp)		# store r9
	stw		r10,   44(sp)		# store r10
	stw		r11,   48(sp)		# store r11
	stw		r12,   52(sp)		# store r12
	
	bl		StreamUpdateRegisters
	bl		DIUpdateRegisters

	lwz		r0, 12(sp)			# restore r0
	lwz		r4,	20(sp)			# restore r4 (context)
	lwz     r3, 128 (r4)
	mtcr    r3					# restore cr
	lwz     r3, 0x0084 (r4)
	mtlr    r3					# restore lr
	lwz     r3, 0x0088 (r4)
	mtctr   r3					# restore ctr
	lwz     r3, 0x008C (r4)
	mtxer   r3					# restore xer
	lwz		r3,   16(sp)		# restore r3
	lwz		r5,   24(sp)		# restore r5
	lwz		r6,   28(sp)		# restore r6
	lwz		r7,   32(sp)		# restore r7
	lwz		r8,   36(sp)		# restore r8
	lwz		r9,   40(sp)		# restore r9
	lwz		r10,   44(sp)		# restore r10
	lwz		r11,   48(sp)		# restore r11
	lis		r12, 0x8000
	lwz		r12, 0x3010(r12)
	mtsrr0	r12					# setup for the rfi
	lwz		r12,   52(sp)		# restore r12
	addi	r1,	r1, 64			# kill stack frame
	rfi							# jump to handler

.globl patched_memcpy
patched_memcpy:
	subis	r0, r3, 0x8000
	cmplwi	r0, 0x1000
	bgelr
	cmplwi	r0, 0x0500
	bne		memcpy
	lis		r4, ext_handler_custom@ha
	addi	r4, r4, ext_handler_custom@l
	li		r5, ext_handler_custom_end-ext_handler_custom
	b		memcpy

.globl ext_handler_custom
ext_handler_custom:
	mtsprg0     r4
	lwz         r4, 0x00C0 (r0)
	stw         r3, 12 (r4)
	mfsprg0     r3
	stw         r3, 16 (r4)
	stw         r5, 20 (r4)
	lhz         r3, 0x01A2 (r4)
	ori         r3, r3, 0x0002
	sth         r3, 0x01A2 (r4)
	mfcr        r3
	stw         r3, 128 (r4)
	mflr        r3
	stw         r3, 0x0084 (r4)
	mfctr       r3
	stw         r3, 0x0088 (r4)
	mfxer       r3
	stw         r3, 0x008C (r4)
	mfsrr0      r3
	stw         r3, 0x0198 (r4)
	mfsrr1      r3
	stw         r3, 0x019C (r4)
	mr          r5, r3
	nop
	mfmsr       r3
	ori         r3, r3, 0x0030
	mtsrr1      r3
	li          r3, 4
	lwz         r4, 0x00D4 (r0)
	lis			r5, process_read_queue@h
	ori			r5, r5, process_read_queue@l
	mtsrr0      r5
	rfi
ext_handler_custom_end:

	.globl appldr_start
appldr_start:
	lis			r3, device_frag_read@h
	ori			r3, r3, device_frag_read@l
	lis			r4, EXECD_RUNNER@h
	ori			r4, r4, EXECD_RUNNER@l
	mtctr		r4
	bctr