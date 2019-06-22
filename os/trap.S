// Copyright (c) 2013, The Regents of the University of California (Regents).
// All Rights Reserved
// See LICENSE.SiFive for license details.
// Modifications copyright (c) Boran Car <boran.car@gmail.com>

#include "bits.h"

.section .init
.align 8
.globl trap_vector
trap_vector:
	csrw sscratch, sp
	addi sp, sp, -32*REGBYTES
	STORE a0, 10*REGBYTES(sp)
	STORE a1, 11*REGBYTES(sp)

	csrr a1, scause
	bgez a1, .Lhandle_trap_in_supervisor_mode

.Lhandle_trap_in_supervisor_mode:
  # Preserve the registers.
  STORE ra, 1*REGBYTES(sp)
  STORE gp, 3*REGBYTES(sp)
  STORE tp, 4*REGBYTES(sp)
  STORE t0, 5*REGBYTES(sp)
  STORE t1, 6*REGBYTES(sp)
  STORE t2, 7*REGBYTES(sp)
  STORE s0, 8*REGBYTES(sp)
  STORE s1, 9*REGBYTES(sp)
  mv a0, sp                        # a0 <- regs
  STORE a2,12*REGBYTES(sp)
  csrr a2, sepc                    # a2 <- sepc
  STORE a3,13*REGBYTES(sp)
  csrrw t0, sscratch, x0           # t0 <- user sp
  STORE a4,14*REGBYTES(sp)
  STORE a5,15*REGBYTES(sp)
  STORE a6,16*REGBYTES(sp)
  STORE a7,17*REGBYTES(sp)
  STORE s2,18*REGBYTES(sp)
  STORE s3,19*REGBYTES(sp)
  STORE s4,20*REGBYTES(sp)
  STORE s5,21*REGBYTES(sp)
  STORE s6,22*REGBYTES(sp)
  STORE s7,23*REGBYTES(sp)
  STORE s8,24*REGBYTES(sp)
  STORE s9,25*REGBYTES(sp)
  STORE s10,26*REGBYTES(sp)
  STORE s11,27*REGBYTES(sp)
  STORE t3,28*REGBYTES(sp)
  STORE t4,29*REGBYTES(sp)
  STORE t5,30*REGBYTES(sp)
  STORE t6,31*REGBYTES(sp)
  STORE t0, 2*REGBYTES(sp)         # sp

#ifndef __riscv_flen
  lw tp, (sp) # Move the emulated FCSR from x0's save slot into tp.
#endif
  STORE x0, (sp) # Zero x0's save slot.

  # Invoke the handler.
  jal trap

#ifndef __riscv_flen
  sw tp, (sp) # Move the emulated FCSR from tp into x0's save slot.
#endif

restore_sscratch:
  csrw sscratch, sp

restore_regs:
  # Restore all of the registers.
  LOAD ra, 1*REGBYTES(sp)
  LOAD gp, 3*REGBYTES(sp)
  LOAD tp, 4*REGBYTES(sp)
  LOAD t0, 5*REGBYTES(sp)
  LOAD t1, 6*REGBYTES(sp)
  LOAD t2, 7*REGBYTES(sp)
  LOAD s0, 8*REGBYTES(sp)
  LOAD s1, 9*REGBYTES(sp)
  LOAD a0,10*REGBYTES(sp)
  LOAD a1,11*REGBYTES(sp)
  LOAD a2,12*REGBYTES(sp)
  LOAD a3,13*REGBYTES(sp)
  LOAD a4,14*REGBYTES(sp)
  LOAD a5,15*REGBYTES(sp)
  LOAD a6,16*REGBYTES(sp)
  LOAD a7,17*REGBYTES(sp)
  LOAD s2,18*REGBYTES(sp)
  LOAD s3,19*REGBYTES(sp)
  LOAD s4,20*REGBYTES(sp)
  LOAD s5,21*REGBYTES(sp)
  LOAD s6,22*REGBYTES(sp)
  LOAD s7,23*REGBYTES(sp)
  LOAD s8,24*REGBYTES(sp)
  LOAD s9,25*REGBYTES(sp)
  LOAD s10,26*REGBYTES(sp)
  LOAD s11,27*REGBYTES(sp)
  LOAD t3,28*REGBYTES(sp)
  LOAD t4,29*REGBYTES(sp)
  LOAD t5,30*REGBYTES(sp)
  LOAD t6,31*REGBYTES(sp)
  LOAD sp, 2*REGBYTES(sp)
  sret
