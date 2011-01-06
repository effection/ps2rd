/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */
 
 /* Ever slightly modified by effection */

# ASM exception handlers


#include "r5900_regs.h"
        
.set noat
.set noreorder

.text
.p2align 4

        .global cpuRegs
     
        # Save all user regs
        # Save HI/LO, SR, BadVAddr, Cause, EPC
        # ShiftAmount, cop0: $24, $25
        # Save float regs??
        # Set EPC to debugger
        # Set stack to 'exception stack'
        # eret
        .global lv1ExceptionHandler
        .ent	lv1ExceptionHandler
lv1ExceptionHandler:
        la      k0, cpuRegs
        sq      zero,0x00(k0)
        sq      at,  0x10(k0)
		# v0 - v1
        sq      v0,  0x20(k0)
        sq      v1,  0x30(k0)
		# a0 - a3
        sq      a0,  0x40(k0)
        sq      a1,  0x50(k0)
        sq      a2,  0x60(k0)
        sq      a3,  0x70(k0)
		# t0 - t7
        sq      t0,  0x80(k0)
        sq      t1,  0x90(k0)
        sq      t2,  0xa0(k0)
        sq      t3,  0xb0(k0)
        sq      t4,  0xc0(k0)
        sq      t5,  0xd0(k0)
        sq      t6,  0xe0(k0)
        sq      t7,  0xf0(k0)
		# s0 - s7
		sq      s0,  0x100(k0)
        sq      s1,  0x110(k0)
        sq      s2,  0x120(k0)
        sq      s3,  0x130(k0)
        sq      s4,  0x140(k0)
        sq      s5,  0x150(k0)
        sq      s6,  0x160(k0)
        sq      s7,  0x170(k0)
		# t8 - t9
        sq      t8,  0x180(k0)
        sq      t9,  0x190(k0)
        # k0, k1, gp,  sp, fp, ra
#       sq      k0,  0x1a0(k0)		# $k0
        sq      zero,0x1a0(k0)		# zero instead
        sq      k1,  0x1b0(k0)
        sq      gp,  0x1c0(k0)
        sq      sp,  0x1d0(k0) 
        sq      fp,  0x1e0(k0)
        sq      ra,  0x1f0(k0) 
		# HI, LO
        pmfhi   t0
        pmflo   t1
        sq      t0, 0x200(k0)
        sq      t1, 0x210(k0)
		
		# Store ShiftAmount at 0x220(k0)
		mfsa    t0
        sw      t0, 0x220(k0)
		# Store BadBAddr @ (0x224 + (8 * 4))(k0)
		mfc0    t0, BadVAddr
		# Store Status @ (0x224 + (12 * 4))(k0)
		mfc0    t1, Status
		sw      t0, 0x244(k0)
        sw      t1, 0x254(k0)
		# Store Cause @ (0x224 + (13 * 4))(k0)
		mfc0    t0, Cause
		# Store EPC @ (0x224 + (14 * 4))(k0)
        mfc0    t1, EPC
        sw      t0, 0x258(k0)
        sw      t1, 0x25c(k0)

        # Kernel saves these two also..
		# Store DEPC @ (0x224 + (24 * 4))(k0)
        mfc0    t0, DEPC
		# Store PerfCnt @ (0x224 + (25 * 4))(k0)
        mfc0    t1, PerfCnt
        sw      t0, 0x284(k0)
        sw      t1, 0x288(k0)

        # Use our own stack..
        la      sp, _exceptionStack+0x2000-16
        la 		gp, _gp                 # Use exception handlers _gp TODO sort out GP
        
        # Return from exception and start debugger
        daddiu	a0, zero, 1				# arg0 - exception level
        la      k0, exceptionDebugger
        mtc0    k0, EPC                 # eret return address
        sync.p
        mfc0    k0, Status              # check this out..
        li      v0, 0xfffffffe
        and     k0, v0
        mtc0    k0, Status
        sync.p
        nop
        nop
        nop
        nop
        eret
        nop
        .end	lv1ExceptionHandler

        
        # Put EE in kernel mode
        # Restore all user regs etc
        # Restore PC? & Stack ptr
        # Restore interrupt sources
        # Jump to EPC
        .ent returnFromLv1Exception
        .global returnFromLv1Exception
returnFromLv1Exception:

        lui     t1, 0x1
_disable:
        di
        sync
        mfc0    t0, Status
        and     t0, t1
        beqz    t0, _disable
        nop
        
        la      k0, cpuRegs
                
		# Restore HI, LO
		lq      t0, 0x200(k0)
        lq      t1, 0x210(k0)
        pmthi   t0
        pmtlo   t1
		
		# Restore ShiftAmount at 0x220(k0)
		lw      t0, 0x220(k0)
		mtsa    t0
        
		# Restore BadBAddr @ (0x224 + (8 * 4))(k0)
		lw      t0, 0x244(k0)
		mtc0    t0, BadVAddr
		# Restore Status @ (0x224 + (12 * 4))(k0)
		lw      t1, 0x254(k0)
		mtc0    t1, Status
		# Restore Cause @ (0x224 + (13 * 4))(k0)
		lw      t0, 0x258(k0)
		mtc0    t0, Cause
		# Restore EPC @ (0x224 + (14 * 4))(k0)
		lw      t1, 0x25c(k0)
        mtc0    t1, EPC        

        # Kernel saves these two also..
		# Restore DEPC @ (0x224 + (24 * 4))(k0)
		lw      t0, 0x284(k0)
        mtc0    t0, DEPC
		# Restore PerfCnt @ (0x224 + (25 * 4))(k0)
		lw      t1, 0x288(k0)
        mtc0    t1, PerfCnt

		# GPRs

#        ori     t2, 0xff
#        sw      t2, 0(k1)
        
#		lq      zero,0x00(k0)
        lq      at,  0x10(k0)
		# v0 - v1
        lq      v0,  0x20(k0)
        lq      v1,  0x30(k0)
		# a0 - a3
        lq      a0,  0x40(k0)
        lq      a1,  0x50(k0)
        lq      a2,  0x60(k0)
        lq      a3,  0x70(k0)
		# t0 - t7
        lq      t0,  0x80(k0)
        lq      t1,  0x90(k0)
        lq      t2,  0xa0(k0)
        lq      t3,  0xb0(k0)
        lq      t4,  0xc0(k0)
        lq      t5,  0xd0(k0)
        lq      t6,  0xe0(k0)
        lq      t7,  0xf0(k0)
		# s0 - s7
		lq      s0,  0x100(k0)
        lq      s1,  0x110(k0)
        lq      s2,  0x120(k0)
        lq      s3,  0x130(k0)
        lq      s4,  0x140(k0)
        lq      s5,  0x150(k0)
        lq      s6,  0x160(k0)
        lq      s7,  0x170(k0)
		# t8 - t9
        lq      t8,  0x180(k0)
        lq      t9,  0x190(k0)
        # k0, k1, gp,  sp, fp, ra
#       lq      k0,  0x1a0(k0)
        lq      k1,  0x1b0(k0)
        lq      gp,  0x1c0(k0)
        lq      sp,  0x1d0(k0) 
        lq      fp,  0x1e0(k0)
        lq      ra,  0x1f0(k0) 

		# Load  EPC which has already been modified to not re-execute next the same instruction
		lw      k0, 0x25c(k0)

        sync.p
        ei
        sync.p
		
        jr      k0
        nop        
        .end returnFromLv1Exception