/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /*
   * Disable FPU to detect illegal usage of
   * floating point in kernel space
   */
  li t0, SR_FS
  csrc sstatus, t0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  csrr a0, CSR_MHARTID
  bnez a0, slave_core

  /* Clear BSS for flat non-ELF images */
  la a3, __bss_start
  la a4, __BSS_END__
  ble a4, a3, clear_bss_done
clear_bss:
  sd zero, (a3)
  add a3, a3, RISCV_SZPTR
  blt a3, a4, clear_bss
clear_bss_done:

  /* setup C environment */
  la tp, pid0_pcb_m
  lw sp, pid0_stack_m
  csrw CSR_SSCRATCH, x0
  j next
slave_core:
  la tp, pid0_pcb_s
  lw sp, pid0_stack_s
  csrw CSR_SSCRATCH, x0

next:
  la a0, riscv_dtb
  sd a1, (a0)

  call main

loop:
  wfi
  j loop

END(_start)
